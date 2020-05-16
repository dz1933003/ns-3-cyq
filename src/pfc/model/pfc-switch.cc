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

#include "pfc-switch.h"

#include "ns3/dpsk-net-device.h"
#include "ns3/dpsk-net-device-impl.h"
#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "pfc-header.h"
#include "pfc-switch-tag.h"
#include "pfc-switch-port.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PfcSwitch");

NS_OBJECT_ENSURE_REGISTERED (PfcSwitch);

TypeId
PfcSwitch::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PfcSwitch")
                          .SetParent<DpskLayer> ()
                          .SetGroupName ("Pfc")
                          .AddConstructor<PfcSwitch> ();
  return tid;
}

PfcSwitch::PfcSwitch ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dpsk = NULL;
  m_name = "PfcSwitch";
  m_nDevices = 0;
  m_mmu = 0;
}

PfcSwitch::~PfcSwitch ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

bool
PfcSwitch::SendFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                           const Address &source, const Address &destination)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination);

  if (m_dpsk != NULL)
    {
      m_dpsk->SendFromDevice (device, packet->Copy (), protocol, source, destination);
    }

  return true;
}

void
PfcSwitch::ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                              const Address &source, const Address &destination,
                              NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination << packetType);
  // TODO cyq: trace packet
  Ptr<DpskNetDevice> inDev = DynamicCast<DpskNetDevice> (device);
  Ptr<DpskNetDevice> outDev = DynamicCast<DpskNetDevice> (GetOutDev (packet));
  if (outDev == 0)
    return; // Drop packet

  NS_ASSERT_MSG (outDev->IsLinkUp (), "The routing table look up should return link that is up");

  Ipv4Header ipHeader;
  packet->PeekHeader (ipHeader);

  uint32_t pSize = packet->GetSize ();
  auto dscp = ipHeader.GetDscp ();
  uint32_t qIndex = (dscp >= m_nQueues) ? m_nQueues : dscp;

  if (qIndex != m_nQueues) // not control queue
    {
      // Check enqueue admission
      if (m_mmu->CheckIngressAdmission (inDev, dscp, pSize) &&
          m_mmu->CheckEgressAdmission (outDev, dscp, pSize))
        {
          // Enqueue
          m_mmu->UpdateIngressAdmission (inDev, dscp, pSize);
          m_mmu->UpdateEgressAdmission (outDev, dscp, pSize);
        }
      else
        return; // Drop packet

      // Check and send PFC
      if (m_mmu->CheckShouldSendPfcPause (inDev, qIndex))
        {
          m_mmu->SetPause (inDev, qIndex);

          PfcHeader pfcHeader (PfcHeader::PfcType::Pause, qIndex);
          Ptr<Packet> pfc = Create<Packet> (0);
          pfc->AddHeader (pfcHeader);

          SendFromDevice (inDev, pfc, PfcHeader::PROT_NUM, inDev->GetAddress (),
                          inDev->GetRemote ());
        }
    }

  SendFromDevice (outDev, packet, protocol, outDev->GetAddress (), outDev->GetRemote ());
}

void
PfcSwitch::InstallDpsk (Ptr<Dpsk> dpsk)
{
  NS_LOG_FUNCTION (dpsk);
  m_dpsk = dpsk;
  m_dpsk->RegisterReceiveFromDeviceHandler (MakeCallback (&PfcSwitch::ReceiveFromDevice, this));
  m_devices = m_dpsk->GetDevices ();
  m_nDevices = m_devices.size ();
  m_nQueues = 0;

  for (const auto &dev : m_devices)
    {
      const auto &dpskDevImpl = DynamicCast<DpskNetDevice> (dev)->GetImplementation ();
      const auto &pfcPortImpl = DynamicCast<PfcSwitchPort> (dpskDevImpl);
      pfcPortImpl->SetDeviceDequeueHandler (MakeCallback (&PfcSwitch::DeviceDequeueHandler, this));
    }
}

void
PfcSwitch::InstallMmu (Ptr<SwitchMmu> mmu)
{
  m_mmu = mmu;
  m_mmu->AggregateDevices (m_devices, m_nQueues);
}

void
PfcSwitch::SetEcmpSeed (uint32_t s)
{
  m_ecmpSeed = s;
}

void
PfcSwitch::SetNQueues (uint32_t n)
{
  m_nQueues = n;
}

void
PfcSwitch::AddRouteTableEntry (const Ipv4Address &dest, Ptr<NetDevice> dev)
{
  uint32_t destVal = dest.Get ();
  m_routeTable[destVal].push_back (dev);
}

void
PfcSwitch::ClearRouteTable ()
{
  m_routeTable.clear ();
}

Ptr<NetDevice>
PfcSwitch::GetOutDev (Ptr<const Packet> p)
{
  Ptr<Packet> packet = p->Copy ();
  // Reveal IP header
  Ipv4Header ipHeader;
  packet->RemoveHeader (ipHeader);
  uint32_t srcAddr = ipHeader.GetSource ().Get ();
  uint32_t destAddr = ipHeader.GetDestination ().Get ();
  uint8_t protocol = ipHeader.GetProtocol ();

  auto entry = m_routeTable.find (destAddr);
  if (entry == m_routeTable.end ())
    return 0;
  auto &nextHops = entry->second;

  union {
    uint8_t u8[12];
    uint32_t u32[3];
  } ecmpKey;
  size_t ecmpKeyLen = 0;

  // Assemble IP to ECMP key
  ecmpKey.u32[0] = srcAddr;
  ecmpKey.u32[1] = destAddr;
  ecmpKeyLen += 2;

  if (protocol == 0x11) // UDP
    {
      // Reveal UDP header
      UdpHeader udpHeader;
      packet->PeekHeader (udpHeader);
      uint32_t srcPort = udpHeader.GetSourcePort ();
      uint32_t destPort = udpHeader.GetDestinationPort ();
      // Assemble UDP to ECMP key
      ecmpKey.u32[2] = srcPort | (destPort << 16);
      ecmpKeyLen++;
    }
  else
    {
      // Unexpected
      NS_ASSERT_MSG (false, "PfcSwitch::GetOutDev: Unexpected IP protocol number");
      return 0;
    }

  uint32_t hashVal = CalcEcmpHash (ecmpKey.u8, ecmpKeyLen) % nextHops.size ();
  return nextHops[hashVal];
}

uint32_t
PfcSwitch::CalcEcmpHash (const uint8_t *key, size_t len)
{
  uint32_t h = m_ecmpSeed;
  if (len > 3)
    {
      const uint32_t *key_x4 = (const uint32_t *) key;
      size_t i = len >> 2;
      do
        {
          uint32_t k = *key_x4++;
          k *= 0xcc9e2d51;
          k = (k << 15) | (k >> 17);
          k *= 0x1b873593;
          h ^= k;
          h = (h << 13) | (h >> 19);
          h += (h << 2) + 0xe6546b64;
        }
      while (--i);
      key = (const uint8_t *) key_x4;
    }
  if (len & 3)
    {
      size_t i = len & 3;
      uint32_t k = 0;
      key = &key[i - 1];
      do
        {
          k <<= 8;
          k |= *key--;
        }
      while (--i);
      k *= 0xcc9e2d51;
      k = (k << 15) | (k >> 17);
      k *= 0x1b873593;
      h ^= k;
    }
  h ^= len;
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

void
PfcSwitch::DeviceDequeueHandler (Ptr<NetDevice> outDev, Ptr<Packet> packet, uint32_t qIndex)
{
  PfcSwitchTag tag;
  packet->PeekPacketTag (tag);
  Ptr<DpskNetDevice> inDev = DynamicCast<DpskNetDevice> (tag.GetInDev ());
  uint32_t pSize = packet->GetSize ();

  if (qIndex == m_nQueues) // control queue
    return;
  // Update ingress and egress
  m_mmu->RemoveFromIngressAdmission (inDev, qIndex, pSize);
  m_mmu->RemoveFromEgressAdmission (outDev, qIndex, pSize);
  // ECN
  bool congested = m_mmu->CheckShouldSetEcn (outDev, qIndex);
  if (congested)
    {
      EthernetHeader ethHeader;
      Ipv4Header ipHeader;
      packet->RemoveHeader (ethHeader);
      packet->RemoveHeader (ipHeader);
      ipHeader.SetEcn (Ipv4Header::EcnType::ECN_CE);
      packet->AddHeader (ipHeader);
      packet->AddHeader (ethHeader);
    }
  // Check and send resume
  if (m_mmu->CheckShouldSendPfcResume (inDev, qIndex))
    {
      m_mmu->SetResume (inDev, qIndex);

      PfcHeader PfcHeader (PfcHeader::PfcType::Resume, qIndex);
      Ptr<Packet> pfc = Create<Packet> (0);
      pfc->AddHeader (PfcHeader);

      SendFromDevice (inDev, pfc, PfcHeader::PROT_NUM, inDev->GetAddress (), inDev->GetRemote ());
    }
}

void
PfcSwitch::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_devices.clear ();
  m_routeTable.clear ();
  m_mmu = 0;
  DpskLayer::DoDispose ();
}

} // namespace ns3
