/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
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
 * Author: Yanqing Chen  <shellqiqi@outlook.com>
 */

#include "ns3/dpsk-application.h"
#include "ns3/dpsk.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DPSKApplication");

NS_OBJECT_ENSURE_REGISTERED (DPSKApplication);

TypeId
DPSKApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DPSKApplication")
                          .SetParent<Application> ()
                          .SetGroupName ("Applications")
                          .AddConstructor<DPSKApplication> ();
  return tid;
}

DPSKApplication::DPSKApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dpsk = NULL;
}

DPSKApplication::~DPSKApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
DPSKApplication::SetupDPSK (Ptr<DPSK> dpsk)
{
  NS_LOG_FUNCTION (dpsk);
  m_dpsk = dpsk;
  dpsk->RegisterReceiveFromDeviceHandler (MakeCallback (&DPSKApplication::HandleRx, this));
}

void
DPSKApplication::HandleTx (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
DPSKApplication::HandleRx (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                           Address const &src, Address const &dst, NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (incomingPort << packet << protocol << &src << &dst << packetType);
}

void
DPSKApplication::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  HandleTx ();
}

void
DPSKApplication::StopApplication (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

} // namespace ns3
