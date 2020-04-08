/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 Nanjing University
 *
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
 * 
 * Author: Chen Yanqing
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/applications-module.h"

#define CYQ_TOS

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TosExample2Apps");

static uint64_t enqueue_0 = 0;
static void
TraceQueueDisc_0 (Ptr<const QueueDiscItem> item)
{
  enqueue_0++;
}

static uint64_t enqueue_1 = 0;
static void
TraceQueueDisc_1 (Ptr<const QueueDiscItem> item)
{
  enqueue_1++;
}

template <class T>
static void
GoodputSampling (T app_1, T app_2, Ptr<OutputStreamWrapper> stream, uint64_t lastRx_1,
                 uint64_t lastRx_2, double lastTime)
{
  uint64_t totalRx_1 = app_1->GetTotalRx ();
  uint64_t diffRx_1 = totalRx_1 - lastRx_1;
  lastRx_1 = totalRx_1;

  uint64_t totalRx_2 = app_2->GetTotalRx ();
  uint64_t diffRx_2 = totalRx_2 - lastRx_2;
  lastRx_2 = totalRx_2;

  double thisTime = Simulator::Now ().GetSeconds ();
  double diffTime = thisTime - lastTime;
  lastTime = thisTime;

  double goodput_1 = diffRx_1 * 8 / diffTime / 1000; // Kbps
  double goodput_2 = diffRx_2 * 8 / diffTime / 1000; // Kbps

  *stream->GetStream () << "Time: " << thisTime << " Rx[1,2]: " << goodput_1 << ',' << goodput_2
                        << " Kbps Enqueue[0,1]: " << enqueue_0 << ',' << enqueue_1 << std::endl;

  Simulator::Schedule (Seconds (diffTime), GoodputSampling<T>, app_1, app_2, stream, lastRx_1,
                       lastRx_2, lastTime);
}

int
main (int argc, char *argv[])
{
  bool tracing = false;

  // Allow the user to override any of the defaults at
  // run-time, via command-line arguments
  CommandLine cmd;
  cmd.AddValue ("tracing", "Flag to enable/disable tracing", tracing);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2us"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  /* Seting up QueueDisc on NetDevice */

  TrafficControlHelper tch;
  // Tos maps to cid     0 1 2 3 4 5 6 7 ...
  std::string prioMap = "1 0 1 2 0 0 0 0 0 0 0 0 0 0 0 0";
  uint16_t tchHandle =
      tch.SetRootQueueDisc ("ns3::PrioQueueDisc", "Priomap", StringValue (prioMap));
  TrafficControlHelper::ClassIdList cid =
      tch.AddQueueDiscClasses (tchHandle, 3, "ns3::QueueDiscClass");
  /* cid
   * High  Low
   * 0 ... N
   */
  tch.AddChildQueueDisc (tchHandle, cid[0], "ns3::FifoQueueDisc");
  tch.AddChildQueueDisc (tchHandle, cid[1], "ns3::FifoQueueDisc");
  tch.AddChildQueueDisc (tchHandle, cid[2], "ns3::TbfQueueDisc", "Rate",
                         DataRateValue (DataRate ("1Mbps")));

  QueueDiscContainer qdiscs = tch.Install (devices);

  // Call TraceQueueDisc when Enqueue changed in Node 0 Queue 0
  qdiscs.Get (0)->GetQueueDiscClass (0)->GetQueueDisc ()->TraceConnectWithoutContext (
      "Enqueue", MakeCallback (&TraceQueueDisc_0));
  qdiscs.Get (0)->GetQueueDiscClass (1)->GetQueueDisc ()->TraceConnectWithoutContext (
      "Enqueue", MakeCallback (&TraceQueueDisc_1));
  /* End */

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  InetSocketAddress sendAddress_1 = InetSocketAddress (interfaces.GetAddress (1), 1);
  InetSocketAddress sendAddress_2 = InetSocketAddress (interfaces.GetAddress (1), 2);
  InetSocketAddress recvAddress_1 = InetSocketAddress (Ipv4Address::GetAny (), 1);
  InetSocketAddress recvAddress_2 = InetSocketAddress (Ipv4Address::GetAny (), 2);

  // Set tos on Inet socket address
  sendAddress_1.SetTos (0x20);
  sendAddress_2.SetTos (0x10);

  OnOffHelper sendOnOffHelper_1 ("ns3::UdpSocketFactory", sendAddress_1);
  sendOnOffHelper_1.SetAttribute ("DataRate", StringValue ("10Mbps"));
  sendOnOffHelper_1.SetAttribute ("OnTime",
                                  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  sendOnOffHelper_1.SetAttribute ("OffTime",
                                  StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer sendOnOff_1 = sendOnOffHelper_1.Install (nodes.Get (0));

  OnOffHelper sendOnOffHelper_2 ("ns3::UdpSocketFactory", sendAddress_2);
  sendOnOffHelper_2.SetAttribute ("DataRate", StringValue ("10Mbps"));
  sendOnOffHelper_2.SetAttribute ("OnTime",
                                  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  sendOnOffHelper_2.SetAttribute ("OffTime",
                                  StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  ApplicationContainer sendOnOff_2 = sendOnOffHelper_2.Install (nodes.Get (0));

  PacketSinkHelper recvSinkHelper_1 ("ns3::UdpSocketFactory", recvAddress_1);
  ApplicationContainer recvSink_1 = recvSinkHelper_1.Install (nodes.Get (1));

  PacketSinkHelper recvSinkHelper_2 ("ns3::UdpSocketFactory", recvAddress_2);
  ApplicationContainer recvSink_2 = recvSinkHelper_2.Install (nodes.Get (1));

  float stopTime = 1;

  sendOnOff_1.Start (Seconds (0));
  sendOnOff_1.Stop (Seconds (stopTime));
  recvSink_1.Start (Seconds (0));
  recvSink_1.Stop (Seconds (stopTime));

  sendOnOff_2.Start (Seconds (0));
  sendOnOff_2.Stop (Seconds (stopTime));
  recvSink_2.Start (Seconds (0));
  recvSink_2.Stop (Seconds (stopTime));

  float samplingPeriod = 0.1;

  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> uploadGoodputStream = ascii.CreateFileStream ("prio-upGoodput2Apps.log");
  Ptr<PacketSink> recvApp_1 = DynamicCast<PacketSink> (recvSink_1.Get (0));
  Ptr<PacketSink> recvApp_2 = DynamicCast<PacketSink> (recvSink_2.Get (0));
  Simulator::Schedule (Seconds (samplingPeriod), GoodputSampling<Ptr<PacketSink>>, recvApp_1,
                       recvApp_2, uploadGoodputStream, 0, 0, 0);

  if (tracing)
    pointToPoint.EnablePcapAll ("prio-tos-2Apps");

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  ns3::PacketMetadata::Enable ();

  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
