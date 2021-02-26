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

#ifndef PFC_HOST_PORT_H
#define PFC_HOST_PORT_H

#include "ns3/dpsk-net-device-impl.h"
#include "ns3/rdma-tx-queue-pair.h"
#include "ns3/rdma-rx-queue-pair.h"
#include "ns3/traced-callback.h"
#include "pfc-header.h"
#include "pfc-host.h"
#include <vector>
#include <queue>
#include <map>

namespace ns3 {

/**
 * \ingroup pfc
 * \class PfcHostPort
 * \brief The Priority Flow Control Net Device Logic Implementation.
 */
class PfcHostPort : public DpskNetDeviceImpl
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * Constructor
   */
  PfcHostPort ();

  /**
   * Destructor
   */
  virtual ~PfcHostPort ();

  /**
   * Setup queues.
   *
   * \param n queue number
   */
  void SetupQueues (uint32_t n);

  /**
   * Clean up queues.
   */
  void CleanQueues ();

  /**
   * Enable or disable PFC.
   * 
   * \param flag PFC enable flag
   */
  void EnablePfc (bool flag);

  /**
   * Enable or disable DCQCN
   * \param flag DCQCN enable flag
   */
  void EnableDcqcn (bool flag);

  /**
   * Add RDMA queue pair for transmitting
   *
   * \param qp queue pair to send
   */
  void AddRdmaTxQueuePair (Ptr<RdmaTxQueuePair> qp);

  /**
   * Get RDMA queue pair for transmitting
   *
   * \return queue pairs to send
   */
  std::vector<Ptr<RdmaTxQueuePair>> GetRdmaTxQueuePairs ();

  /**
   * Get RDMA queue pair for receiving
   *
   * \return queue pairs to receive
   */
  std::map<uint32_t, Ptr<RdmaRxQueuePair>> GetRdmaRxQueuePairs ();

  /**
   * L2 retransmission mode
   */
  enum L2_RTX_MODE {
    NONE, // No retransmission
    B20, // Back to zero (Not implemented)
    B2N, // Back to N (Not implemented)
    IRN // Selective ACK
  };

  /**
   * Set L2 retransmission mode
   *
   * \param mode L2 retransmission mode
   */
  void SetL2RetransmissionMode (uint32_t mode);

  /**
   * Convert L2 retransmission mode string to number
   *
   * \param mode L2 retransmission mode name
   * \return mode number
   */
  static uint32_t L2RtxModeStringToNum (const std::string &mode);

  // IRN related configurations

  /**
   * Setup IRN configurations
   *
   * \param size bitmap size
   * \param rtoh timeout high
   * \param rtol timeout low
   * \param n timeout low threshold packets of a flow
   */
  void SetupIrn (uint32_t size, Time rtoh, Time rtol, uint32_t n);

protected:
  /**
   * PFC host port transmitting logic.
   *
   * \return Ptr to the packet.
   */
  virtual Ptr<Packet> Transmit ();

  /**
   * \param packet packet sent from above down to Network Device
   * \param source source mac address (so called "MAC spoofing")
   * \param dest mac address of the destination (already resolved)
   * \param protocolNumber identifies the type of payload contained in
   *        this packet. Used to call the right L3Protocol when the packet
   *        is received.
   *
   *  Called by Host Node.
   *
   * \return whether the Send operation succeeded
   */
  virtual bool Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                     uint16_t protocolNumber);

  /**
   * PFC host port receiving logic.
   *
   * \param p Ptr to the received packet.
   * \return whether need to forward up.
   */
  virtual bool Receive (Ptr<Packet> p);

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

  // Admission to PfcHost
  friend class PfcHost;

private:
  uint32_t m_nQueues; //!< queue count of the port (control queue not included)
  bool m_pfcEnabled; //!< PFC enabled
  bool m_dcqcnEnabled; //!< DCQCN enable
  bool m_sendCnpWithAck; //!send cnp and ack in one packet

  std::vector<bool> m_pausedStates; //!< paused state of queues

  std::queue<Ptr<Packet>> m_controlQueue; //!< control queue

  std::map<uint32_t, uint32_t> m_txQueuePairTable; //!< hash and transmitted queue pairs
  std::vector<Ptr<RdmaTxQueuePair>> m_txQueuePairs; //!< transmit queue pair vector for round-robin
  std::map<uint32_t, Ptr<RdmaRxQueuePair>> m_rxQueuePairs; //!< hash and received queue pairs

  std::deque<std::pair<Ptr<RdmaTxQueuePair>, uint32_t>>
      m_rtxPacketQueue; //!< packets that need to be retransmitted, qp with seq number

  uint32_t m_lastQpIndex; //!< last transmitted queue pair index (for round-robin)

  uint32_t m_l2RetransmissionMode; //!< L2 retransmission mode

  struct
  {
    uint32_t maxBitmapSize; //!< Maximum bitmap size
    Time rtoHigh; //!< Retransmission timeout high
    Time rtoLow; //!< Retransmission timeout low
    uint32_t rtoLowThreshold; //!< Retransmission timeout low threshold
  } m_irn; //!< IRN configuration

  struct
  {
    double m_nack_interval;
    uint32_t m_chunk;
    uint32_t m_ack_interval; //if = 0 disable ack
    bool m_backto0;
  } m_b20_b2n;

  struct
  {
    DataRate m_minRate = 100; //< Min sending rate
    double m_g = 1.0 / 16; //feedback weight
    double m_rateOnFirstCNP = 1.0; // the fraction of line rate to set on first CNP
    bool m_EcnClampTgtRate = false;
    double m_rpgTimeReset = 1500.0;
    double m_rateDecreaseInterval = 4.0;
    uint32_t m_rpgThreshold = 5;
    double m_alpha_resume_interval = 55.0;
    DataRate m_rai = DataRate ("5Mb/s"); //< Rate of additive increase
    DataRate m_rhai = DataRate ("50Mb/s"); //< Rate of hyper-additive increase

    EventId m_nextSend; //< The next send event(RP parameters)
    Time interframeGap = Seconds (0.0);
  } m_dcqcn; //!<DCQCN configuration

  /**
   * every fixed time slot, update alpha.
   */
  void UpdateAlphaMlx (Ptr<RdmaTxQueuePair> qp);
  void ScheduleUpdateAlphaMlx (Ptr<RdmaTxQueuePair> qp);

  /** 
   *  receive a CNP packet call this function
   */
  void CnpReceived (Ptr<RdmaTxQueuePair> qp);

  /**
   *  Mellanox's version of rate decrease
   *  It checks every m_rateDecreaseInterval if CNP arrived (m_decrease_cnp_arrived).
   *  If so, decrease rate, and reset all rate increase related things
   */
  void CheckRateDecreaseMlx (Ptr<RdmaTxQueuePair> qp);
  void ScheduleDecreaseRateMlx (Ptr<RdmaTxQueuePair> qp, uint32_t delta);

  /**
   *  Mellanox's version of rate increase
   */
  void RateIncEventTimerMlx (Ptr<RdmaTxQueuePair> qp);
  void RateIncEventMlx (Ptr<RdmaTxQueuePair> qp);
  void FastRecoveryMlx (Ptr<RdmaTxQueuePair> qp);
  void ActiveIncreaseMlx (Ptr<RdmaTxQueuePair> qp);
  void HyperIncreaseMlx (Ptr<RdmaTxQueuePair> qp);

  /**
   * update qp next send time after send a new packet of qp
   */
  void UpdateNextAvail (Ptr<RdmaTxQueuePair> qp, uint32_t pkt_size);

  /**
   * update send event after receive a packet
   * \param qp received flow
   */
  void UpdateNextSend (Ptr<RdmaTxQueuePair> qp);

  /**
   * call this function after changing datarate
   */
  void ChangeRate (Ptr<RdmaTxQueuePair> qp, DataRate old_rate);

  /**
   * Generate data packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \return data packet
   */
  Ptr<Packet> GenData (Ptr<RdmaTxQueuePair> qp);

  /**
   * Generate data packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \param o_irnSeq output sequence number
   * \return data packet
   */
  Ptr<Packet> GenData (Ptr<RdmaTxQueuePair> qp, uint32_t &o_irnSeq);

  /**
   * Generate data packet that needs to be retransmitted
   * 
   * \param qp queue pair
   * \param irnSeq packet seq
   * \param size packet size
   * \return data packet
   */
  Ptr<Packet> ReGenData (Ptr<RdmaTxQueuePair> qp, uint32_t irnSeq, uint32_t size);

  /**
   * Generate ACK packet of target transmitting queue pair for B20 B2N
   *
   * \param qp queue pair
   * \param seq ack sequence number of this packet
   * \param ack if ture send ack else send nack
   * \return ACK packet
   */
  Ptr<Packet> GenACK (Ptr<RdmaRxQueuePair> qp, uint32_t seq, bool ack);
  /**
   * Generate ACK packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \param irnAck ack sequence number of this packet
   * \return ACK packet
   */
  Ptr<Packet> GenACK (Ptr<RdmaRxQueuePair> qp, uint32_t irnAck);

  /**
   * Generate SACK packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \param irnAck ack sequence number of this packet
   * \param irnNack cumulative acknowledgment (expected sequence number)
   * \return SACK packet
   */
  Ptr<Packet> GenSACK (Ptr<RdmaRxQueuePair> qp, uint32_t irnAck, uint32_t irnNack);

  /**
   * Schedule IRN retransmission timer for each packet of one queue pair
   * 
   * \param qp queue pair
   * \param irnSeq sequence number of this data packet
   * \return NS3 event id
   */
  EventId IrnTimer (Ptr<RdmaTxQueuePair> qp, uint32_t irnSeq);

  /**
   * Generate IRN retransmission packet to retransmission queue
   * 
   * \param qp queue pair
   * \param irnSeq sequence number of this data packet
   */
  void IrnTimerHandler (Ptr<RdmaTxQueuePair> qp, uint32_t irnSeq);

  /**
   * The trace source fired for received a PFC packet.
   *
   * \param Ptr Dpsk net device
   * \param uint32_t target queue index
   * \param PfcType PFC type
   * \param uint16_t pause time
   */
  TracedCallback<Ptr<DpskNetDevice>, uint32_t, PfcHeader::PfcType, uint16_t> m_pfcRxTrace;

  /**
   * The trace source fired for completing sending a queue pair.
   *
   * \param Ptr pointer of RdmaTxQueuePair
   */
  TracedCallback<Ptr<RdmaTxQueuePair>> m_queuePairTxCompleteTrace;

  /**
   * The trace source fired for completing receiving a queue pair.
   *
   * \param Ptr pointer of RdmaRxQueuePair
   */
  TracedCallback<Ptr<RdmaRxQueuePair>> m_queuePairRxCompleteTrace;

public:
  /// Statistics

  uint64_t m_nTxBytes; //!< total transmit bytes
  uint64_t m_nRxBytes; //!< total receive bytes

private:
  /**
   * Disabled method.
   */
  PfcHostPort &operator= (const PfcHostPort &o);

  /**
   * Disabled method.
   */
  PfcHostPort (const PfcHostPort &o);
};

} // namespace ns3

#endif /* PFC_HOST_PORT_H */
