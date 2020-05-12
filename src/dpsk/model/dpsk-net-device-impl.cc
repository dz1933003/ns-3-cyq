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

void
DpskNetDeviceImpl::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_dev = 0;
  Object::DoDispose ();
}

bool
DpskNetDeviceImpl::Attach (Ptr<DpskNetDevice> device)
{
  NS_LOG_FUNCTION (this << &device);

  m_dev = device;

  m_dev->ResetTransmitRequestHandler ();
  m_dev->ResetReceivePostProcessHandler ();

  m_dev->SetTransmitRequestHandler (MakeCallback (&DpskNetDeviceImpl::Tx, this));
  m_dev->SetReceivePostProcessHandler (MakeCallback (&DpskNetDeviceImpl::Rx, this));

  return true;
}

void
DpskNetDeviceImpl::Detach ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dev->ResetTransmitRequestHandler ();
  m_dev->ResetReceivePostProcessHandler ();
  m_dev = 0;
}

Ptr<Packet>
DpskNetDeviceImpl::Tx ()
{
  // User's transmitting implementation
  return 0;
}

void
DpskNetDeviceImpl::Rx (Ptr<Packet> p)
{
  // User's receiving post-process implementation
}

} // namespace ns3
