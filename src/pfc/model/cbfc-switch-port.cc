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

#include "cbfc-switch-port.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/cbfc-header.h"
#include "ns3/dpsk-net-device.h"
#include "pfc-switch-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("CbfcSwitchPort");

NS_OBJECT_ENSURE_REGISTERED (CbfcSwitchPort);

TypeId
CbfcSwitchPort::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbfcSwitchPort")
                          .SetParent<Object> ()
                          .SetGroupName ("CbfcSwitchPort")
                          .AddConstructor<CbfcSwitchPort> ()
                          .AddTraceSource ("CbfcRx", "Receive a CBFC packet",
                                           MakeTraceSourceAccessor (&CbfcSwitchPort::m_cbfcRxTrace),
                                           "Ptr<DpskNetDevice>, uint32_t, uint64_t");
  return tid;
}

CbfcSwitchPort::CbfcSwitchPort ()
    : m_nQueues (0),
      m_lastQueueIdx (0),
      m_nInQueueBytes (0),
      m_nInQueuePackets (0),
      m_nTxBytes (0),
      m_nRxBytes (0)
{
  NS_LOG_FUNCTION (this);
  m_name = "CbfcSwitchPort";
}

CbfcSwitchPort::~CbfcSwitchPort ()
{
  NS_LOG_FUNCTION (this);
}

void
CbfcSwitchPort::SetupQueues (uint32_t n)
{
  NS_LOG_FUNCTION (n);
  CleanQueues ();
  m_nQueues = n;
  for (uint32_t i = 0; i <= m_nQueues; i++) // with another control queue
    {
      m_queues.push_back (std::queue<Ptr<Packet>> ());
      m_txStates.push_back (TxState ());
      m_inQueueBytesList.push_back (0);
      m_inQueuePacketsList.push_back (0);
    }
}

void
CbfcSwitchPort::CleanQueues ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_queues.clear ();
  m_txStates.clear ();
  m_inQueueBytesList.clear ();
  m_inQueuePacketsList.clear ();
}

void
CbfcSwitchPort::SetDeviceDequeueHandler (DeviceDequeueNotifier h)
{
  m_mmuCallback = h;
}

Ptr<Packet>
CbfcSwitchPort::Transmit ()
{
  NS_LOG_FUNCTION_NOARGS ();

  uint32_t qIndex;
  Ptr<Packet> p = Dequeue (qIndex);

  if (p == 0)
    return 0;

  // Notify switch dequeue event
  m_mmuCallback (m_dev, p, qIndex);

  PfcSwitchTag tag;
  p->RemovePacketTag (tag);

  m_nTxBytes += p->GetSize ();

  return p;
}

bool
CbfcSwitchPort::Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                      uint16_t protocolNumber)
{
  // Assembly Ethernet header
  EthernetHeader ethHeader;
  ethHeader.SetSource (Mac48Address::ConvertFrom (source));
  ethHeader.SetDestination (Mac48Address::ConvertFrom (dest));
  ethHeader.SetLengthType (protocolNumber);

  if (protocolNumber == CbfcHeader::PROT_NUM) // CBFC protocol number
    {
      // Add Ethernet header
      packet->AddHeader (ethHeader);
      // Enqueue control queue
      m_queues[m_nQueues].push (packet);
      m_inQueueBytesList[m_nQueues] += packet->GetSize ();
      m_inQueuePacketsList[m_nQueues]++;
    }
  else // Not CBFC
    {
      // Get queue index
      Ipv4Header ipHeader;
      packet->PeekHeader (ipHeader);
      auto dscp = ipHeader.GetDscp ();
      // Add Ethernet header
      packet->AddHeader (ethHeader);
      // Enqueue
      m_queues[dscp].push (packet);
      m_inQueueBytesList[dscp] += packet->GetSize ();
      m_inQueuePacketsList[dscp]++;
    }

  m_nInQueueBytes += packet->GetSize ();
  m_nInQueuePackets++;

  return true; // Enqueue packet successfully
}

bool
CbfcSwitchPort::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_nRxBytes += p->GetSize ();

  PfcSwitchTag tag (m_dev->GetIfIndex ());
  p->AddPacketTag (tag); // Add tag for tracing input device when dequeue in the net device

  // Get Ethernet header
  EthernetHeader ethHeader;
  p->PeekHeader (ethHeader);

  if (ethHeader.GetLengthType () == CbfcHeader::PROT_NUM) // CBFC protocol number
    {
      // Pop Ethernet header
      p->RemoveHeader (ethHeader);
      // Pop CBFC header
      CbfcHeader cbfcHeader;
      p->RemoveHeader (cbfcHeader);

      const auto fccl = cbfcHeader.GetFccl ();
      const auto qIndex = cbfcHeader.GetQIndex ();

      m_txStates[qIndex].txFccl = fccl;
      m_cbfcRxTrace (m_dev, qIndex, fccl);

      m_dev->TriggerTransmit ();
      return false;
    }
  else // Not CBFC
    {
      return true; // Forward up to node without any change
    }
}

Ptr<Packet>
CbfcSwitchPort::DequeueRoundRobin (uint32_t &qIndex)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_nInQueuePackets == 0 || m_nQueues == 0)
    {
      NS_LOG_LOGIC ("Queues empty");
      return 0;
    }
  else // Not control queues
    {
      for (uint32_t i = 0; i < m_nQueues; i++)
        {
          uint32_t qIdx = (m_lastQueueIdx + i) % m_nQueues;
          if (CanDequeue (qIdx))
            {
              Ptr<Packet> p = m_queues[qIdx].front ();
              m_queues[qIdx].pop ();

              m_txStates[qIdx].txFctbs += p->GetSize ();

              m_nInQueueBytes -= p->GetSize ();
              m_nInQueuePackets--;
              m_inQueueBytesList[qIdx] -= p->GetSize ();
              m_inQueuePacketsList[qIdx]--;

              m_lastQueueIdx = qIdx;
              qIndex = qIdx;

              return p;
            }
        }
      return 0;
    }
}

Ptr<Packet>
CbfcSwitchPort::Dequeue (uint32_t &qIndex)
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_inQueuePacketsList[m_nQueues] == 0) // No control packets
    {
      return DequeueRoundRobin (qIndex);
    }
  else // Dequeue control packets without FCCL check
    {
      Ptr<Packet> p = m_queues[m_nQueues].front ();
      m_queues[m_nQueues].pop ();

      m_nInQueueBytes -= p->GetSize ();
      m_nInQueuePackets--;
      m_inQueueBytesList[m_nQueues] -= p->GetSize ();
      m_inQueuePacketsList[m_nQueues]--;

      qIndex = m_nQueues;
      return p;
    }
}

bool
CbfcSwitchPort::CanDequeue (uint32_t qIndex)
{
  NS_LOG_FUNCTION (qIndex);
  // No packets in queue
  if (m_inQueuePacketsList[qIndex] == 0)
    return false;
  // Packets in queue
  Ptr<Packet> p = m_queues[qIndex].front ();
  auto fccl = m_txStates[qIndex].txFccl;
  auto fctbs = m_txStates[qIndex].txFctbs;
  // FCTBS greater than FCCL after send
  if (fccl < fctbs + p->GetSize ())
    return false;
  // Packets in queue and FCTBS less or equal to FCCL after send
  return true;
}

void
CbfcSwitchPort::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_queues.clear ();
  DpskNetDeviceImpl::DoDispose ();
}

} // namespace ns3
