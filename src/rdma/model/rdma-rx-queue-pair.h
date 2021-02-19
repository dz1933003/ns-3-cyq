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

#ifndef RDMA_RX_QUEUE_PAIR_H
#define RDMA_RX_QUEUE_PAIR_H

#include "ns3/object.h"
#include "ns3/ipv4-address.h"
#include "ns3/simulator.h"
#include <deque>

namespace ns3 {

class RdmaRxQueuePair : public Object
{
public:
  Ipv4Address m_sIp, m_dIp;
  uint16_t m_sPort, m_dPort;
  uint64_t m_size;
  uint16_t m_priority;

  uint64_t m_receivedSize;

  enum IRN_STATE {
    ACK, // Acknowledged
    NACK, // Lost
    UNDEF // Out of window
  };

  struct
  {
    std::deque<IRN_STATE> pkg_state;
    uint32_t base_seq = 1;

    IRN_STATE
    GetIrnState (const uint32_t &seq) const
    {
      if (seq >= GetNextSequenceNumber ()) // out of window
        return IRN_STATE::UNDEF;
      else if (seq >= base_seq) // in window
        return pkg_state[seq - base_seq];
      else // before window
        return IRN_STATE::ACK;
    }

    void
    MoveWindow ()
    {
      while (!pkg_state.empty () && pkg_state.front () == IRN_STATE::ACK)
        {
          pkg_state.pop_front ();
          base_seq++;
        }
    }

    void
    UpdateIrnState (const uint32_t &seq)
    {
      // Packet not seen before
      if (GetIrnState (seq) == IRN_STATE::UNDEF)
        {
          // Sequence number out of order
          if (seq > GetNextSequenceNumber ())
            {
              while (seq > GetNextSequenceNumber ())
                pkg_state.push_back (IRN_STATE::NACK);
            }
          // ACK this packet
          pkg_state.push_back (IRN_STATE::ACK);
        }
      // Retransmission packet
      else if (GetIrnState (seq) == IRN_STATE::NACK)
        {
          // Ack this packet
          pkg_state[seq - base_seq] = IRN_STATE::ACK;
        }
      // If is ACKed, do nothing.
      MoveWindow ();
    }

    uint32_t
    GetNextSequenceNumber () const
    {
      return base_seq + pkg_state.size ();
    }

    bool
    IsReceived (const uint32_t &seq) const
    {
      return GetIrnState (seq) == IRN_STATE::ACK;
    }
  } m_irn;

  static TypeId GetTypeId (void);
  RdmaRxQueuePair ();
  RdmaRxQueuePair (Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort, uint16_t dPort, uint64_t size,
                   uint16_t priority);

  uint64_t GetRemainBytes ();
  uint32_t GetHash (void);
  static uint32_t GetHash (const Ipv4Address &sIp, const Ipv4Address &dIp, const uint16_t &sPort,
                           const uint16_t &dPort);
  bool IsFinished ();
};

} // namespace ns3

#endif /* RDMA_RX_QUEUE_PAIR_H */
