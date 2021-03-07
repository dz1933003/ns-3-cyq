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
    NONE_RTX, // No retransmission
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

  /**
   * Setup IRN configurations
   *
   * \param size bitmap size
   * \param rtoh timeout high
   * \param rtol timeout low
   * \param n timeout low threshold packets of a flow
   */
  void SetupIrn (uint32_t size, Time rtoh, Time rtol, uint32_t n);

  /**
   * Setup B2N or B20 configurations
   * 
   * \param chunk chunk size
   * \param ackInterval ACK interval bytes
   * \param nackInterval NACK interval time
   */
  void SetupB2x (uint32_t chunk, uint32_t ackInterval, Time nackInterval);

  /**
   * Congestion control mode
   */
  enum CC_MODE {
    NONE_CC, // No cc
    DCQCN // Dcqcn
  };

  /**
   * Set congestion control mode
   * 
   * \param mode congestion control mode
   */
  void SetCcMode (uint32_t mode);

  /**
   * Convert congestion control mode string to number
   * 
   * \param mode congestion control mode name
   * \return mode number
   */
  static uint32_t CcModeStringToNum (const std::string &mode);

  struct Dcqcn
  {
    double g; //!< Control gain parameter which determines the level of rate decrease
    double rateFracOnFirstCnp; //!< the fraction of line rate to set on first CNP
    bool clampTargetRate; //!< Clamp target rate on CNP
    Time incRateInterval; //!< The rate increase interval at RP
    Time decRateInterval; //!< The interval of rate decrease check
    uint32_t fastRecTimes; //!< Fast recovery times
    Time alphaResumeInterval; //!< The interval of resuming alpha

    DataRate rai; //!< Rate increase of additive increase
    DataRate rhai; //!< Rate increase of hyper-additive increase

    DataRate minRate; //!< Min sending rate
    bool isRateBound; //!< using DCQCN rate control
  };

  /**
   * Setup DCQCN configurations
   * 
   * \param dcqcn DCQCN configuration structure
   */
  void SetupDcqcn (PfcHostPort::Dcqcn dcqcn);

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

  std::vector<bool> m_pausedStates; //!< paused state of queues

  std::queue<Ptr<Packet>> m_controlQueue; //!< control queue

  std::map<uint32_t, uint32_t> m_txQueuePairTable; //!< hash and transmitted queue pairs
  std::vector<Ptr<RdmaTxQueuePair>> m_txQueuePairs; //!< transmit queue pair vector for round-robin
  std::map<uint32_t, Ptr<RdmaRxQueuePair>> m_rxQueuePairs; //!< hash and received queue pairs

  std::vector<std::queue<uint32_t>>
      m_rtxSeqQueues; //!< packets need to be retransmitted, qp index with IRN seq
  uint32_t m_rtxQueuingCnt; //!< retransmitted packets in queue

  uint32_t m_lastQpIndex; //!< last transmitted queue pair index (for round-robin)

  uint32_t m_l2RetransmissionMode; //!< L2 retransmission mode

  uint32_t m_ccMode; //!< congestion control mode

  EventId m_nextTransmitEvent; //< device next send event

  struct //!< IRN configuration
  {
    uint32_t maxBitmapSize; //!< Maximum bitmap size
    Time rtoHigh; //!< Retransmission timeout high
    Time rtoLow; //!< Retransmission timeout low
    uint32_t rtoLowThreshold; //!< Retransmission timeout low threshold
  } m_irn;

  Dcqcn m_dcqcn; //!< DCQCN configuration

  struct B2x //!< B2N or B20 configuration
  {
    uint32_t chunk; //!< Chunk size by byte
    uint32_t ackInterval; //!< ACK generate interval bytes
    Time nackInterval; //!< NACK generate interval time
  } m_b2x;

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
   * Generate ACK packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \param seq sequence number
   * \param irnAck ack sequence number of this packet
   * \param cnp whether tag CNP flag
   * \return ACK packet
   */
  Ptr<Packet> GenACK (Ptr<RdmaRxQueuePair> qp, uint32_t seq, uint32_t irnAck, bool cnp);

  /**
   * Generate SACK packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \param seq sequence number
   * \param irnAck ack sequence number of this packet
   * \param irnNack cumulative acknowledgment (expected sequence number)
   * \param cnp whether tag CNP flag
   * \return SACK packet
   */
  Ptr<Packet> GenSACK (Ptr<RdmaRxQueuePair> qp, uint32_t seq, uint32_t irnAck, uint32_t irnNack,
                       bool cnp);

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
   * Update next packet available time for the queue pair
   * 
   * \param qp queue pair
   * \param interframeGap net device interframe gap time
   * \param size last transmitted packet size
   */
  void UpdateNextAvail (Ptr<RdmaTxQueuePair> qp, const Time &interframeGap, const uint32_t &size);

  /**
   * DCQCN CNP receive handler
   * 
   * \param qp queue pair
   */
  void DcqcnCnpReceived (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN update alpha with scheduling
   * 
   * \param qp queue pair
   */
  void DcqcnUpdateAlpha (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN schedule next alpha update
   * 
   * \param qp queue pair
   */
  void DcqcnScheduleUpdateAlpha (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN decrease rate with scheduling.
   * It checks every decrease interval if CNP arrived (decrease CNP arrived).
   * If so, decrease rate, and reset all rate increase related things.
   * 
   * \param qp queue pair
   */
  void DcqcnDecRate (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN schedule next rate decrease
   * 
   * \param qp queue pair
   * \param delta scheduling delay time
   */
  void DcqcnScheduleDecRate (Ptr<RdmaTxQueuePair> qp, const Time &delta);

  /**
   * DCQCN schedule next rate increase
   * 
   * \param qp queue pair
   */
  void DcqcnScheduleIncRate (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN increase rate
   * 
   * \param qp queue pair
   */
  void DcqcnIncRate (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN fast recovery of increase rate
   * 
   * \param qp queue pair
   */
  void DcqcnFastRecovery (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN active increase rate
   * 
   * \param qp queue pair
   */
  void DcqcnActiveIncrease (Ptr<RdmaTxQueuePair> qp);

  /**
   * DCQCN hyper increase rate
   * 
   * \param qp queue pair
   */
  void DcqcnHyperIncrease (Ptr<RdmaTxQueuePair> qp);

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

  uint64_t m_irnRtxBytes; //!< IRN retransmission bytes

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
