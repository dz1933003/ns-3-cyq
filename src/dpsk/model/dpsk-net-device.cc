/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2008 University of Washington
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

#include "ns3/log.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"
#include "ns3/mac48-address.h"
#include "ns3/llc-snap-header.h"
#include "ns3/error-model.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "dpsk-net-device.h"
#include "dpsk-net-device-impl.h"
#include "dpsk-channel.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DpskNetDevice");

NS_OBJECT_ENSURE_REGISTERED (DpskNetDevice);

TypeId
DpskNetDevice::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::DpskNetDevice")
          .SetParent<NetDevice> ()
          .SetGroupName ("DpskNetDevice")
          .AddConstructor<DpskNetDevice> ()
          .AddAttribute ("Mtu", "The MAC-level Maximum Transmission Unit",
                         UintegerValue (DEFAULT_MTU),
                         MakeUintegerAccessor (&DpskNetDevice::SetMtu, &DpskNetDevice::GetMtu),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("Address", "The MAC address of this device.",
                         Mac48AddressValue (Mac48Address ("ff:ff:ff:ff:ff:ff")),
                         MakeMac48AddressAccessor (&DpskNetDevice::m_address),
                         MakeMac48AddressChecker ())
          .AddAttribute ("DataRate", "The default data rate for point to point links",
                         DataRateValue (DataRate ("32768b/s")),
                         MakeDataRateAccessor (&DpskNetDevice::m_bps), MakeDataRateChecker ())
          .AddAttribute ("ReceiveErrorModel",
                         "The receiver error model used to simulate packet loss", PointerValue (),
                         MakePointerAccessor (&DpskNetDevice::m_receiveErrorModel),
                         MakePointerChecker<ErrorModel> ())
          .AddAttribute ("InterframeGap", "The time to wait between packet (frame) transmissions",
                         TimeValue (Seconds (0.0)),
                         MakeTimeAccessor (&DpskNetDevice::m_tInterframeGap), MakeTimeChecker ())
          .AddAttribute ("TxMode", "The mode of transmitting, passive or active",
                         EnumValue (PASSIVE), MakeEnumAccessor (&DpskNetDevice::m_txMode),
                         MakeEnumChecker (PASSIVE, "Passive", ACTIVE, "Active"))

          //
          // Transmit queueing discipline for the device which includes its own set
          // of trace hooks.
          //
          .AddAttribute ("TxQueue", "A queue to use as the transmit queue in the device.",
                         PointerValue (), MakePointerAccessor (&DpskNetDevice::m_queue),
                         MakePointerChecker<Queue<Packet>> ())

          //
          // Trace sources at the "top" of the net device, where packets transition
          // to/from higher layers.
          //
          .AddTraceSource ("MacTx",
                           "Trace source indicating a packet has arrived "
                           "for transmission by this device",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_macTxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacTxDrop",
                           "Trace source indicating a packet has been dropped "
                           "by the device before transmission",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_macTxDropTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacPromiscRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a promiscuous trace,",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_macPromiscRxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("MacRx",
                           "A packet has been received by this device, "
                           "has been passed up from the physical layer "
                           "and is being forwarded up the local protocol stack.  "
                           "This is a non-promiscuous trace,",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_macRxTrace),
                           "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("MacRxDrop", 
                     "Trace source indicating a packet was dropped "
                     "before being forwarded up the stack",
                     MakeTraceSourceAccessor (&DpskNetDevice::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
#endif
          //
          // Trace sources at the "bottom" of the net device, where packets transition
          // to/from the channel.
          //
          .AddTraceSource ("PhyTxBegin",
                           "Trace source indicating a packet has begun "
                           "transmitting over the channel",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_phyTxBeginTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxEnd",
                           "Trace source indicating a packet has been "
                           "completely transmitted over the channel",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_phyTxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyTxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device during transmission",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_phyTxDropTrace),
                           "ns3::Packet::TracedCallback")
#if 0
    // Not currently implemented for this device
    .AddTraceSource ("PhyRxBegin", 
                     "Trace source indicating a packet has begun "
                     "being received by the device",
                     MakeTraceSourceAccessor (&DpskNetDevice::m_phyRxBeginTrace),
                     "ns3::Packet::TracedCallback")
#endif
          .AddTraceSource ("PhyRxEnd",
                           "Trace source indicating a packet has been "
                           "completely received by the device",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_phyRxEndTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PhyRxDrop",
                           "Trace source indicating a packet has been "
                           "dropped by the device during reception",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_phyRxDropTrace),
                           "ns3::Packet::TracedCallback")

          //
          // Trace sources designed to simulate a packet sniffer facility (tcpdump).
          // Note that there is really no difference between promiscuous and
          // non-promiscuous traces in a point-to-point link.
          //
          .AddTraceSource ("Sniffer",
                           "Trace source simulating a non-promiscuous packet sniffer "
                           "attached to the device",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_snifferTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("PromiscSniffer",
                           "Trace source simulating a promiscuous packet sniffer "
                           "attached to the device",
                           MakeTraceSourceAccessor (&DpskNetDevice::m_promiscSnifferTrace),
                           "ns3::Packet::TracedCallback");
  return tid;
}

DpskNetDevice::DpskNetDevice ()
    : m_keepTransmit (false),
      m_txMachineState (READY),
      m_channel (0),
      m_linkUp (false),
      m_currentPkt (0)
{
  NS_LOG_FUNCTION (this);
}

DpskNetDevice::~DpskNetDevice ()
{
  NS_LOG_FUNCTION (this);
}

void
DpskNetDevice::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_node = 0;
  m_channel = 0;
  m_receiveErrorModel = 0;
  m_currentPkt = 0;
  m_queue = 0;
  NetDevice::DoDispose ();
}

void
DpskNetDevice::SetDataRate (DataRate bps)
{
  NS_LOG_FUNCTION (this);
  m_bps = bps;
}

void
DpskNetDevice::SetInterframeGap (Time t)
{
  NS_LOG_FUNCTION (this << t.GetSeconds ());
  m_tInterframeGap = t;
}

DpskNetDevice::TxMode
DpskNetDevice::GetTxMode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txMode;
}

void
DpskNetDevice::SetTxMode (const TxMode &mode)
{
  NS_LOG_FUNCTION (this << mode);
  m_txMode = mode;
}

void
DpskNetDevice::SetImplementation (Ptr<DpskNetDeviceImpl> impl)
{
  NS_LOG_FUNCTION (impl);
  m_impl = impl;
  m_impl->m_dev = this;
  SetTransmitInterceptor (MakeCallback (&DpskNetDeviceImpl::Transmit, m_impl));
  SetSendInterceptor (MakeCallback (&DpskNetDeviceImpl::Send, m_impl));
  SetReceiveInterceptor (MakeCallback (&DpskNetDeviceImpl::Receive, m_impl));
  AggregateObject (m_impl);
}

Ptr<DpskNetDeviceImpl>
DpskNetDevice::GetImplementation ()
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_impl;
}

void
DpskNetDevice::ResetImplementation ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_impl = 0;
  m_impl->m_dev = 0;
  ResetTransmitInterceptor ();
  ResetSendInterceptor ();
  ResetReceiveInterceptor ();
  ResetUpdateNextAvail ();
}

void
DpskNetDevice::SetTransmitInterceptor (TransmitInterceptor h)
{
  NS_LOG_FUNCTION (&h);
  m_txInterceptor = h;
}

void
DpskNetDevice::ResetTransmitInterceptor ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_txInterceptor.Nullify ();
}

void
DpskNetDevice::SetSendInterceptor (SendInterceptor h)
{
  NS_LOG_FUNCTION (&h);
  m_sendInterceptor = h;
}

void
DpskNetDevice::ResetSendInterceptor ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sendInterceptor.Nullify ();
}

void
DpskNetDevice::SetUpdateNextAvail (UpdateNextAvail h)
{
  NS_LOG_FUNCTION (&h);
  m_updateNextAvail = h;
}

void
DpskNetDevice::ResetUpdateNextAvail ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_updateNextAvail.Nullify ();
}

void
DpskNetDevice::SetReceiveInterceptor (ReceiveInterceptor h)
{
  NS_LOG_FUNCTION (&h);
  m_rxInterceptor = h;
}

void
DpskNetDevice::ResetReceiveInterceptor ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rxInterceptor.Nullify ();
}

void
DpskNetDevice::TriggerTransmit ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT_MSG (m_txMode == ACTIVE, "Must be ACTIVE transmit mode");

  if (m_keepTransmit == false && m_txMode == ACTIVE)
    {
      m_keepTransmit = true;
      TransmitRequest ();
    }
  // otherwise this device is still transmitting
}

void
DpskNetDevice::PauseTransmit ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT_MSG (m_txMode == ACTIVE, "Must be ACTIVE transmit mode");

  if (m_txMode == ACTIVE)
    m_keepTransmit = false;
}

bool
DpskNetDevice::TransmitStart (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_LOGIC ("UID is " << p->GetUid () << ")");

  //
  // This function is called to start the process of transmitting a packet.
  // We need to tell the channel that we've started wiggling the wire and
  // schedule an event that will be executed when the transmission is complete.
  //
  NS_ASSERT_MSG (m_txMachineState == READY, "Must be READY to transmit");
  m_txMachineState = BUSY;
  m_currentPkt = p;
  m_phyTxBeginTrace (m_currentPkt);

  Time txTime = m_bps.CalculateBytesTxTime (p->GetSize ());
  Time txCompleteTime = txTime + m_tInterframeGap;

  NS_LOG_LOGIC ("Schedule TransmitCompleteEvent in " << txCompleteTime.GetSeconds () << "sec");
  Simulator::Schedule (txCompleteTime, &DpskNetDevice::TransmitComplete, this);

  bool result = m_channel->TransmitStart (p, this, txTime);
  if (result == false)
    {
      m_phyTxDropTrace (p);
    }
  return result;
}

bool
DpskNetDevice::TransmitRequest ()
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Packet> p = m_txInterceptor (); // DPSK transmit process
  if (p == 0)
    {
      m_keepTransmit = false;
      return false;
    }
  else
    {
      m_snifferTrace (p);
      m_promiscSnifferTrace (p);
      bool result = TransmitStart (p);
      if (!m_updateNextAvail.IsNull ())
        m_updateNextAvail (p, m_tInterframeGap);
      return result;
    }
}

void
DpskNetDevice::TransmitComplete (void)
{
  NS_LOG_FUNCTION (this);

  //
  // This function is called to when we're all done transmitting a packet.
  // We try and pull another packet off of the transmit queue.  If the queue
  // is empty, we are done, otherwise we need to start transmitting the
  // next packet.
  //
  NS_ASSERT_MSG (m_txMachineState == BUSY, "Must be BUSY if transmitting");
  m_txMachineState = READY;

  NS_ASSERT_MSG (m_currentPkt != 0, "DpskNetDevice::TransmitComplete(): m_currentPkt zero");

  m_phyTxEndTrace (m_currentPkt);
  m_currentPkt = 0;

  if (m_txMode == PASSIVE)
    {
      Ptr<Packet> p = m_queue->Dequeue ();
      if (p == 0)
        {
          NS_LOG_LOGIC ("No pending packets in device queue after tx complete");
          return;
        }

      //
      // Got another packet off of the queue, so start the transmit process again.
      //
      m_snifferTrace (p);
      m_promiscSnifferTrace (p);
      TransmitStart (p);
      m_updateNextAvail (p, m_tInterframeGap);
    }
  else if (m_txMode == ACTIVE)
    {
      if (m_keepTransmit)
        {
          NS_LOG_LOGIC ("Request transmiting");
          TransmitRequest ();
        }
    }
  else
    {
      NS_ASSERT_MSG (false, "DpskNetDevice::TransmitComplete(): m_txMode outbound");
    }
}

bool
DpskNetDevice::Attach (Ptr<DpskChannel> ch)
{
  NS_LOG_FUNCTION (this << &ch);

  m_channel = ch;

  m_channel->Attach (this);

  //
  // This device is up whenever it is attached to a channel.  A better plan
  // would be to have the link come up when both devices are attached, but this
  // is not done for now.
  //
  NotifyLinkUp ();
  return true;
}

void
DpskNetDevice::SetQueue (Ptr<Queue<Packet>> q)
{
  NS_LOG_FUNCTION (this << q);
  m_queue = q;
}

void
DpskNetDevice::SetReceiveErrorModel (Ptr<ErrorModel> em)
{
  NS_LOG_FUNCTION (this << em);
  m_receiveErrorModel = em;
}

void
DpskNetDevice::Receive (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  uint16_t protocol = 0;

  if (m_receiveErrorModel && m_receiveErrorModel->IsCorrupt (packet))
    {
      //
      // If we have an error model and it indicates that it is time to lose a
      // corrupted packet, don't forward this packet up, let it go.
      //
      m_phyRxDropTrace (packet);
    }
  else
    {
      //
      // Hit the trace hooks.  All of these hooks are in the same place in this
      // device because it is so simple, but this is not usually the case in
      // more complicated devices.
      //
      m_snifferTrace (packet);
      m_promiscSnifferTrace (packet);
      m_phyRxEndTrace (packet);

      //
      // Trace sinks will expect complete packets, not packets without some of the
      // headers.
      //
      Ptr<Packet> originalPacket = packet->Copy ();

      if (m_rxInterceptor (packet))
        {
          if (!m_promiscCallback.IsNull ())
            {
              m_macPromiscRxTrace (originalPacket);
              m_promiscCallback (this, packet, protocol, GetRemote (), GetAddress (),
                                 NetDevice::PACKET_HOST); // DPSK receive process
            }

          m_macRxTrace (originalPacket);

          m_rxCallback (this, packet, protocol, GetRemote ()); // Other protocols
        }
    }
}

Ptr<Queue<Packet>>
DpskNetDevice::GetQueue (void) const
{
  NS_LOG_FUNCTION (this);
  return m_queue;
}

void
DpskNetDevice::NotifyLinkUp (void)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = true;
  m_linkChangeCallbacks ();
}

void
DpskNetDevice::SetIfIndex (const uint32_t index)
{
  NS_LOG_FUNCTION (this);
  m_ifIndex = index;
}

uint32_t
DpskNetDevice::GetIfIndex (void) const
{
  return m_ifIndex;
}

Ptr<Channel>
DpskNetDevice::GetChannel (void) const
{
  return m_channel;
}

//
// This is a DPSK net device, so we really don't need any kind of address
// information.  However, the base class NetDevice wants us to define the
// methods to get and set the address.  Rather than be rude and assert, we let
// clients get and set the address, but simply ignore them.

void
DpskNetDevice::SetAddress (Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_address = Mac48Address::ConvertFrom (address);
}

Address
DpskNetDevice::GetAddress (void) const
{
  return m_address;
}

bool
DpskNetDevice::IsLinkUp (void) const
{
  NS_LOG_FUNCTION (this);
  return m_linkUp;
}

void
DpskNetDevice::AddLinkChangeCallback (Callback<void> callback)
{
  NS_LOG_FUNCTION (this);
  m_linkChangeCallbacks.ConnectWithoutContext (callback);
}

//
// This is a DPSK net device, so every transmission is a broadcast to
// all of the devices on the network.
//
bool
DpskNetDevice::IsBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

//
// We don't really need any addressing information since this is a
// DPSK net device.  The base class NetDevice wants us to return a
// broadcast address, so we make up something reasonable.
//
Address
DpskNetDevice::GetBroadcast (void) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("ff:ff:ff:ff:ff:ff");
}

bool
DpskNetDevice::IsMulticast (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

Address
DpskNetDevice::GetMulticast (Ipv4Address multicastGroup) const
{
  NS_LOG_FUNCTION (this);
  return Mac48Address ("01:00:5e:00:00:00");
}

Address
DpskNetDevice::GetMulticast (Ipv6Address addr) const
{
  NS_LOG_FUNCTION (this << addr);
  return Mac48Address ("33:33:00:00:00:00");
}

bool
DpskNetDevice::IsPointToPoint (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

bool
DpskNetDevice::IsBridge (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

bool
DpskNetDevice::Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << dest << protocolNumber);
  return SendFrom (packet, m_address, dest, protocolNumber);
}

bool
DpskNetDevice::SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                         uint16_t protocolNumber)
{
  NS_LOG_FUNCTION (this << packet << source << dest << protocolNumber);
  NS_LOG_LOGIC ("p=" << packet << ", dest=" << &dest);
  NS_LOG_LOGIC ("UID is " << packet->GetUid ());

  //
  // If IsLinkUp() is false it means there is no channel to send any packet
  // over so we just hit the drop trace on the packet and return an error.
  //
  if (IsLinkUp () == false)
    {
      m_macTxDropTrace (packet);
      return false;
    }

  m_macTxTrace (packet);

  if (m_txMode == PASSIVE)
    {
      //
      // We should enqueue and dequeue the packet to hit the tracing hooks.
      //
      if (m_queue->Enqueue (packet))
        {
          //
          // If the channel is ready for transition we send the packet right now
          //
          if (m_txMachineState == READY)
            {
              packet = m_queue->Dequeue ();
              m_snifferTrace (packet);
              m_promiscSnifferTrace (packet);
              bool ret = TransmitStart (packet);
              return ret;
            }
          return true;
        }

      // Enqueue may fail (overflow)

      m_macTxDropTrace (packet);
      return false;
    }
  else if (m_txMode == ACTIVE)
    {
      bool status = m_sendInterceptor (packet, source, dest, protocolNumber); // DPSK send process
      if (status == false)
        {
          m_macTxDropTrace (packet);
          return false;
        }
      else
        {
          TriggerTransmit ();
          return status;
        }
    }
  else
    {
      NS_ASSERT_MSG (false, "DpskNetDevice::SendFrom(): m_txMode outbound");
    }
}

Ptr<Node>
DpskNetDevice::GetNode (void) const
{
  return m_node;
}

void
DpskNetDevice::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION (this);
  m_node = node;
}

bool
DpskNetDevice::NeedsArp (void) const
{
  NS_LOG_FUNCTION (this);
  return false;
}

void
DpskNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback cb)
{
  m_rxCallback = cb;
}

void
DpskNetDevice::SetPromiscReceiveCallback (NetDevice::PromiscReceiveCallback cb)
{
  m_promiscCallback = cb;
}

bool
DpskNetDevice::SupportsSendFrom (void) const
{
  NS_LOG_FUNCTION (this);
  return true;
}

void
DpskNetDevice::DoMpiReceive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  Receive (p);
}

DataRate
DpskNetDevice::GetDataRate () const
{
  NS_LOG_FUNCTION (this);
  return m_bps;
}

Address
DpskNetDevice::GetRemote (void) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_channel->GetNDevices () == 2);
  for (std::size_t i = 0; i < m_channel->GetNDevices (); ++i)
    {
      Ptr<NetDevice> tmp = m_channel->GetDevice (i);
      if (tmp != this)
        {
          return tmp->GetAddress ();
        }
    }
  NS_ASSERT (false);
  // quiet compiler.
  return Address ();
}

bool
DpskNetDevice::SetMtu (uint16_t mtu)
{
  NS_LOG_FUNCTION (this << mtu);
  m_mtu = mtu;
  return true;
}

uint16_t
DpskNetDevice::GetMtu (void) const
{
  NS_LOG_FUNCTION (this);
  return m_mtu;
}

} // namespace ns3
