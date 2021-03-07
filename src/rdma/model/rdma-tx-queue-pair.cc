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

#include "rdma-tx-queue-pair.h"

#include "ns3/hash.h"
#include "ns3/log.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/qbb-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RdmaTxQueuePair");

NS_OBJECT_ENSURE_REGISTERED (RdmaTxQueuePair);

TypeId
RdmaTxQueuePair::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RdmaTxQueuePair")
                          .SetParent<Object> ()
                          .SetGroupName ("Rdma")
                          .AddConstructor<RdmaTxQueuePair> ();
  return tid;
}

RdmaTxQueuePair::RdmaTxQueuePair ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

RdmaTxQueuePair::RdmaTxQueuePair (Time startTime, Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort,
                                  uint16_t dPort, uint64_t size, uint16_t priority)
    : m_startTime (startTime),
      m_sIp (sIp),
      m_dIp (dIp),
      m_sPort (sPort),
      m_dPort (dPort),
      m_size (size),
      m_priority (priority),
      m_txSize (0),
      m_unackSize (0),
      m_nextAvail (0)
{
  NS_LOG_FUNCTION (this << startTime << sIp << dIp << sPort << dPort << size << priority);
}

uint64_t
RdmaTxQueuePair::GetRemainBytes () const
{
  return (m_size >= m_txSize) ? m_size - m_txSize : 0;
}

uint32_t
RdmaTxQueuePair::GetHash (void)
{
  union {
    struct
    {
      uint32_t sIp;
      uint32_t dIp;
      uint16_t sPort;
      uint16_t dPort;
    };
    char bytes[12];
  } buf;
  buf.sIp = m_sIp.Get ();
  buf.dIp = m_dIp.Get ();
  buf.sPort = m_sPort;
  buf.dPort = m_dPort;
  return Hash32 (buf.bytes, sizeof (buf));
}

uint32_t
RdmaTxQueuePair::GetHash (const Ipv4Address &sIp, const Ipv4Address &dIp, const uint16_t &sPort,
                          const uint16_t &dPort)
{
  union {
    struct
    {
      uint32_t sIp;
      uint32_t dIp;
      uint16_t sPort;
      uint16_t dPort;
    };
    char bytes[12];
  } buf;
  buf.sIp = dIp.Get ();
  buf.dIp = sIp.Get ();
  buf.sPort = dPort;
  buf.dPort = sPort;
  return Hash32 (buf.bytes, sizeof (buf));
}

bool
RdmaTxQueuePair::IsTxFinished () const
{
  return m_txSize >= m_size;
}

bool
RdmaTxQueuePair::IsAckedFinished () const
{
  return m_unackSize >= m_size;
}

void
RdmaTxQueuePair::SetupRate (const DataRate &maxRate, const DataRate &initRate)
{
  m_maxRate = maxRate;
  m_rate = initRate;
}

// B2N and B20 function implementation

void
RdmaTxQueuePair::SetupB2x (const bool &isVarWin, const uint32_t &winSize)
{
  m_isVarWin = isVarWin;
  m_winSize = winSize;
}

void
RdmaTxQueuePair::B2xRecover ()
{
  m_txSize = m_unackSize;
}

void
RdmaTxQueuePair::B2xAck (const uint64_t &ack)
{
  if (ack > m_unackSize)
    m_unackSize = ack;
}

uint64_t
RdmaTxQueuePair::B2xGetOnTheFly () const
{
  return m_txSize - m_unackSize;
}

bool
RdmaTxQueuePair::B2xIsWinBound () const
{
  const uint64_t w = B2xGetWin ();
  return w != 0 && B2xGetOnTheFly () >= w;
}

uint64_t
RdmaTxQueuePair::B2xGetWin () const
{
  if (m_winSize == 0)
    return 0;
  uint64_t w;
  if (m_isVarWin)
    {
      w = m_winSize * m_rate.GetBitRate () / m_maxRate.GetBitRate ();
      if (w == 0)
        w = 1; // must > 0
    }
  else
    {
      w = m_winSize;
    }
  return w;
}

// IRN inner class implementation

void
RdmaTxQueuePair::Irn::SendNewPacket (uint32_t payloadSize)
{
  m_states.push_back (RdmaTxQueuePair::IRN_STATE::UNACK);
  m_payloads.push_back (payloadSize);
  m_rtxEvents.push_back (EventId ());
}

RdmaTxQueuePair::IRN_STATE
RdmaTxQueuePair::Irn::GetIrnState (const uint32_t &seq) const
{
  if (seq >= GetNextSequenceNumber ())
    return IRN_STATE::UNDEF;
  else if (seq >= m_baseSeq)
    return m_states[seq - m_baseSeq];
  else
    return IRN_STATE::ACK;
}

uint64_t
RdmaTxQueuePair::Irn::GetPayloadSize (const uint32_t &seq) const
{
  if (seq >= GetNextSequenceNumber () || seq < m_baseSeq)
    {
      NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::GetPayloadSize: "
                            "Out of bound sequence number");
      return 0;
    }
  else
    return m_payloads[seq - m_baseSeq];
}

void
RdmaTxQueuePair::Irn::MoveWindow ()
{
  while (!m_states.empty () && m_states.front () == IRN_STATE::ACK)
    {
      m_states.pop_front ();
      m_payloads.pop_front ();
      m_rtxEvents.pop_front ();
      m_baseSeq++;
    }
}

void
RdmaTxQueuePair::Irn::AckIrnState (const uint32_t &seq)
{
  if (GetIrnState (seq) == IRN_STATE::UNDEF)
    {
      NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::AckIrnState: "
                            "Out of bound sequence number");
    }
  else if (GetIrnState (seq) == IRN_STATE::UNACK || GetIrnState (seq) == IRN_STATE::NACK)
    {
      m_states[seq - m_baseSeq] = IRN_STATE::ACK;
      // Cancel timer
      Simulator::Cancel (m_rtxEvents[seq - m_baseSeq]);
    }
  // If is ACK, do nothing.
  MoveWindow ();
}

void
RdmaTxQueuePair::Irn::SackIrnState (const uint32_t &seq, const uint32_t &ack)
{
  if (GetIrnState (seq) == IRN_STATE::UNDEF)
    {
      NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::SackIrnState: "
                            "Out of bound sequence number");
    }
  else if (GetIrnState (seq) == IRN_STATE::UNACK)
    {
      auto expIndex = ack - m_baseSeq;
      auto index = seq - m_baseSeq;
      m_states[index] = IRN_STATE::ACK;
      // Set NACK sequence
      for (auto i = expIndex; i < index; i++)
        {
          m_states[i] = IRN_STATE::NACK;
          // Cancel timer (because we will set timer when retransmitting)
          Simulator::Cancel (m_rtxEvents[i]);
        }
    }
  else if (GetIrnState (seq) == IRN_STATE::NACK || GetIrnState (seq) == IRN_STATE::ACK)
    {
      NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::SackIrnState: "
                            "Invalid SACK packet");
    }
  MoveWindow ();
}

void
RdmaTxQueuePair::Irn::SetRtxEvent (const uint32_t &seq, const EventId &id)
{
  if (seq >= m_baseSeq && seq < GetNextSequenceNumber ())
    {
      Simulator::Cancel (m_rtxEvents[seq - m_baseSeq]);
      m_rtxEvents[seq - m_baseSeq] = id;
    }
  else
    {
      NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::SetRtxEvent: "
                            "Invalid sequence number");
    }
}

uint32_t
RdmaTxQueuePair::Irn::GetNextSequenceNumber () const
{
  return m_baseSeq + m_states.size ();
}

uint32_t
RdmaTxQueuePair::Irn::GetWindowSize () const
{
  return m_states.size ();
}

// DCQCN inner class implementation

void
RdmaTxQueuePair::Dcqcn::CleanupTimer ()
{
  Simulator::Cancel (m_eventUpdateAlpha);
  Simulator::Cancel (m_eventDecreaseRate);
  Simulator::Cancel (m_rpTimer);
}

} // namespace ns3
