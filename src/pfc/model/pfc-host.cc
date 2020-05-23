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
  // XXX cyq: seems no need to handle packet here now
}

void
PfcHost::InstallDpsk (Ptr<Dpsk> dpsk)
{
  NS_LOG_FUNCTION (dpsk);
  m_dpsk = dpsk;

  for (const auto &dev : m_dpsk->GetDevices ())
    {
      const auto &dpskDev = DynamicCast<DpskNetDevice> (dev);
      const auto &dpskDevImpl = dpskDev->GetImplementation ();
      const auto &pfcPort = DynamicCast<PfcHostPort> (dpskDevImpl);
      m_devices.insert ({dpskDev, pfcPort});
    }

  m_nDevices = m_devices.size ();
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
  m_devices[outDev]->AddRdmaTxQueuePair (qp);
}

Ptr<DpskNetDevice>
PfcHost::GetOutDev (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);

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
