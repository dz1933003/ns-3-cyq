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
 *
 * Author: Yanqing Chen  <shellqiqi@outlook.com>
 */

#include "dpsk-layer.h"
#include "ns3/packet.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DpskLayer");

NS_OBJECT_ENSURE_REGISTERED (DpskLayer);

TypeId
DpskLayer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DpskLayer")
                          .SetParent<Object> ()
                          .SetGroupName ("Dpsk")
                          .AddConstructor<DpskLayer> ();
  return tid;
}

DpskLayer::DpskLayer ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dpsk = NULL;
}

DpskLayer::~DpskLayer ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
DpskLayer::SendFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                           const Address &source, const Address &destination)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination);
  return true;
}

void
DpskLayer::ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                              const Address &source, const Address &destination,
                              NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination << packetType);
}

void
DpskLayer::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  for (DpskLayerList::iterator iter = m_layers.begin (); iter != m_layers.end (); iter++)
    {
      *iter = NULL;
    }
  m_dpsk = NULL;
  Object::DoDispose ();
}

} // namespace ns3
