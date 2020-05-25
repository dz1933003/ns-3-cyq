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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("PFC Test");

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("PFC Test");

  /**
   * host 1 --100Gbps-- sw 1 --1bps-- host 2
   */

  NodeContainer host;
  host.Add (CreateObject<Node> ());
  host.Add (CreateObject<Node> ());
  auto host1 = host.Get (0); // host 1
  auto host2 = host.Get (1); // host 2

  NodeContainer sw;
  sw.Add (CreateObject<Node> ());
  auto sw1 = sw.Get (0); // sw 1

  DpskNetDeviceHelper dpskNetDeviceHelper;
  dpskNetDeviceHelper.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gbps")));
  dpskNetDeviceHelper.SetDeviceAttribute ("TxMode", EnumValue (DpskNetDevice::TxMode::ACTIVE));
  auto devResult = dpskNetDeviceHelper.Install (host1, sw1);
  auto host1_sw1_dev = DynamicCast<DpskNetDevice> (devResult.Get (0));
  auto sw1_host1_dev = DynamicCast<DpskNetDevice> (devResult.Get (1));
  auto host1_sw1_dev_impl = CreateObject<PfcHostPort> ();
  host1_sw1_dev->SetImplementation (host1_sw1_dev_impl);
  auto sw1_host1_dev_impl = CreateObject<PfcSwitchPort> ();
  sw1_host1_dev->SetImplementation (sw1_host1_dev_impl);
  dpskNetDeviceHelper.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("1bps")));
  devResult = dpskNetDeviceHelper.Install (host2, sw1);
  auto host2_sw1_dev = DynamicCast<DpskNetDevice> (devResult.Get (0));
  auto sw1_host2_dev = DynamicCast<DpskNetDevice> (devResult.Get (1));
  auto host2_sw1_dev_impl = CreateObject<PfcHostPort> ();
  host2_sw1_dev->SetImplementation (host2_sw1_dev_impl);
  auto sw1_host2_dev_impl = CreateObject<PfcSwitchPort> ();
  sw1_host2_dev->SetImplementation (sw1_host2_dev_impl);
  host1_sw1_dev_impl->SetupQueues (1);
  sw1_host1_dev_impl->SetupQueues (1);
  host2_sw1_dev_impl->SetupQueues (1);
  sw1_host2_dev_impl->SetupQueues (1);

  DpskHelper dpskHelper;

  auto sw1_dpsk = dpskHelper.Install (sw1);
  auto sw1_pfc = CreateObject<PfcSwitch> ();
  sw1_pfc->InstallDpsk (sw1_dpsk);
  sw1_pfc->SetEcmpSeed (1);
  sw1_pfc->SetNQueues (1);
  sw1_pfc->AddRouteTableEntry ("10.0.0.2", sw1_host2_dev);
  auto sw1_mmu = CreateObject<SwitchMmu> ();
  sw1_pfc->InstallMmu (sw1_mmu);
  sw1_mmu->ConfigBufferSize (12 * 1024 * 1024);
  sw1_mmu->ConfigEcn (10 * 1024 * 1024, 12 * 1024 * 1024, 1.);
  sw1_mmu->ConfigHeadroom (1024 * 1024);
  sw1_mmu->ConfigReserve (1024 * 1024);
  sw1_mmu->ConfigResumeOffset (1024 * 1024);

  auto host1_dpsk = dpskHelper.Install (host1);
  auto host1_pfc = CreateObject<PfcHost> ();
  host1_pfc->InstallDpsk (host1_dpsk);
  host1_pfc->AddRouteTableEntry ("10.0.0.2", host1_sw1_dev);
  auto qp1 =
      CreateObject<RdmaTxQueuePair> (Time::FromInteger (1, Time::Unit::S), Ipv4Address ("10.0.0.1"),
                                     Ipv4Address ("10.0.0.2"), 1, 1, 32 * 1024 * 1024, 0);
  host1_pfc->AddRdmaTxQueuePair (qp1);

  auto host2_dpsk = dpskHelper.Install (host2);
  auto host2_pfc = CreateObject<PfcHost> ();
  host2_pfc->InstallDpsk (host2_dpsk);

  Simulator::Run ();
  Simulator::Destroy ();
}
