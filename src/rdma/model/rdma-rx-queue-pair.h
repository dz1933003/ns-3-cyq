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

/**
 * \ingroup rdma
 * \class RdmaRxQueuePair
 * \brief Rdma rx queue pair table entry.
 */
class RdmaRxQueuePair : public Object
{
public:
  Ipv4Address m_sIp, m_dIp; //!< source IP, dst IP
  uint16_t m_sPort, m_dPort; //!< source port, dst port
  uint64_t m_size; //!< source port, dst port
  uint16_t m_priority; //!< flow priority

  uint64_t m_receivedSize; //!< total valid received size (next expected sequence)

  static TypeId GetTypeId (void);
  RdmaRxQueuePair ();

  /**
   * Constructor
   * 
   * \param sIP source IP
   * \param dIP destination IP
   * \param sPort source port
   * \param dPort destination port
   * \param size queue pair length by byte
   * \param priority queue pair priority
   */
  RdmaRxQueuePair (Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort, uint16_t dPort, uint64_t size,
                   uint16_t priority);

  /**
   * Get remaining bytes of this queue pair
   * 
   * \return remaining bytes
   */
  uint64_t GetRemainBytes ();

  uint32_t GetHash (void);
  static uint32_t GetHash (const Ipv4Address &sIp, const Ipv4Address &dIp, const uint16_t &sPort,
                           const uint16_t &dPort);

  /**
   * \return true for finished queue pair
   */
  bool IsFinished ();

  /**
   * IRN rx bitmap state
   */
  enum IRN_STATE {
    ACK, // Acknowledged
    NACK, // Lost
    UNDEF // Out of window
  };

  /**
   * \ingroup rdma
   * \class Irn
   * \brief Rdma rx queue pair IRN infomation.
   */
  class Irn
  {
  public:
    /**
     * Get IRN bitmap state of this sequence number
     * 
     * \param seq sequence number
     * \return IRN bitmap state
     */
    IRN_STATE GetIrnState (const uint32_t &seq) const;

    /**
     * Move IRN bitmap window
     */
    void MoveWindow ();

    /**
     * After received a packet, update its IRN bitmap
     * 
     * \param seq sequence number
     */
    void UpdateIrnState (const uint32_t &seq);

    /**
     * Get next sequence number
     * 
     * \return expected sequence number
     */
    uint32_t GetNextSequenceNumber () const;

    /**
     * Is target sequence number of packet was received
     * 
     * \param seq sequence number
     * \return true for received, false for new packet
     */
    bool IsReceived (const uint32_t &seq) const;

  private:
    std::deque<IRN_STATE> m_states; //!< packet state bitmap window
    uint32_t m_baseSeq = 1; //!< bitmap window base sequence i.e. number of index 0
  } m_irn; //!< IRN infomation

  /**
   * \ingroup rdma
   * \class B2x
   * \brief Rdma rx queue pair back to N or back to zero infomation.
   */
  class B2x
  {
  public:
    Time m_nackTimer = Time (0);
    uint32_t m_rxMilestone = 0;
    uint32_t m_lastNack = 0;
  } m_b2x;
};

} // namespace ns3

#endif /* RDMA_RX_QUEUE_PAIR_H */
