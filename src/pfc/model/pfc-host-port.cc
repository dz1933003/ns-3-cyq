/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Nanjing University
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

#include "pfc-host-port.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/pfc-header.h"
#include "ns3/dpsk-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PfcHostPort");

NS_OBJECT_ENSURE_REGISTERED (PfcHostPort);

TypeId
PfcHostPort::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PfcHostPort")
                          .SetParent<Object> ()
                          .SetGroupName ("Pfc")
                          .AddConstructor<PfcHostPort> ();
  return tid;
}

PfcHostPort::PfcHostPort ()
{
  NS_LOG_FUNCTION (this);
}

PfcHostPort::~PfcHostPort ()
{
  NS_LOG_FUNCTION (this);
}

// void
// PfcHostPort::SetupQueues (uint32_t n)
// {
//   NS_LOG_FUNCTION (n);
//   CleanQueues ();
//   m_nQueues = n;
//   for (uint32_t i = 0; i <= m_nQueues; i++) // with another control queue
//     {
//       m_queues.push_back (std::queue<Ptr<Packet>> ());
//       m_pausedStates.push_back (false);
//       m_inQueueBytesList.push_back (0);
//       m_inQueuePacketsList.push_back (0);
//     }
// }

// void
// PfcHostPort::CleanQueues ()
// {
//   NS_LOG_FUNCTION_NOARGS ();
//   m_queues.clear ();
//   m_pausedStates.clear ();
//   m_inQueueBytesList.clear ();
//   m_inQueuePacketsList.clear ();
// }

void
PfcHostPort::AddRdmaQueuePair (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);
  // TODO cyq: add queue pair to the device
}

Ptr<Packet>
PfcHostPort::Transmit ()
{
  NS_LOG_FUNCTION_NOARGS ();
  // TODO cyq: transmit logic
  return 0;
}

bool
PfcHostPort::Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                   uint16_t protocolNumber)
{
  // TODO cyq: send logic
  return true;
}

bool
PfcHostPort::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION_NOARGS ();
  // TODO cyq: receive logic
  return true;
}

void
PfcHostPort::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  // TODO cyq: do dispose
  DpskNetDeviceImpl::DoDispose ();
}

} // namespace ns3
