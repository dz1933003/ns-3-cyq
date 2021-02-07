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

#ifndef RDMA_TX_QUEUE_PAIR_H
#define RDMA_TX_QUEUE_PAIR_H

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include <deque>

namespace ns3 {

class RdmaTxQueuePair : public Object
{
public:
  Time m_startTime;
  Ipv4Address m_sIp, m_dIp;
  uint16_t m_sPort, m_dPort;
  uint64_t m_size;
  uint16_t m_priority;

  uint64_t m_sentSize;

  enum IRN_STATE { UNACK, ACK, NACK, UNDEF };

  struct
  {
    std::deque<IRN_STATE> pkg_state;
    std::deque<uint64_t> pkg_payload;
    uint32_t base_seq = 1;

    IRN_STATE
    GetIrnState (const uint32_t &seq) const
    {
      if (seq >= GetNextSequenceNumber ())
        return IRN_STATE::UNDEF;
      else if (seq >= base_seq)
        return pkg_state[seq - base_seq];
      else
        return IRN_STATE::ACK;
    }

    uint64_t
    GetPayloadSize (const uint32_t &seq) const
    {
      if (seq >= GetNextSequenceNumber () || seq < base_seq)
        {
          NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::GetPayloadSize: "
                                "Out of bound sequence number");
          return 0;
        }
      else
        return pkg_payload[seq - base_seq];
    }

    void
    MoveWindow ()
    {
      while (!pkg_state.empty () && pkg_state.front () == IRN_STATE::ACK)
        {
          pkg_state.pop_front ();
          pkg_payload.pop_front();
          base_seq++;
        }
    }

    void
    AckIrnState (const uint32_t &seq)
    {
      if (GetIrnState (seq) == IRN_STATE::UNDEF)
        {
          NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::AckIrnState: "
                                "Out of bound sequence number");
        }
      else if (GetIrnState (seq) == IRN_STATE::UNACK || GetIrnState (seq) == IRN_STATE::NACK)
        {
          pkg_state[seq - base_seq] = IRN_STATE::ACK;
        }
      // If is ACK, do nothing.
      MoveWindow ();
    }

    std::vector<uint32_t>
    SackIrnState (const uint32_t &seq)
    {
      std::vector<uint32_t> rtxSeqList;
      if (GetIrnState (seq) == IRN_STATE::UNDEF)
        {
          NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::SackIrnState: "
                                "Out of bound sequence number");
        }
      else if (GetIrnState (seq) == IRN_STATE::UNACK)
        {
          auto index = seq - base_seq;
          pkg_state[index] = IRN_STATE::ACK;
          // Set NACK sequence
          for (index--; index > 0 && pkg_state[index] != IRN_STATE::UNACK; index--)
            {
              pkg_state[index] = IRN_STATE::NACK;
              rtxSeqList.push_back (index + base_seq);
            }
        }
      else if (GetIrnState (seq) == IRN_STATE::NACK || GetIrnState (seq) == IRN_STATE::ACK)
        {
          NS_ASSERT_MSG (false, "RdmaTxQueuePair::m_irn::SackIrnState: "
                                "Invalid SACK packet");
        }
      MoveWindow ();
      return rtxSeqList;
    }

    uint32_t
    GetNextSequenceNumber () const
    {
      return base_seq + pkg_state.size ();
    }
  } m_irn;

  static TypeId GetTypeId (void);
  RdmaTxQueuePair ();
  RdmaTxQueuePair (Time startTime, Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort, uint16_t dPort,
                   uint64_t size, uint16_t priority);

  uint64_t GetRemainBytes ();
  uint32_t GetHash (void);
  static uint32_t GetHash (const Ipv4Address &sIp, const Ipv4Address &dIp, const uint16_t &sPort,
                           const uint16_t &dPort);
  bool IsFinished ();
};

} // namespace ns3

#endif /* RDMA_TX_QUEUE_PAIR_H */
