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
#include "ns3/data-rate.h"

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
  /* Flow infomation */
  Time m_startTime; //!< queue pair arrival time
  Ipv4Address m_sIp, m_dIp; //!< source IP, dst IP
  uint16_t m_sPort, m_dPort; //!< source port, dst port
  uint64_t m_size; //!< source port, dst port
  uint16_t m_priority; //!< flow priority

  /* Transmission statics */
  uint64_t m_txSize; //!< total valid sent size (next seq to send)

  /* B2N & B20 */
  uint64_t m_unackSize; //!< the highest unacked seq for B2N or B20

  /* RDMA window */
  bool m_isVarWin; //!< whether enabled variable window size for B2N or B20
  uint32_t m_winSize; //!< bound of on-the-fly bytes for B2N or B20

  /* Rate limiter */
  DataRate m_maxRate; //!< queue pair max rate
  DataRate m_rate; //!< current queue pair rate
  Time m_nextAvail; //!< soonest time of next send of queue pair

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
  uint64_t GetRemainBytes () const;

  uint32_t GetHash (void);
  static uint32_t GetHash (const Ipv4Address &sIp, const Ipv4Address &dIp, const uint16_t &sPort,
                           const uint16_t &dPort);

  /**
   * Finished tx but may not all packets are acked.
   * But finished all acked with IRN.
   *
   * \return true for finished
   */
  bool IsTxFinished () const;

  /**
   * Finished with all packets are acked.
   * This is only for B2N and B20 with DCQCN.
   * 
   * \return true for finished
   */
  bool IsAckedFinished () const;

  /**
   * Setup queue pair rate limiter (now used for DCQCN only)
   * 
   * \param maxRate
   * \param initRate
   */
  void SetupRate (const DataRate &maxRate, const DataRate &initRate);

  /**
   * Setup B2N or B20 window
   * 
   * \param isVarWin whether a variable window size
   * \param winSize window size
   */
  void SetupB2x (const bool &isVarWin, const uint32_t &winSize);

  /**
   * Recover queue when receive NACK for B2N or B20
   */
  void B2xRecover ();

  /**
   * Acknowleage sequence for B2N or B20
   * 
   * \param ack ack sequence
   */
  void B2xAck (const uint64_t &ack);

  /**
   * Get on the fly bytes for B2N or B20
   * 
   * \return on the fly bytes
   */
  uint64_t B2xGetOnTheFly () const;

  /**
   * Whether B2N or B20 window is full
   * 
   * \return true if window is full
   */
  bool B2xIsWinBound () const;

  /**
   * B2N or B20 window size
   * 
   * \return window size by byte
   */
  uint64_t B2xGetWin () const;

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

  /**
   * \ingroup rdma
   * \class Dcqcn
   * \brief Rdma tx queue pair DCQCN infomation.
   */
  class Dcqcn
  {
  public:
    double m_alpha = 1;
    bool m_firstCnp = true; // indicate if the current CNP is the first CNP
    bool m_alphaCnpArrived = false; // indicate if CNP arrived in the last slot
    bool m_decreaseCnpArrived = false; // indicate if CNP arrived in the last slot
    uint32_t m_rpTimeStage = 0;

    DataRate m_targetRate; //< Target rate

    EventId m_eventUpdateAlpha;
    EventId m_eventDecreaseRate;
    EventId m_rpTimer;

    /**
     * Cleanup DCQCN rate control timer when queue pair is complete
     */
    void CleanupTimer ();
  } m_dcqcn;
};

} // namespace ns3

#endif /* RDMA_TX_QUEUE_PAIR_H */
