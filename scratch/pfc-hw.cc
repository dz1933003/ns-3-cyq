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
#include "cyq-utils.hpp"

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
std::set<Ptr<RdmaTxQueuePair>> allTxQueuePairs;
std::map<uint32_t, Ptr<RdmaRxQueuePair>> allRxQueuePairs;

uint64_t maxBdp = 0;
Time maxRtt (0);

struct Interface
{
  Ptr<DpskNetDevice> device;
  Time delay;
  DataRate bandwidth;
};
std::map<Ptr<Node>, std::map<Ptr<Node>, Interface>> nbr2if;
std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>>>> nextHopTable;
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
void CalculateRttBdp ();

void DoTrace (const std::string &configFile);

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
              impl->SetupQueues (nQueue);
              allPorts[node].push_back (dev);
            }
          // Install DPSK
          const auto dpsk = dpskHelper.Install (node);
          // Install PFC host DPSK layer
          auto pfcHost = CreateObject<PfcHost> ();
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
              impl->SetupQueues (nQueue);
              allPorts[node].push_back (dev);
            }
          // Install DPSK
          const auto dpsk = dpskHelper.Install (node);
          // Install PFC switch DPSK layer
          const auto pfcSwitch = CreateObject<PfcSwitch> ();
          pfcSwitch->InstallDpsk (dpsk);
          pfcSwitch->SetEcmpSeed (ecmpSeed);
          pfcSwitch->SetNQueues (nQueue);
          // Install switch MMU
          const auto mmu = CreateObject<SwitchMmu> ();
          pfcSwitch->InstallMmu (mmu);
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
  CalculateRttBdp ();
  NS_LOG_UNCOND ("Max RTT: " << maxRtt);
  NS_LOG_UNCOND ("Max BDP: " << maxBdp);

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
      auto txQp = CreateObject<RdmaTxQueuePair> (startTime, sourceIp, destinationIp, sourcePort,
                                                 destinationPort, size, priority);
      auto sendDpskLayer = allNodes[fromNode]->GetObject<PfcHost> ();
      sendDpskLayer->AddRdmaTxQueuePair (txQp);
      auto receiveDpskLayer = allNodes[toNode]->GetObject<PfcHost> ();
      receiveDpskLayer->AddRdmaRxQueuePairSize (txQp->GetHash (), size);
      allTxQueuePairs.insert (txQp);
    }

  // TODO cyq: add trace and logs
  NS_LOG_UNCOND ("====Trace====");

  NS_LOG_UNCOND ("====Simulate====");
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND ("====Output====");
  DoTrace (conf["TraceConfigFile"]);

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
  std::vector<Ptr<Node>> bfsQueue; // Queue for the BFS
  std::map<Ptr<Node>, int> distances; // Distance from the host to each node
  std::map<Ptr<Node>, Time> delays; // Delay from the host to each node
  std::map<Ptr<Node>, Time> txDelays; // Transmit delay from the host to each node
  std::map<Ptr<Node>, DataRate> bandwidths; // Bandwidth from the host to each node
  // Init BFS
  bfsQueue.push_back (host);
  distances[host] = 0;
  delays[host] = Time (0);
  txDelays[host] = Time (0);
  bandwidths[host] = DataRate (UINT64_MAX);
  // Do BFS
  for (size_t i = 0; i < bfsQueue.size (); i++)
    {
      const auto currNode = bfsQueue[i];
      for (const auto &next : nbr2if[currNode])
        {
          const auto nextNode = next.first;
          const auto nextInterface = next.second;
          // If 'nextNode' have not been visited.
          if (distances.find (nextNode) == distances.end ())
            {
              distances[nextNode] = distances[currNode] + 1;
              delays[nextNode] = delays[currNode] + nextInterface.delay;
              txDelays[nextNode] =
                  txDelays[currNode] + nextInterface.bandwidth.CalculateBytesTxTime (CYQ_MTU);
              bandwidths[nextNode] = std::min (bandwidths[currNode], nextInterface.bandwidth);
              // Only enqueue switch because we do not want packets to go through host as middle point
              if (switchNodes.find (nextNode) != switchNodes.end ())
                bfsQueue.push_back (nextNode);
            }
          // if 'currNode' is on the shortest path from 'nextNode' to 'host'.
          if (distances[currNode] + 1 == distances[nextNode])
            {
              nextHopTable[nextNode][host].push_back (currNode);
            }
        }
    }
  for (const auto &it : delays)
    pairDelay[it.first][host] = it.second;
  for (const auto &it : txDelays)
    pairTxDelay[it.first][host] = it.second;
  for (const auto &it : bandwidths)
    pairBw[it.first][host] = it.second;
}

void
SetRoutingEntries ()
{
  // For each node
  for (const auto &nextHopEntry : nextHopTable)
    {
      const auto fromNode = nextHopEntry.first;
      const auto toNodeTable = nextHopEntry.second;
      for (const auto &toNodeEntry : toNodeTable)
        {
          // The destination node
          const auto toNode = toNodeEntry.first;
          // The next hops towards the destination
          const auto nextNodeTable = toNodeEntry.second;
          // The IP address of the destination
          Ipv4Address dstAddr = allIpv4Addresses[toNode];
          for (const auto &nextNode : nextNodeTable)
            {
              const auto &device = nbr2if[fromNode][nextNode].device;
              if (switchNodes.find (fromNode) != switchNodes.end ())
                {
                  auto dpskLayer = fromNode->GetObject<PfcSwitch> ();
                  dpskLayer->AddRouteTableEntry (dstAddr, device);
                }
              else if (hostNodes.find (fromNode) != hostNodes.end ())
                {
                  auto dpskLayer = fromNode->GetObject<PfcHost> ();
                  dpskLayer->AddRouteTableEntry (dstAddr, device);
                }
            }
        }
    }
}

void
CalculateRttBdp ()
{
  for (const auto &src : hostNodes)
    {
      for (const auto &dst : hostNodes)
        {
          if (src != dst)
            {
              const auto delay = pairDelay[src][dst];
              const auto txDelay = pairTxDelay[src][dst];
              const auto rtt = delay + txDelay + delay;
              const auto bandwidth = pairBw[src][dst];
              const uint64_t bdp = rtt.GetSeconds () * bandwidth.GetBitRate () / 8;
              pairRtt[src][dst] = rtt;
              pairBw[src][dst] = bdp;
              maxBdp = std::max (bdp, maxBdp);
              maxRtt = std::max (rtt, maxRtt);
            }
        }
    }
}

/*******************
 * Trace Functions *
 *******************/

std::string outputFolder;

void TraceFlow ();

void
DoTrace (const std::string &configFile)
{
  std::ifstream file (configFile);
  json conf = json::parse (file);

  outputFolder = conf["OutputFolder"];

  if (conf["Flow"]["Enable"] == true)
    {
      TraceFlow ();
    }
}

void
TraceFlow ()
{
  // std::string outputFileName =
  //     outputFolder + "/flow_" + cyq::Time::GetCurrTimeStr ("%Y%m%d%H%M%S") + ".log";
  // std::ofstream file (outputFileName);

  // for (const auto &host : hostNodes)
  //   {
  //     const auto dpskLayer = host->GetObject<PfcHost> ();
  //     const auto queuePairs = dpskLayer->GetRdmaRxQueuePairs ();
  //     allRxQueuePairs.insert (queuePairs.begin (), queuePairs.end ());
  //   }
  // for (const auto &txQp : allTxQueuePairs)
  //   {
  //     const auto rxQp = allRxQueuePairs[txQp->GetHash ()];
  //     file << rxQp->
  //   }
}