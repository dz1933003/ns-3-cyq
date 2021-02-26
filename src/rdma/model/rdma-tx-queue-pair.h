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
#include "ns3/data-rate.h"
#include "ns3/packet.h"
#include <deque>

namespace ns3 {

/**
 * \ingroup rdma
 * \class RdmaTxQueuePair
 * \brief Rdma tx queue pair table entry.
 */
class RdmaTxQueuePair : public Object
{
public:
  Time m_startTime; //!< queue pair arrival time
  Ipv4Address m_sIp, m_dIp; //!< source IP, dst IP
  uint16_t m_sPort, m_dPort; //!< source port, dst port
  uint64_t m_size; //!< source port, dst port
  uint16_t m_priority; //!< flow priority

  uint64_t m_sentSize; //!< total valid sent size

  static TypeId GetTypeId (void);
  RdmaTxQueuePair ();

  /**
   * Constructor
   * 
   * \param startTime queue pair arrival time
   * \param sIP source IP
   * \param dIP destination IP
   * \param sPort source port
   * \param dPort destination port
   * \param size queue pair length by byte
   * \param priority queue pair priority
   */
  RdmaTxQueuePair (Time startTime, Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort, uint16_t dPort,
                   uint64_t size, uint16_t priority);

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
   * IRN tx bitmap state
   */
  enum IRN_STATE {
    UNACK, // Not acked
    ACK, // Acked
    NACK, // Lost
    UNDEF // Out of window
  };

  /**
   * \ingroup rdma
   * \class Irn
   * \brief Rdma tx queue pair IRN infomation.
   */
  class Irn
  {
  public:
    /**
     * After generate and send a new packet, update its IRN bitmap
     * 
     * \param payloadSize log new packet payload size
     */
    void SendNewPacket (uint32_t payloadSize);

    /**
     * Get IRN bitmap state of this sequence number
     * 
     * \param seq sequence number
     * \return IRN bitmap state
     */
    IRN_STATE GetIrnState (const uint32_t &seq) const;

    /**
     * Get payload size of this sequence number
     * 
     * \param seq sequence number
     * \return payload size
     */
    uint64_t GetPayloadSize (const uint32_t &seq) const;

    /**
     * Move IRN bitmap window
     */
    void MoveWindow ();

    /**
     * Update IRN state after received ACK
     * 
     * \param seq ACKed sequence number
     */
    void AckIrnState (const uint32_t &seq);

    /**
     * Update IRN state after received SACK
     * 
     * \param seq ACKed sequence number
     * \param ack expected sequence number
     */
    void SackIrnState (const uint32_t &seq, const uint32_t &ack);

    /**
     * Log retransmission event id
     * 
     * \param seq sequence number
     * \param id NS3 event ID
     */
    void SetRtxEvent (const uint32_t &seq, const EventId &id);

    /**
     * Get next sequence number
     * 
     * \return expected sequence number
     */
    uint32_t GetNextSequenceNumber () const;

    /**
     * Get bitmap window size by packet count
     * 
     * \return windows size by packet count
     */
    uint32_t GetWindowSize () const;

  private:
    std::deque<IRN_STATE> m_states; //!< packet state bitmap window
    std::deque<uint64_t> m_payloads; //!< packet payload bitmap window
    std::deque<EventId> m_rtxEvents; //!< packet retransmission event bitmap window
    uint32_t m_baseSeq = 1; //!< bitmap window base sequence i.e. number of index 0
  } m_irn; //!< IRN infomation

  class Dcqcn
  {
  public:
    Dcqcn ();

    /**
     * set m_devDataRate for the qp
     * \param r the datarate
     */
    void SetDevDataRate (DataRate r);

    /**
     * get m_rate of this qp
     * \return m_rate
     */
    DataRate GetRate ();

    /**
     * call after rate change
     */
    void ChangeRate ();

  public:
    DataRate m_rate;
    DataRate m_targetRate; //< Target rate
    EventId m_eventUpdateAlpha;
    double m_alpha;
    bool m_alpha_cnp_arrived; // indicate if CNP arrived in the last slot
    bool m_first_cnp; // indicate if the current CNP is the first CNP
    EventId m_eventDecreaseRate;
    bool m_decrease_cnp_arrived; // indicate if CNP arrived in the last slot
    uint32_t m_rpTimeStage;
    EventId m_rpTimer;

    DataRate m_devDataRate;
    Time m_nextAvail; // Soonest time of next send
    uint32_t m_lastPktSize; //last packet size that has been sent
  } m_dcqcn;

  class B20_B2N
  {
  public:
    void Acknowledge (uint64_t ack);

  public:
    uint64_t snd_nxt = 0;
    uint64_t snd_una = 0; // next seq to send, the highest unacked seq
  } m_b20_b2n;
};

} // namespace ns3

#endif /* RDMA_TX_QUEUE_PAIR_H */
