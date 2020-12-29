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
#include <chrono>

#include <boost/bimap.hpp>

#include "json.hpp"
#include "csv.hpp"
#include "cyq-utils.hpp"

using namespace ns3;
using json = nlohmann::json;

/*******************************
 * Global simulation variables *
 *******************************/

static const uint32_t CYQ_MTU = 1500;
static DpskHelper dpskHelper;
static std::string traceTag = "";

uint32_t nQueue;
uint32_t ecmpSeed;
DpskNetDevice::TxMode portTxMode;
boost::bimap<std::string, Ptr<Node>> allNodes;
std::map<Ptr<Node>, std::vector<Ptr<DpskNetDevice>>> allPorts;
boost::bimap<Ptr<Node>, Ipv4Address> allIpv4Addresses;
std::set<Ptr<Node>> hostNodes;
std::set<Ptr<Node>> switchNodes;
std::map<uint32_t, Ptr<RdmaTxQueuePair>> allTxQueuePairs;
std::map<uint32_t, Ptr<RdmaRxQueuePair>> allRxQueuePairs;

uint32_t txCompleteCnt = 0;
uint32_t rxCompleteCnt = 0;

uint64_t maxBdp = 0;
Time maxRtt (0);

std::string outputFolder; // Log output folder

/*******************************
 * Route calculation variables *
 *******************************/

struct Interface
{
  Ptr<DpskNetDevice> device;
  Time delay;
  DataRate bandwidth;
};
std::map<Ptr<Node>, std::map<Ptr<Node>, Interface>> onewayOutDev;
std::map<Ptr<Node>, std::map<Ptr<Node>, std::vector<Ptr<Node>>>> nextHopTable;
std::map<Ptr<Node>, std::map<Ptr<Node>, Time>> pairDelay;
std::map<Ptr<Node>, std::map<Ptr<Node>, Time>> pairTxDelay;
std::map<Ptr<Node>, std::map<Ptr<Node>, DataRate>> pairBandwidth;
std::map<Ptr<Node>, std::map<Ptr<Node>, uint64_t>> pairBdp; // byte
std::map<Ptr<Node>, std::map<Ptr<Node>, Time>> pairRtt;

/***************************************
 * Help functions for simulation setup *
 ***************************************/

void ConfigMmuPort (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile);
void ConfigMmuQueue (Ptr<Node> node, Ptr<SwitchMmu> mmu, Ptr<NetDevice> port,
                     const std::string &configFile);

void CalculateRoute ();
void CalculateRoute (Ptr<Node> host);
void SetRoutingEntries ();
void CalculateRttBdp ();

void DoTrace (const std::string &configFile);
void DoLog ();

/********************
 * Simulation setup *
 ********************/

NS_LOG_COMPONENT_DEFINE ("PFC CYQ");

int
main (int argc, char *argv[])
{
  if (argc < 2)
    {
      NS_LOG_UNCOND ("ERROR: No config file");
      return 1;
    }

  std::ifstream file (argv[1]);
  if (argc == 3)
    traceTag = argv[2];

  json conf = json::parse (file);

  const auto sim_start = std::chrono::system_clock::now ();

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
          allNodes.insert ({name, node});
          allIpv4Addresses.insert (
              {node, Ipv4AddressGenerator::NextAddress (Ipv4Mask ("255.0.0.0"))});
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
              impl->EnablePfc (host.contains ("PfcEnable") ? host["PfcEnable"].get<bool> () : true);
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
          allNodes.insert ({name, node});
          // Install ports
          for (const auto &swPort : sw["Port"])
            {
              const Ptr<DpskNetDevice> dev = CreateObject<DpskNetDevice> ();
              dev->SetAddress (Mac48Address::Allocate ());
              dev->SetTxMode (portTxMode);
              node->AddDevice (dev);
              std::string portType = swPort["Type"];
              if (portType == "PFC")
                {
                  const Ptr<PfcSwitchPort> impl = CreateObject<PfcSwitchPort> ();
                  dev->SetImplementation (impl);
                  impl->SetupQueues (nQueue);
                  if (swPort.contains ("PassThrough"))
                    {
                      impl->SetPassThrough (swPort["PassThrough"]);
                    }
                }
              else if (portType == "CBFC")
                {
                  const Ptr<CbfcSwitchPort> impl = CreateObject<CbfcSwitchPort> ();
                  dev->SetImplementation (impl);
                  impl->SetupQueues (nQueue);
                }
              else if (portType == "CBPFC")
                {
                  const Ptr<CbpfcSwitchPort> impl = CreateObject<CbpfcSwitchPort> ();
                  dev->SetImplementation (impl);
                  impl->SetupQueues (nQueue);
                }
              else if (portType == "PTPFC")
                {
                  const Ptr<PtpfcSwitchPort> impl = CreateObject<PtpfcSwitchPort> ();
                  dev->SetImplementation (impl);
                  impl->SetupQueues (nQueue);
                }
              else if (portType == "NOPFC")
                {
                  const Ptr<NoPfcSwitchPort> impl = CreateObject<NoPfcSwitchPort> ();
                  dev->SetImplementation (impl);
                  impl->SetupQueues (nQueue);
                }
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
          pfcSwitch->InitSendCbfcFeedback ();
          pfcSwitch->InitSendCbpfcFeedback ();
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
      const auto s_node = allNodes.left.at (fromNode);
      const auto d_node = allNodes.left.at (toNode);
      const auto s_dev = allPorts[s_node][fromPort];
      const auto d_dev = allPorts[d_node][toPort];
      const Ptr<DpskChannel> channel = CreateObject<DpskChannel> ();
      s_dev->SetAttribute ("DataRate", DataRateValue (dataRate));
      d_dev->SetAttribute ("DataRate", DataRateValue (dataRate));
      channel->SetAttribute ("Delay", TimeValue (delay));
      s_dev->Attach (channel);
      d_dev->Attach (channel);
      onewayOutDev[s_node][d_node] = {.device = s_dev, .delay = delay, .bandwidth = dataRate};
      onewayOutDev[d_node][s_node] = {.device = d_dev, .delay = delay, .bandwidth = dataRate};
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
      const auto sourceIp = allIpv4Addresses.left.at (allNodes.left.at (fromNode));
      const auto destinationIp = allIpv4Addresses.left.at (allNodes.left.at (toNode));
      const uint64_t size = cyq::DataSize::GetBytes (sizeInput);
      auto txQp = CreateObject<RdmaTxQueuePair> (startTime, sourceIp, destinationIp, sourcePort,
                                                 destinationPort, size, priority);
      auto sendDpskLayer = allNodes.left.at (fromNode)->GetObject<PfcHost> ();
      sendDpskLayer->AddRdmaTxQueuePair (txQp);
      auto receiveDpskLayer = allNodes.left.at (toNode)->GetObject<PfcHost> ();
      receiveDpskLayer->AddRdmaRxQueuePairSize (txQp->GetHash (), size);
      allTxQueuePairs[txQp->GetHash ()] = txQp;
    }

  NS_LOG_UNCOND ("====Trace====");
  outputFolder = conf["LogOutputFolder"];
  DoTrace (conf["TraceConfigFile"]);

  NS_LOG_UNCOND ("====Simulate====");
  Simulator::Run ();
  Simulator::Destroy ();

  NS_LOG_UNCOND ("====Output====");
  DoLog ();

  NS_LOG_UNCOND ("====Done====");
  const auto sim_end = std::chrono::system_clock::now ();
  const std::chrono::duration<double> elapsed_seconds = sim_end - sim_start;
  NS_LOG_UNCOND ("Elapsed " << elapsed_seconds.count () << "s");
}

/*********************
 * MMU configuration *
 *********************/

void
ConfigMmuPort (Ptr<Node> node, Ptr<SwitchMmu> mmu, const std::string &configFile)
{
  std::ifstream file (configFile);
  json conf = json::parse (file);
  for (const auto &port : conf)
    {
      for (const auto &index : port["PortIndex"])
        {
          ConfigMmuQueue (node, mmu, allPorts[node][index], port["QueueConfigFile"]);
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
      for (const auto &index : queue["QueueIndex"])
        {
          const auto portType = PfcSwitch::DeviceToL2Type (port);
          if (portType == PfcSwitch::PFC)
            {
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
          else if (portType == PfcSwitch::CBFC)
            {
              if (queue.contains ("Ingress"))
                {
                  const uint64_t ingress = cyq::DataSize::GetBytes (queue["Ingress"]);
                  mmu->ConfigCbfcBufferSize (port, index, ingress);
                }
              if (queue.contains ("Period"))
                {
                  const Time period (queue["Period"].get<std::string> ());
                  mmu->ConfigCbfcFeedbackPeroid (port, index, period);
                }
            }
          else if (portType == PfcSwitch::CBPFC)
            {
              if (queue.contains ("Ingress"))
                {
                  const uint64_t ingress = cyq::DataSize::GetBytes (queue["Ingress"]);
                  mmu->ConfigCbpfcBufferSize (port, index, ingress);
                }
              if (queue.contains ("Period"))
                {
                  const Time period (queue["Period"].get<std::string> ());
                  mmu->ConfigCbpfcFeedbackPeroid (port, index, period);
                }
            }
          else if (portType == PfcSwitch::PTPFC)
            {
              if (queue.contains ("Ingress"))
                {
                  const uint64_t ingress = cyq::DataSize::GetBytes (queue["Ingress"]);
                  mmu->ConfigPtpfcBufferSize (port, index, ingress);
                }
            }
          else if (portType == PfcSwitch::NOPFC)
            {
              if (queue.contains ("Ingress"))
                {
                  const uint64_t ingress = cyq::DataSize::GetBytes (queue["Ingress"]);
                  mmu->ConfigNoPfcBufferSize (port, index, ingress);
                }
            }
          if (queue.contains ("Ecn"))
            {
              const uint64_t kMin = cyq::DataSize::GetBytes (queue["Ecn"]["kMin"]);
              const uint64_t kMax = cyq::DataSize::GetBytes (queue["Ecn"]["kMax"]);
              const double pMax = queue["Ecn"]["pMax"];
              mmu->ConfigEcn (port, index, kMin, kMax, pMax);
            }
        }
    }
}

/*********************
 * Route calculation *
 *********************/

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
      for (const auto &next : onewayOutDev[currNode])
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
    pairBandwidth[it.first][host] = it.second;
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
          Ipv4Address dstAddr = allIpv4Addresses.left.at (toNode);
          for (const auto &nextNode : nextNodeTable)
            {
              const auto &device = onewayOutDev[fromNode][nextNode].device;
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
              const auto bandwidth = pairBandwidth[src][dst];
              const uint64_t bdp = rtt.GetSeconds () * bandwidth.GetBitRate () / 8;
              pairRtt[src][dst] = rtt;
              pairBandwidth[src][dst] = bdp;
              maxBdp = std::max (bdp, maxBdp);
              maxRtt = std::max (rtt, maxRtt);
            }
        }
    }
}

/*******************
 * Trace Functions *
 *******************/

std::map<std::string, std::stringstream> logStreams;

void TraceFct ();
void TraceQueuePairTxComplete (Ptr<RdmaTxQueuePair> qp);
void TraceQueuePairRxComplete (Ptr<RdmaRxQueuePair> qp);

void CheckQueuePair ();

void TraceSwitch (const json &conf);
void TraceIngressDropPacket (Time interval, Time end);
void TraceBufferUsed (Time interval, Time end, std::string name, uint32_t portIndex);

void TraceTxByte (Time interval, Time end, std::string name, uint32_t portIndex);
void TraceRxByte (Time interval, Time end, std::string name, uint32_t portIndex);

void TracePfcRx (Ptr<DpskNetDevice> dev, uint32_t qIndex, PfcHeader::PfcType type, uint16_t time);
void TraceCbfcRx (Ptr<DpskNetDevice> dev, uint32_t qIndex, uint64_t fccl);

void
DoTrace (const std::string &configFile)
{
  std::ifstream file (configFile);
  json conf = json::parse (file);

  if (conf["Fct"]["Enable"] == true)
    {
      TraceFct ();
    }
  if (conf["Switch"]["Enable"] == true)
    {
      TraceSwitch (conf["Switch"]);
    }
  if (conf["TxByte"]["Enable"] == true)
    {
      logStreams["TxByte"] << "Time,Node,PortIndex,TxByte\n";
      const auto interval = Time (conf["TxByte"]["Interval"].get<std::string> ());
      const auto start = Time (conf["TxByte"]["Start"].get<std::string> ());
      const auto end = Time (conf["TxByte"]["End"].get<std::string> ());
      for (const auto &target : conf["TxByte"]["Target"])
        {
          for (const auto &name : target["Name"])
            {
              for (const auto &portIndex : target["PortIndex"])
                {
                  Simulator::Schedule (start, &TraceTxByte, interval, end, name, portIndex);
                }
            }
        }
    }
  if (conf["RxByte"]["Enable"] == true)
    {
      logStreams["RxByte"] << "Time,Node,PortIndex,RxByte\n";
      const auto interval = Time (conf["RxByte"]["Interval"].get<std::string> ());
      const auto start = Time (conf["RxByte"]["Start"].get<std::string> ());
      const auto end = Time (conf["RxByte"]["End"].get<std::string> ());
      for (const auto &target : conf["RxByte"]["Target"])
        {
          for (const auto &name : target["Name"])
            {
              for (const auto &portIndex : target["PortIndex"])
                {
                  Simulator::Schedule (start, &TraceRxByte, interval, end, name, portIndex);
                }
            }
        }
    }
  if (conf.contains ("PfcRx") && conf["PfcRx"]["Enable"] == true)
    {
      logStreams["PfcRx"] << "Time,Node,IfIndex,qIndex,PfcType,Time\n";
      for (const auto &target : conf["PfcRx"]["Target"])
        {
          for (const auto &name : target["Name"])
            {
              for (const auto &portIndex : target["PortIndex"])
                {
                  const auto node = allNodes.left.at (name);
                  const auto dev = allPorts[node][portIndex];
                  const auto impl = dev->GetImplementation ();
                  impl->TraceConnectWithoutContext ("PfcRx", MakeCallback (&TracePfcRx));
                }
            }
        }
    }
  if (conf.contains ("CbfcRx") && conf["CbfcRx"]["Enable"] == true)
    {
      logStreams["CbfcRx"] << "Time,Node,IfIndex,qIndex,Fccl\n";
      for (const auto &target : conf["CbfcRx"]["Target"])
        {
          for (const auto &name : target["Name"])
            {
              for (const auto &portIndex : target["PortIndex"])
                {
                  const auto node = allNodes.left.at (name);
                  const auto dev = allPorts[node][portIndex];
                  const auto impl = dev->GetImplementation ();
                  impl->TraceConnectWithoutContext ("CbfcRx", MakeCallback (&TraceCbfcRx));
                }
            }
        }
    }
}

void
TraceFct ()
{
  logStreams["QueuePairRxComplete"]
      << "FromNode,ToNode,SourcePort,DestinationPort,Size,Priority,StartTime,EndTime,Duration\n";
  for (const auto &host : hostNodes)
    {
      const auto devs = allPorts[host];
      for (const auto &dev : devs)
        {
          const auto impl = dev->GetObject<PfcHostPort> ();
          impl->TraceConnectWithoutContext ("QueuePairTxComplete",
                                            MakeCallback (&TraceQueuePairTxComplete));
          impl->TraceConnectWithoutContext ("QueuePairRxComplete",
                                            MakeCallback (&TraceQueuePairRxComplete));
        }
    }
}

void
TraceQueuePairTxComplete (Ptr<RdmaTxQueuePair> qp)
{
  txCompleteCnt++;
  CheckQueuePair ();
}

void
TraceQueuePairRxComplete (Ptr<RdmaRxQueuePair> qp)
{
  const auto fromNode = allNodes.right.at (allIpv4Addresses.right.at (qp->m_sIp));
  const auto toNode = allNodes.right.at (allIpv4Addresses.right.at (qp->m_dIp));
  const auto startTime = allTxQueuePairs[qp->GetHash ()]->m_startTime;
  const auto endTime = Simulator::Now ();
  const auto duration = endTime - startTime;
  logStreams["QueuePairRxComplete"]
      << fromNode << "," << toNode << "," << qp->m_sPort << "," << qp->m_dPort << "," << qp->m_size
      << "," << qp->m_priority << "," << startTime << "," << endTime << "," << duration << "\n";
  rxCompleteCnt++;
  CheckQueuePair ();
}

void
CheckQueuePair ()
{
  const auto allQPCnt = allTxQueuePairs.size ();
  std::clog << "\rTx: " << txCompleteCnt << "/" << allQPCnt << " Rx: " << rxCompleteCnt << "/"
            << allQPCnt;
  if (txCompleteCnt == allQPCnt && rxCompleteCnt == allQPCnt)
    {
      NS_LOG_UNCOND ("\nComplete Simulation: " << Simulator::Now ());
      Simulator::Stop ();
    }
}

void
TraceSwitch (const json &conf)
{
  if (conf["IngressDropPacket"]["Enable"] == true)
    {
      logStreams["IngressDropPacket"] << "Time,Node,PortIndex,DropPacket\n";
      const auto interval = Time (conf["IngressDropPacket"]["Interval"].get<std::string> ());
      const auto start = Time (conf["IngressDropPacket"]["Start"].get<std::string> ());
      const auto end = Time (conf["IngressDropPacket"]["End"].get<std::string> ());
      Simulator::Schedule (start, &TraceIngressDropPacket, interval, end);
    }
  if (conf["BufferUsed"]["Enable"] == true)
    {
      logStreams["BufferUsed"] << "Time,Node,PortIndex,Used\n";
      const auto interval = Time (conf["BufferUsed"]["Interval"].get<std::string> ());
      const auto start = Time (conf["BufferUsed"]["Start"].get<std::string> ());
      const auto end = Time (conf["BufferUsed"]["End"].get<std::string> ());
      for (const auto &target : conf["BufferUsed"]["Target"])
        {
          for (const auto &name : target["Name"])
            {
              for (const auto &portIndex : target["PortIndex"])
                {
                  Simulator::Schedule (start, &TraceBufferUsed, interval, end, name, portIndex);
                }
            }
        }
    }
}

void
TraceIngressDropPacket (Time interval, Time end)
{
  for (const auto &sw : switchNodes)
    {
      const auto dpskLayer = sw->GetObject<PfcSwitch> ();
      const auto dropOfPorts = dpskLayer->m_nIngressDropPacket;
      for (const auto dropOfPortEntry : dropOfPorts)
        {
          const auto dev = dropOfPortEntry.first;
          const auto drop = dropOfPortEntry.second;
          logStreams["IngressDropPacket"] << Simulator::Now () << ","
                                          << allNodes.right.at (dev->GetNode ()) << ","
                                          << dev->GetIfIndex () << "," << drop << "\n";
        }
    }
  if (Simulator::Now () < end)
    Simulator::Schedule (interval, &TraceIngressDropPacket, interval, end);
}

void
TraceBufferUsed (Time interval, Time end, std::string name, uint32_t portIndex)
{
  const auto node = allNodes.left.at (name);
  const auto port = allPorts[node][portIndex];
  uint64_t bufferUsed = 0;
  if (hostNodes.find (node) != hostNodes.end ())
    return;
  else if (switchNodes.find (node) != switchNodes.end ())
    bufferUsed = node->GetObject<PfcSwitch> ()->GetObject<SwitchMmu> ()->GetBufferUsed (port);
  logStreams["BufferUsed"] << Simulator::Now () << "," << name << "," << portIndex << ","
                           << bufferUsed << "\n";
  if (Simulator::Now () < end)
    Simulator::Schedule (interval, &TraceBufferUsed, interval, end, name, portIndex);
}

void
TraceTxByte (Time interval, Time end, std::string name, uint32_t portIndex)
{
  const auto node = allNodes.left.at (name);
  const auto port = allPorts[node][portIndex];
  uint64_t txByte = 0;
  if (hostNodes.find (node) != hostNodes.end ())
    {
      txByte = port->GetObject<PfcHostPort> ()->m_nTxBytes;
    }
  else if (switchNodes.find (node) != switchNodes.end ())
    {
      const auto portType = PfcSwitch::DeviceToL2Type (port);
      if (portType == PfcSwitch::PFC)
        {
          txByte = port->GetObject<PfcSwitchPort> ()->m_nTxBytes;
        }
      else if (portType == PfcSwitch::CBFC)
        {
          txByte = port->GetObject<CbfcSwitchPort> ()->m_nTxBytes;
        }
      else if (portType == PfcSwitch::CBPFC)
        {
          txByte = port->GetObject<CbpfcSwitchPort> ()->m_nTxBytes;
        }
      else if (portType == PfcSwitch::PTPFC)
        {
          txByte = port->GetObject<PtpfcSwitchPort> ()->m_nTxBytes;
        }
      else if (portType == PfcSwitch::NOPFC)
        {
          txByte = port->GetObject<NoPfcSwitchPort> ()->m_nTxBytes;
        }
    }
  logStreams["TxByte"] << Simulator::Now () << "," << name << "," << portIndex << "," << txByte
                       << "\n";
  if (Simulator::Now () < end)
    Simulator::Schedule (interval, &TraceTxByte, interval, end, name, portIndex);
}

void
TraceRxByte (Time interval, Time end, std::string name, uint32_t portIndex)
{
  const auto node = allNodes.left.at (name);
  const auto port = allPorts[node][portIndex];
  uint64_t rxByte = 0;
  if (hostNodes.find (node) != hostNodes.end ())
    {
      rxByte = port->GetObject<PfcHostPort> ()->m_nRxBytes;
    }
  else if (switchNodes.find (node) != switchNodes.end ())
    {
      const auto portType = PfcSwitch::DeviceToL2Type (port);
      if (portType == PfcSwitch::PFC)
        {
          rxByte = port->GetObject<PfcSwitchPort> ()->m_nRxBytes;
        }
      else if (portType == PfcSwitch::CBFC)
        {
          rxByte = port->GetObject<CbfcSwitchPort> ()->m_nRxBytes;
        }
      else if (portType == PfcSwitch::CBPFC)
        {
          rxByte = port->GetObject<CbpfcSwitchPort> ()->m_nRxBytes;
        }
      else if (portType == PfcSwitch::PTPFC)
        {
          rxByte = port->GetObject<PtpfcSwitchPort> ()->m_nRxBytes;
        }
      else if (portType == PfcSwitch::NOPFC)
        {
          rxByte = port->GetObject<NoPfcSwitchPort> ()->m_nRxBytes;
        }
    }
  logStreams["RxByte"] << Simulator::Now () << "," << name << "," << portIndex << "," << rxByte
                       << "\n";
  if (Simulator::Now () < end)
    Simulator::Schedule (interval, &TraceRxByte, interval, end, name, portIndex);
}

void
TracePfcRx (Ptr<DpskNetDevice> dev, uint32_t qIndex, PfcHeader::PfcType type, uint16_t time)
{
  const std::string pfcType = (type == PfcHeader::PfcType::Pause) ? "P" : "R";
  logStreams["PfcRx"] << Simulator::Now () << "," << allNodes.right.at (dev->GetNode ()) << ","
                      << dev->GetIfIndex () << "," << qIndex << "," << pfcType << "," << time
                      << "\n";
}

void
TraceCbfcRx (Ptr<DpskNetDevice> dev, uint32_t qIndex, uint64_t fccl)
{
  logStreams["CbfcRx"] << Simulator::Now () << "," << allNodes.right.at (dev->GetNode ()) << ","
                       << dev->GetIfIndex () << "," << qIndex << "," << fccl << "\n";
}

/*****************
 * Log Functions *
 *****************/

void
DoLog ()
{
  for (const auto &streamItem : logStreams)
    {
      const auto &name = streamItem.first;
      const auto &ss = streamItem.second;
      const std::string filePath = outputFolder + "/" +
                                   ((traceTag.empty ()) ? "" : (traceTag + "_")) +
                                   cyq::Time::GetCurrTimeStr ("%Y%m%d%H%M%S") + "_" + name + ".csv";
      std::ofstream file (filePath);
      file << ss.str ();
      file.close ();
    }
}