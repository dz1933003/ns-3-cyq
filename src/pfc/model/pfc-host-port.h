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
#include <vector>
#include <queue>

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

  // /**
  //  * Setup queues.
  //  *
  //  * \param n queue number
  //  */
  // void SetupQueues (uint32_t n);

  // /**
  //  * Clean up queues.
  //  */
  // void CleanQueues ();

  /**
   * Add RDMA queue pair
   *
   * \param qp queue pair to send
   */
  void AddRdmaQueuePair (Ptr<RdmaTxQueuePair> qp);

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

private:
  std::vector<bool> m_pausedStates; //!< paused state of queues
  std::vector<Ptr<RdmaTxQueuePair>> m_txQueuePairs; //!< transmit queue pairs
  std::vector<Ptr<RdmaRxQueuePair>> m_rxQueuePairs; //!< received queue pairs

public:
  /// Statistics

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
