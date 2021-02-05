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

  // IRN related configurations

  /**
   * Setup IRN configurations
   *
   * \param size bitmap size
   * \param rtoh timeout high
   * \param rtol timeout low
   */
  void SetupIrn (uint32_t size, Time rtoh, Time rtol);

  
  void TimeOutRetran(Ptr<RdmaTxQueuePair> qp, uint32_t siz, uint32_t seq);//time out retransmit function
  bool FindRetranPkg(Ptr<Packet> p);//judge if a pack has been added to RetranPkg

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

  std::vector<Ptr<RdmaTxQueuePair>> m_txQueuePairs; //!< transmit queue pairs
  std::map<uint32_t, Ptr<RdmaRxQueuePair>> m_rxQueuePairs; //!< hash and received queue pairs
  std::map<uint32_t, uint32_t> m_transmittedQP; //!< hash and transmitted queue pairs
  std::vector<Ptr<Packet>> RetranPkg;//packs that need to be retransmitted

  uint32_t m_lastQpIndex; //!< last transmitted queue pair index (for round-robin)

  uint32_t m_l2RetrasmissionMode; //!< L2 retransmission mode

  struct
  {
    uint32_t maxBitmapSize; //!< Maximum bitmap size
    Time rtoHigh; //!< Retransmission timeout high
    Time rtoLow; //!< Retransmission timeout low
  } m_irn; //!< IRN configuration

  /**
   * Generate data packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \return data packet
   */
  Ptr<Packet> GenData (Ptr<RdmaTxQueuePair> qp);

  Ptr<Packet> ReGenData (Ptr<RdmaTxQueuePair> qp, uint32_t siz, uint32_t seq);
  /**
   * Generate data packet of target transmitting queue pair in  IRN mode
   *
   * \param qp queue pair
   * \return data packet
   */
  Ptr<Packet> IRN_GenData (Ptr<RdmaTxQueuePair> qp);

  /**
   * Generate ACK or SACK packet of target transmitting queue pair
   *
   * \param qp queue pair
   * \param seq ack seq number
   * \param ack true for ACK and false for SACK
   * \return data packet
   */
  Ptr<Packet> GenACK (Ptr<RdmaRxQueuePair> qp, uint32_t seq, bool ack);

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
