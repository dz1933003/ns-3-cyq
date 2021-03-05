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

#include "rdma-rx-queue-pair.h"

#include "ns3/hash.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RdmaRxQueuePair");

NS_OBJECT_ENSURE_REGISTERED (RdmaRxQueuePair);

TypeId
RdmaRxQueuePair::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RdmaRxQueuePair")
                          .SetParent<Object> ()
                          .SetGroupName ("Rdma")
                          .AddConstructor<RdmaRxQueuePair> ();
  return tid;
}

RdmaRxQueuePair::RdmaRxQueuePair ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

RdmaRxQueuePair::RdmaRxQueuePair (Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort, uint16_t dPort,
                                  uint64_t size, uint16_t priority)
    : m_sIp (sIp),
      m_dIp (dIp),
      m_sPort (sPort),
      m_dPort (dPort),
      m_size (size),
      m_priority (priority),
      m_receivedSize (0)
{
  NS_LOG_FUNCTION (this << sIp << dIp << sPort << dPort << size << priority);
}

uint64_t
RdmaRxQueuePair::GetRemainBytes ()
{
  return (m_size >= m_receivedSize) ? m_size - m_receivedSize : 0;
}

uint32_t
RdmaRxQueuePair::GetHash (void)
{
  return GetHash (m_sIp, m_dIp, m_sPort, m_dPort);
}

uint32_t
RdmaRxQueuePair::GetHash (const Ipv4Address &sIp, const Ipv4Address &dIp, const uint16_t &sPort,
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
  buf.sIp = sIp.Get ();
  buf.dIp = dIp.Get ();
  buf.sPort = sPort;
  buf.dPort = dPort;
  return Hash32 (buf.bytes, sizeof (buf));
}

bool
RdmaRxQueuePair::IsFinished ()
{
  return m_receivedSize >= m_size;
}

// IRN inner class implementation

RdmaRxQueuePair::IRN_STATE
RdmaRxQueuePair::Irn::GetIrnState (const uint32_t &seq) const
{
  if (seq >= GetNextSequenceNumber ()) // out of window
    return IRN_STATE::UNDEF;
  else if (seq >= m_baseSeq) // in window
    return m_states[seq - m_baseSeq];
  else // before window
    return IRN_STATE::ACK;
}

void
RdmaRxQueuePair::Irn::MoveWindow ()
{
  while (!m_states.empty () && m_states.front () == IRN_STATE::ACK)
    {
      m_states.pop_front ();
      m_baseSeq++;
    }
}

void
RdmaRxQueuePair::Irn::UpdateIrnState (const uint32_t &seq)
{
  // Packet not seen before
  if (GetIrnState (seq) == IRN_STATE::UNDEF)
    {
      // Sequence number out of order
      if (seq > GetNextSequenceNumber ())
        {
          while (seq > GetNextSequenceNumber ())
            m_states.push_back (IRN_STATE::NACK);
        }
      // ACK this packet
      m_states.push_back (IRN_STATE::ACK);
    }
  // Retransmission packet
  else if (GetIrnState (seq) == IRN_STATE::NACK)
    {
      // Ack this packet
      m_states[seq - m_baseSeq] = IRN_STATE::ACK;
    }
  // If is ACKed, do nothing.
  MoveWindow ();
}

uint32_t
RdmaRxQueuePair::Irn::GetNextSequenceNumber () const
{
  return m_baseSeq + m_states.size ();
}

bool
RdmaRxQueuePair::Irn::IsReceived (const uint32_t &seq) const
{
  return GetIrnState (seq) == IRN_STATE::ACK;
}

} // namespace ns3
