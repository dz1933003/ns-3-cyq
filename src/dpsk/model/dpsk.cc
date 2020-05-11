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

#include "dpsk.h"
#include "ns3/node.h"
#include "ns3/channel.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Dpsk");

NS_OBJECT_ENSURE_REGISTERED (Dpsk);

TypeId
Dpsk::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::Dpsk")
          .SetParent<NetDevice> ()
          .SetGroupName ("Dpsk")
          .AddConstructor<Dpsk> ()
          .AddAttribute ("Address", "The MAC address of this device (Not used)",
                         Mac48AddressValue (Mac48Address ()),
                         MakeMac48AddressAccessor (&Dpsk::m_address), MakeMac48AddressChecker ())
          .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit (Not used)",
                         UintegerValue (1500), MakeUintegerAccessor (&Dpsk::SetMtu, &Dpsk::GetMtu),
                         MakeUintegerChecker<uint16_t> ());
  return tid;
}

Dpsk::Dpsk () : m_node (0), m_ifIndex (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

Dpsk::~Dpsk ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
Dpsk::AddDevice (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (device);
  NS_ASSERT (device != this);

  if (!Mac48Address::IsMatchingType (device->GetAddress ()))
    {
      NS_FATAL_ERROR ("Device does not support eui 48 addresses: cannot be added to Dpsk.");
    }

  if (!device->SupportsSendFrom ())
    {
      NS_FATAL_ERROR ("Device does not support SendFrom: cannot be added to Dpsk.");
    }

  NS_LOG_DEBUG ("RegisterProtocolHandler for " << device->GetInstanceTypeId ().GetName ());
  m_node->RegisterProtocolHandler (MakeCallback (&Dpsk::ReceiveFromDevice, this), 0, device, true);

  m_ports.push_back (device);
}

uint32_t
Dpsk::GetNDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports.size ();
}

Ptr<NetDevice>
Dpsk::GetDevice (uint32_t n) const
{
  NS_LOG_FUNCTION (n);
  return m_ports[n];
}

std::vector<Ptr<NetDevice>>
Dpsk::GetDevices (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ports;
}

bool
Dpsk::SendFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                      Address const &source, Address const &destination)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination);

  if (device == NULL)
    {
      Ptr<Packet> pktCopy;
      for (std::vector<Ptr<NetDevice>>::iterator iter = m_ports.begin (); iter != m_ports.end ();
           iter++)
        {
          pktCopy = packet->Copy ();
          Ptr<NetDevice> port = *iter;
          port->SendFrom (pktCopy, source, destination, protocol);
        }
    }
  else
    {
      device->SendFrom (packet->Copy (), source, destination, protocol);
    }

  return true;
}

void
Dpsk::RegisterReceiveFromDeviceHandler (ReceiveFromDeviceHandler handler)
{
  NS_LOG_FUNCTION (&handler);
  m_handlers.push_back (handler);
}

void
Dpsk::UnregisterReceiveFromDeviceHandler (ReceiveFromDeviceHandler handler)
{
  NS_LOG_FUNCTION (&handler);

  for (ReceiveFromDeviceHandlerList::iterator iter = m_handlers.begin (); iter != m_handlers.end ();
       iter++)
    {
      if (iter->IsEqual (handler))
        {
          m_handlers.erase (iter);
          break;
        }
    }
}

void
Dpsk::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (index);
  m_ifIndex = index;
}

uint32_t
Dpsk::GetIfIndex (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_ifIndex;
}

Ptr<Channel>
Dpsk::GetChannel (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return NULL;
}

void
Dpsk::SetAddress (Address address)
{
  NS_LOG_FUNCTION (address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
Dpsk::GetAddress (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_address;
}

bool
Dpsk::SetMtu (const uint16_t mtu)
{
  NS_LOG_FUNCTION (mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
Dpsk::GetMtu (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_mtu;
}

bool
Dpsk::IsLinkUp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
Dpsk::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (&callback);
}

bool
Dpsk::IsBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
Dpsk::GetBroadcast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
Dpsk::IsMulticast (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

Address
Dpsk::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (multicastGroup);
  Mac48Address multicast = Mac48Address::GetMulticast (multicastGroup);
  return multicast;
}

Address
Dpsk::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (addr);
  return Mac48Address::GetMulticast (addr);
}

bool
Dpsk::IsBridge (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

bool
Dpsk::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return false;
}

bool
Dpsk::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
Dpsk::SendFrom (Ptr<Packet> packet, const Address &src, const Address &dest,
                uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (packet << src << dest << protocolNumber);

  // data was not port specific
  // flood through all ports.
  SendFromDevice (NULL, packet, protocolNumber, src, dest);

  return true;
}

Ptr<Node>
Dpsk::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void
Dpsk::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (node);
  m_node = node;
}

bool
Dpsk::NeedsArp (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
Dpsk::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_rxCallback = cb;
}

void
Dpsk::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  NS_LOG_FUNCTION (&cb);
  m_promiscRxCallback = cb;
}

bool
Dpsk::SupportsSendFrom () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return true;
}

void
Dpsk::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  for (std::vector<Ptr<NetDevice>>::iterator iter = m_ports.begin (); iter != m_ports.end ();
       iter++)
    {
      *iter = NULL;
    }
  m_ports.clear ();
  m_node = NULL;
  NetDevice::DoDispose ();
}

void
Dpsk::ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                         const Address &source, const Address &destination, PacketType packetType)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination << packetType);

  for (ReceiveFromDeviceHandlerList::iterator iter = m_handlers.begin (); iter != m_handlers.end ();
       iter++)
    {
      (*iter) (device, packet, protocol, source, destination, packetType);
    }
}

} // namespace ns3
