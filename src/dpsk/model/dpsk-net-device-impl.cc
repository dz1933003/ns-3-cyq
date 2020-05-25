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

#include "dpsk-net-device-impl.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "dpsk-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DpskNetDeviceImpl");

NS_OBJECT_ENSURE_REGISTERED (DpskNetDeviceImpl);

TypeId
DpskNetDeviceImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DpskNetDeviceImpl")
                          .SetParent<Object> ()
                          .SetGroupName ("DpskNetDeviceImpl")
                          .AddConstructor<DpskNetDeviceImpl> ();
  return tid;
}

DpskNetDeviceImpl::DpskNetDeviceImpl ()
{
  NS_LOG_FUNCTION (this);
}

DpskNetDeviceImpl::~DpskNetDeviceImpl ()
{
  NS_LOG_FUNCTION (this);
}

std::string
DpskNetDeviceImpl::GetName () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_name;
}

void
DpskNetDeviceImpl::SetName (std::string name)
{
  NS_LOG_FUNCTION (name);
  m_name = name;
}

void
DpskNetDeviceImpl::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_dev = 0;
  Object::DoDispose ();
}

Ptr<Packet>
DpskNetDeviceImpl::Transmit ()
{
  // User's transmitting implementation
  return 0;
}

bool
DpskNetDeviceImpl::Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                         uint16_t protocolNumber)
{
  // User's sending implementation
  return false;
}

bool
DpskNetDeviceImpl::Receive (Ptr<Packet> p)
{
  // User's receiving post-process implementation
  return false;
}

} // namespace ns3
