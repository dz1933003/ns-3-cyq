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

#include "pfc-host.h"

#include "ns3/dpsk-net-device.h"
#include "ns3/dpsk-net-device-impl.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PfcHost");

NS_OBJECT_ENSURE_REGISTERED (PfcHost);

TypeId
PfcHost::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PfcHost")
                          .SetParent<DpskLayer> ()
                          .SetGroupName ("Pfc")
                          .AddConstructor<PfcHost> ();
  return tid;
}

PfcHost::PfcHost ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dpsk = NULL;
  m_name = "PfcHost";
  m_nDevices = 0;
}

PfcHost::~PfcHost ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
PfcHost::SendFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                         const Address &source, const Address &destination)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination);
  NS_ASSERT_MSG (false, "PfcHost::SendFromDevice: Do not use this function");
  return false;
}

void
PfcHost::ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                            const Address &source, const Address &destination,
                            NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination << packetType);
  // cyq: seems no need to handle packet here now
}

void
PfcHost::InstallDpsk (Ptr<Dpsk> dpsk)
{
  NS_LOG_FUNCTION (dpsk);
  m_dpsk = dpsk;

  for (const auto &dev : m_dpsk->GetDevices ())
    {
      const auto dpskDev = DynamicCast<DpskNetDevice> (dev);
      m_devices.insert (dpskDev);
    }

  m_nDevices = m_devices.size ();

  AggregateObject (m_dpsk->GetNode ());
}

void
PfcHost::AddRouteTableEntry (const Ipv4Address &dest, Ptr<DpskNetDevice> dev)
{
  NS_LOG_FUNCTION (dest << dev);
  NS_ASSERT_MSG (m_devices.find (dev) != m_devices.end (),
                 "PfcHost::AddRouteTableEntry: No such device");
  uint32_t destVal = dest.Get ();
  m_routeTable[destVal].push_back (dev);
}

std::unordered_map<uint32_t, std::vector<Ptr<DpskNetDevice>>>
PfcHost::GetRouteTable ()
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_routeTable;
}

void
PfcHost::ClearRouteTable ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_routeTable.clear ();
}

void
PfcHost::AddRdmaTxQueuePair (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);

  auto outDev = GetOutDev (qp);
  if (outDev == 0)
    {
      NS_ASSERT_MSG (false, "PfcHost::AddRdmaTxQueuePair: Find no output device");
      return;
    }
  outDev->GetObject<PfcHostPort> ()->AddRdmaTxQueuePair (qp);
}

std::vector<Ptr<RdmaTxQueuePair>>
PfcHost::GetRdmaTxQueuePairs ()
{
  NS_LOG_FUNCTION_NOARGS ();
  std::vector<Ptr<RdmaTxQueuePair>> result;
  for (const auto &dev : m_devices)
    {
      const auto temp = dev->GetObject<PfcHostPort> ()->GetRdmaTxQueuePairs ();
      result.insert (result.end (), temp.begin (), temp.end ());
    }
  return result;
}

std::map<uint32_t, Ptr<RdmaRxQueuePair>>
PfcHost::GetRdmaRxQueuePairs ()
{
  NS_LOG_FUNCTION_NOARGS ();
  std::map<uint32_t, Ptr<RdmaRxQueuePair>> result;
  for (const auto &dev : m_devices)
    {
      const auto temp = dev->GetObject<PfcHostPort> ()->GetRdmaRxQueuePairs ();
      result.insert (temp.begin (), temp.end ());
    }
  return result;
}

void
PfcHost::AddRdmaRxQueuePairSize (uint32_t key, uint64_t size)
{
  NS_LOG_FUNCTION (key << size);
  m_rxQueuePairSize[key] = size;
}

uint64_t
PfcHost::GetRdmaRxQueuePairSize (uint32_t key)
{
  NS_LOG_FUNCTION (key);
  return m_rxQueuePairSize[key];
}

Ptr<DpskNetDevice>
PfcHost::GetOutDev (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);

  if (m_routeTable.find (qp->m_dIp.Get ()) == m_routeTable.end ())
    {
      NS_ASSERT_MSG (false, "PfcHost::GetOutDev: No such route");
      return 0;
    }
  auto nextHops = m_routeTable[qp->m_dIp.Get ()];
  if (nextHops.empty () == false)
    {
      uint32_t key = qp->GetHash ();
      return nextHops[key % nextHops.size ()];
    }
  NS_ASSERT_MSG (false, "PfcHost::GetOutDev: No next hops");
  return 0;
}

void
PfcHost::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_devices.clear ();
  m_routeTable.clear ();
  DpskLayer::DoDispose ();
}

} // namespace ns3
