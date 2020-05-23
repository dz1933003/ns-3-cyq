/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/dpsk-module.h"
#include "ns3/pfc-module.h"
#include "ns3/rdma-module.h"
#include "ns3/internet-module.h"

#include <fstream>
#include <map>
#include <set>

#include "json.hpp"
#include "csv.hpp"
#include "data-size.hpp"

using namespace ns3;
using json = nlohmann::json;

static const uint32_t CYQ_MTU = 1500;
static DpskHelper dpskHelper;

uint32_t nQueue;
uint32_t ecmpSeed;
DpskNetDevice::TxMode portTxMode;
std::map<std::string, Ptr<Node>> allNodes;
std::map<Ptr<Node>, std::vector<Ptr<DpskNetDevice>>> allPorts;
std::map<Ptr<Node>, Ipv4Address> allIpv4Addresses;
std::set<Ptr<Node>> hostNodes;
std::set<Ptr<Node>> switchNodes;

struct Interface
{
  Ptr<DpskNetDevice> device;
  Time delay;
  DataRate bandwidth;
};
std::map<Ptr<Node>, std::map<Ptr<Node>, Interface>> nbr2if;
std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>>>> nextHop;
std::map<Ptr<Node>, std::map<Ptr<Node>, Time>> pairDelay;
std::map<Ptr<Node>, std::map<Ptr<Node>, Time>> pairTxDelay;
std::map<Ptr<Node>, std::map<Ptr<Node>, DataRate>> pairBw;
std::map<Ptr<Node>, std::map<Ptr<Node>, uint64_t>> pairBdp; // byte
std::map<Ptr<Node>, std::map<Ptr<Node>, Time>> pairRtt;

void ConfigMmuPort (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile);
void ConfigMmuQueue (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile);
void ConfigMmuQueue (Ptr<Node> node, Ptr<SwitchMmu> mmu, Ptr<NetDevice> port,
                     const std::string &configFile);

void CalculateRoute ();
void CalculateRoute (Ptr<Node> host);

void SetRoutingEntries ();

NS_LOG_COMPONENT_DEFINE ("PFC HW");

int
main (int argc, char *argv[])
{
  if (argc != 2)
    {
      NS_LOG_UNCOND ("ERROR: No config file");
      return 1;
    }

  std::ifstream file (argv[1]);
  json conf = json::parse (file);

  NS_LOG_UNCOND ("====Global====");
  nQueue = conf["Global"]["QueueNumber"];
  ecmpSeed = conf["Global"]["EcmpSeed"];
  portTxMode = DpskNetDevice::TxMode::ACTIVE;
  NS_LOG_UNCOND ("QueueNumber: " << nQueue);
  NS_LOG_UNCOND ("EcmpSeed: " << ecmpSeed);

  NS_LOG_UNCOND ("====Host====");
  for (const auto &host : conf["Host"])
    {
      for (const auto &name : host["Name"])
        {
          // Create host node
          const Ptr<Node> node = CreateObject<Node> ();
          hostNodes.insert (node);
          allNodes[name] = node;
          allIpv4Addresses[node] = Ipv4AddressGenerator::NextAddress (Ipv4Mask ("255.0.0.0"));
          // Install ports
          for (size_t i = 0; i < host["PortNumber"]; i++)
            {
              const Ptr<DpskNetDevice> dev = CreateObject<DpskNetDevice> ();
              dev->SetAddress (Mac48Address::Allocate ());
              dev->SetTxMode (portTxMode);
              node->AddDevice (dev);
              const Ptr<PfcHostPort> impl = CreateObject<PfcHostPort> ();
              dev->SetImplementation (impl);
              dev->AggregateObject (impl);
              impl->SetupQueues (nQueue);
              allPorts[node].push_back (dev);
            }
          // Install DPSK
          const auto dpsk = dpskHelper.Install (node);
          // Install PFC host DPSK layer
          auto pfcHost = CreateObject<PfcHost> ();
          node->AggregateObject (pfcHost);
          pfcHost->InstallDpsk (dpsk);
        }
    }

  NS_LOG_UNCOND ("====Switch====");
  for (const auto &sw : conf["Switch"])
    {
      for (const auto &name : sw["Name"])
        {
          // Create switch node
          const Ptr<Node> node = CreateObject<Node> ();
          switchNodes.insert (node);
          allNodes[name] = node;
          // Install ports
          for (size_t i = 0; i < sw["PortNumber"]; i++)
            {
              const Ptr<DpskNetDevice> dev = CreateObject<DpskNetDevice> ();
              dev->SetAddress (Mac48Address::Allocate ());
              dev->SetTxMode (portTxMode);
              node->AddDevice (dev);
              const Ptr<PfcSwitchPort> impl = CreateObject<PfcSwitchPort> ();
              dev->SetImplementation (impl);
              dev->AggregateObject (impl);
              impl->SetupQueues (nQueue);
              allPorts[node].push_back (dev);
            }
          // Install DPSK
          const auto dpsk = dpskHelper.Install (node);
          // Install PFC switch DPSK layer
          const auto pfcSwitch = CreateObject<PfcSwitch> ();
          node->AggregateObject (pfcSwitch);
          pfcSwitch->InstallDpsk (dpsk);
          pfcSwitch->SetEcmpSeed (ecmpSeed);
          pfcSwitch->SetNQueues (nQueue);
          // Install switch MMU
          const auto mmu = CreateObject<SwitchMmu> ();
          pfcSwitch->InstallMmu (mmu);
          pfcSwitch->AggregateObject (mmu);
          const uint64_t buffer = cyq::DataSize::GetBytes (sw["Config"]["Buffer"]);
          mmu->ConfigBufferSize (buffer);
          ConfigMmuPort (node, mmu, sw["Config"]["ConfigFile"]);
          NS_LOG_DEBUG (mmu->Dump ());
        }
    }

  NS_LOG_UNCOND ("====Link====");
  io::CSVReader<6> linkConfig (conf["LinkConfigFile"]);
  linkConfig.read_header (io::ignore_no_column, "FromNode", "FromPort", "ToNode", "ToPort",
                          "DataRate", "Delay");
  while (true)
    {
      std::string fromNode, toNode;
      size_t fromPort, toPort;
      std::string dataRateInput;
      std::string delayInput;
      if (!linkConfig.read_row (fromNode, fromPort, toNode, toPort, dataRateInput, delayInput))
        break;
      const auto dataRate = DataRate (dataRateInput);
      const auto delay = Time (delayInput);
      const auto s_node = allNodes[fromNode];
      const auto d_node = allNodes[toNode];
      const auto s_dev = allPorts[s_node][fromPort];
      const auto d_dev = allPorts[d_node][toPort];
      const Ptr<DpskChannel> channel = CreateObject<DpskChannel> ();
      s_dev->SetAttribute ("DataRate", DataRateValue (dataRate));
      d_dev->SetAttribute ("DataRate", DataRateValue (dataRate));
      channel->SetAttribute ("Delay", TimeValue (delay));
      s_dev->Attach (channel);
      d_dev->Attach (channel);
      nbr2if[s_node][d_node] = {.device = s_dev, .delay = delay, .bandwidth = dataRate};
      nbr2if[d_node][s_node] = {.device = d_dev, .delay = delay, .bandwidth = dataRate};
    }

  NS_LOG_UNCOND ("====Route====");
  CalculateRoute ();
  SetRoutingEntries ();

  NS_LOG_UNCOND ("====Flow====");
  io::CSVReader<7> flowConfig (conf["FlowConfigFile"]);
  flowConfig.read_header (io::ignore_no_column, "StartTime", "FromNode", "ToNode", "SourcePort",
                          "DestinationPort", "Size", "Priority");
  while (true)
    {
      std::string startInput;
      std::string fromNode, toNode;
      uint16_t sourcePort, destinationPort;
      std::string sizeInput;
      uint16_t priority;
      if (!flowConfig.read_row (startInput, fromNode, toNode, sourcePort, destinationPort,
                                sizeInput, priority))
        break;
      const auto startTime = Time (startInput);
      const auto sourceIp = allIpv4Addresses[allNodes[fromNode]];
      const auto destinationIp = allIpv4Addresses[allNodes[toNode]];
      const uint64_t size = cyq::DataSize::GetBytes (sizeInput);
      auto qp = CreateObject<RdmaTxQueuePair> (startTime, sourceIp, destinationIp,
                                                     sourcePort, destinationPort, size, priority);
      auto dpskLayer = allNodes[fromNode]->GetObject<PfcHost> ();
      dpskLayer->AddRdmaTxQueuePair (qp);
    }

  // TODO cyq: add trace and logs

  NS_LOG_UNCOND ("====Simulate====");
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND ("====Log====");
  NS_LOG_UNCOND ("====Done====");
}

void
ConfigMmuPort (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile)
{
  std::ifstream file (configFile);
  json conf = json::parse (file);
  for (const auto &port : conf)
    {
      if (port["PortIndex"].is_string () && port["PortIndex"] == "all")
        {
          ConfigMmuQueue (node, mmu, port["QueueConfigFile"]);
        }
      else if (port["PortIndex"].is_array ())
        {
          for (const auto &index : port["PortIndex"])
            {
              ConfigMmuQueue (node, mmu, allPorts[node][index], port["QueueConfigFile"]);
            }
        }
    }
}

void
ConfigMmuQueue (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile)
{
  std::ifstream file (configFile);
  json conf = json::parse (file);
  for (const auto &queue : conf)
    {
      if (queue["QueueIndex"].is_string () && queue["QueueIndex"] == "all")
        {
          if (queue.contains ("Ecn"))
            {
              const uint64_t kMin = cyq::DataSize::GetBytes (queue["Ecn"]["kMin"]);
              const uint64_t kMax = cyq::DataSize::GetBytes (queue["Ecn"]["kMax"]);
              const double pMax = queue["Ecn"]["pMax"];
              mmu->ConfigEcn (kMin, kMax, pMax);
            }
          if (queue.contains ("Headroom"))
            {
              const uint64_t headroom = cyq::DataSize::GetBytes (queue["Headroom"]);
              mmu->ConfigHeadroom (headroom);
            }
          if (queue.contains ("Reserve"))
            {
              const uint64_t reserve = cyq::DataSize::GetBytes (queue["Reserve"]);
              mmu->ConfigReserve (reserve);
            }
          if (queue.contains ("ResumeOffset"))
            {
              const uint64_t resumeOffset = cyq::DataSize::GetBytes (queue["ResumeOffset"]);
              mmu->ConfigResumeOffset (resumeOffset);
            }
        }
      else if (queue["QueueIndex"].is_array ())
        {
          for (const auto &index : queue["QueueIndex"])
            {
              if (queue.contains ("Ecn"))
                {
                  const uint64_t kMin = cyq::DataSize::GetBytes (queue["Ecn"]["kMin"]);
                  const uint64_t kMax = cyq::DataSize::GetBytes (queue["Ecn"]["kMax"]);
                  const double pMax = queue["Ecn"]["pMax"];
                  mmu->ConfigEcn (index, kMin, kMax, pMax);
                }
              if (queue.contains ("Headroom"))
                {
                  const uint64_t headroom = cyq::DataSize::GetBytes (queue["Headroom"]);
                  mmu->ConfigHeadroom (index, headroom);
                }
              if (queue.contains ("Reserve"))
                {
                  const uint64_t reserve = cyq::DataSize::GetBytes (queue["Reserve"]);
                  mmu->ConfigReserve (index, reserve);
                }
              if (queue.contains ("ResumeOffset"))
                {
                  const uint64_t resumeOffset = cyq::DataSize::GetBytes (queue["ResumeOffset"]);
                  mmu->ConfigResumeOffset (index, resumeOffset);
                }
            }
        }
    }
}

void
ConfigMmuQueue (Ptr<Node> node, Ptr<SwitchMmu> mmu, Ptr<NetDevice> port,
                const std::string &configFile)
{
  std::ifstream file (configFile);
  json conf = json::parse (file);
  for (const auto &queue : conf)
    {
      if (queue["QueueIndex"].is_string () && queue["QueueIndex"] == "all")
        {
          if (queue.contains ("Ecn"))
            {
              const uint64_t kMin = cyq::DataSize::GetBytes (queue["Ecn"]["kMin"]);
              const uint64_t kMax = cyq::DataSize::GetBytes (queue["Ecn"]["kMax"]);
              const double pMax = queue["Ecn"]["pMax"];
              mmu->ConfigEcn (port, kMin, kMax, pMax);
            }
          if (queue.contains ("Headroom"))
            {
              const uint64_t headroom = cyq::DataSize::GetBytes (queue["Headroom"]);
              mmu->ConfigHeadroom (port, headroom);
            }
          if (queue.contains ("Reserve"))
            {
              const uint64_t reserve = cyq::DataSize::GetBytes (queue["Reserve"]);
              mmu->ConfigReserve (port, reserve);
            }
          if (queue.contains ("ResumeOffset"))
            {
              const uint64_t resumeOffset = cyq::DataSize::GetBytes (queue["ResumeOffset"]);
              mmu->ConfigResumeOffset (port, resumeOffset);
            }
        }
      else if (queue["QueueIndex"].is_array ())
        {
          for (const auto &index : queue["QueueIndex"])
            {
              if (queue.contains ("Ecn"))
                {
                  const uint64_t kMin = cyq::DataSize::GetBytes (queue["Ecn"]["kMin"]);
                  const uint64_t kMax = cyq::DataSize::GetBytes (queue["Ecn"]["kMax"]);
                  const double pMax = queue["Ecn"]["pMax"];
                  mmu->ConfigEcn (port, index, kMin, kMax, pMax);
                }
              if (queue.contains ("Headroom"))
                {
                  const uint64_t headroom = cyq::DataSize::GetBytes (queue["Headroom"]);
                  mmu->ConfigHeadroom (port, index, headroom);
                }
              if (queue.contains ("Reserve"))
                {
                  const uint64_t reserve = cyq::DataSize::GetBytes (queue["Reserve"]);
                  mmu->ConfigReserve (port, index, reserve);
                }
              if (queue.contains ("ResumeOffset"))
                {
                  const uint64_t resumeOffset = cyq::DataSize::GetBytes (queue["ResumeOffset"]);
                  mmu->ConfigResumeOffset (port, index, resumeOffset);
                }
            }
        }
    }
}

void
CalculateRoute ()
{
  for (const auto &host : hostNodes)
    {
      CalculateRoute (host);
    }
}

void
CalculateRoute (Ptr<Node> host)
{
  // queue for the BFS.
  std::vector<Ptr<Node>> q;
  // Distance from the host to each node.
  std::map<Ptr<Node>, int> dis;
  std::map<Ptr<Node>, Time> delay;
  std::map<Ptr<Node>, Time> txDelay;
  std::map<Ptr<Node>, DataRate> bw;
  // init BFS.
  q.push_back (host);
  dis[host] = 0;
  delay[host] = Time (0);
  txDelay[host] = Time (0);
  bw[host] = DataRate (UINT64_MAX);
  // BFS.
  for (int i = 0; i < (int) q.size (); i++)
    {
      Ptr<Node> now = q[i];
      int d = dis[now];
      for (auto it = nbr2if[now].begin (); it != nbr2if[now].end (); it++)
        {
          Ptr<Node> next = it->first;
          // If 'next' have not been visited.
          if (dis.find (next) == dis.end ())
            {
              dis[next] = d + 1;
              delay[next] = delay[now] + it->second.delay;
              txDelay[next] = txDelay[now] + it->second.bandwidth.CalculateBytesTxTime (CYQ_MTU);
              bw[next] = std::min (bw[now], it->second.bandwidth);
              // we only enqueue switch, because we do not want packets to go through host as middle point
              if (switchNodes.find (next) != switchNodes.end ())
                q.push_back (next);
            }
          // if 'now' is on the shortest path from 'next' to 'host'.
          if (d + 1 == dis[next])
            {
              nextHop[next][host].push_back (now);
            }
        }
    }
  for (auto it : delay)
    pairDelay[it.first][host] = it.second;
  for (auto it : txDelay)
    pairTxDelay[it.first][host] = it.second;
  for (auto it : bw)
    pairBw[it.first][host] = it.second;
}

void
SetRoutingEntries ()
{
  // For each node.
  for (auto i = nextHop.begin (); i != nextHop.end (); i++)
    {
      Ptr<Node> node = i->first;
      auto &table = i->second;
      for (auto j = table.begin (); j != table.end (); j++)
        {
          // The destination node.
          Ptr<Node> dst = j->first;
          // The IP address of the dst.
          Ipv4Address dstAddr = allIpv4Addresses[dst];
          // The next hops towards the dst.
          std::vector<Ptr<Node>> nexts = j->second;
          for (int k = 0; k < (int) nexts.size (); k++)
            {
              Ptr<Node> next = nexts[k];
              auto interface = nbr2if[node][next].device;
              if (switchNodes.find (node) != switchNodes.end ())
                {
                  auto dpskLayer = node->GetObject<PfcSwitch> ();
                  dpskLayer->AddRouteTableEntry (dstAddr, interface);
                }
              else
                {
                  auto dpskLayer = node->GetObject<PfcHost> ();
                  dpskLayer->AddRouteTableEntry (dstAddr, interface);
                }
            }
        }
    }
}