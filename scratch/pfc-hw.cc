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

static DpskHelper dpskHelper;

uint32_t nQueue;
uint32_t ecmpSeed;
DpskNetDevice::TxMode portTxMode;
std::map<std::string, Ptr<Node>> allNodes;
std::map<Ptr<Node>, std::vector<Ptr<DpskNetDevice>>> allPorts;
std::set<Ptr<Node>> hostNodes;
std::set<Ptr<Node>> switchNodes;

struct Interface
{
  Ptr<DpskNetDevice> device;
  std::string delay;
  std::string bandwidth;
};
std::map<Ptr<Node>, std::map<Ptr<Node>, Interface>> nbr2if;
std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>>>> nextHop;
std::map<Ptr<Node>, std::map<Ptr<Node>, uint64_t>> pairDelay;
std::map<Ptr<Node>, std::map<Ptr<Node>, uint64_t>> pairTxDelay;
std::map<Ptr<Node>, std::map<Ptr<Node>, uint64_t>> pairBw;
std::map<Ptr<Node>, std::map<Ptr<Node>, uint64_t>> pairBdp;
std::map<Ptr<Node>, std::map<Ptr<Node>, uint64_t>> pairRtt;

void ConfigMmuPort (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile);
void ConfigMmuQueue (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile);
void ConfigMmuQueue (Ptr<Node> node, Ptr<SwitchMmu> mmu, Ptr<NetDevice> port,
                     const std::string &configFile);

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
          pfcHost->InstallDpsk (dpsk);
          // TODO cyq: add route
          // TODO cyq: add flows
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
          // TODO cyq: add route
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
      std::string dataRate;
      std::string delay;
      if (!linkConfig.read_row (fromNode, fromPort, toNode, toPort, dataRate, delay))
        break;
      const auto s_node = allNodes[fromNode];
      const auto d_node = allNodes[toNode];
      const auto s_dev = allPorts[s_node][fromPort];
      const auto d_dev = allPorts[d_node][toPort];
      const Ptr<DpskChannel> channel = CreateObject<DpskChannel> ();
      s_dev->SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
      d_dev->SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
      channel->SetAttribute ("Delay", TimeValue (Time (delay)));
      s_dev->Attach (channel);
      d_dev->Attach (channel);
      nbr2if[s_node][d_node] = {.device = d_dev, .delay = delay, .bandwidth = dataRate};
      nbr2if[d_node][s_node] = {.device = s_dev, .delay = delay, .bandwidth = dataRate};
    }

  // TODO cyq: complete read configuration

  Simulator::Run ();
  Simulator::Destroy ();
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
