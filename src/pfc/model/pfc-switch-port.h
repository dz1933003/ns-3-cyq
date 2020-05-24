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

#ifndef PFC_SWITCH_PORT_H
#define PFC_SWITCH_PORT_H

#include "ns3/dpsk-net-device-impl.h"
#include "ns3/traced-callback.h"
#include "ns3/pfc-header.h"
#include <vector>
#include <queue>

namespace ns3 {

/**
 * \ingroup pfc
 * \class PfcSwitchPort
 * \brief The Priority Flow Control Net Device Logic Implementation.
 *
 * Attention: No data packet modify on the port when receive (for mmu statics). Only
 * handle PFC frames.
 * Add Ethenet header when transmit.
 */
class PfcSwitchPort : public DpskNetDeviceImpl
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
  PfcSwitchPort ();

  /**
   * Destructor
   */
  virtual ~PfcSwitchPort ();

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
   * Transmit callback to notify mmu
   *
   * \param output device
   * \param output packet
   * \param output queue index
   * \return void
   */
  typedef Callback<void, Ptr<NetDevice>, Ptr<Packet>, uint32_t> DeviceDequeueNotifier;

  /**
   * Setup dequeue event notifier
   *
   * \param h callback
   */
  void SetDeviceDequeueHandler (DeviceDequeueNotifier h);

protected:
  /**
   * PFC switch port transmitting logic.
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
   *  Called by Switch Node.
   *
   * \return whether the Send operation succeeded
   */
  virtual bool Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                     uint16_t protocolNumber);

  /**
   * PFC switch port receiving logic.
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
  /**
   * Dequeue by round robin for queues of the port
   * except the control queue.
   *
   * \param qIndex dequeue queue index (output)
   * \return Ptr to the dequeued packet
   */
  Ptr<Packet> DequeueRoundRobin (uint32_t &qIndex);

  /**
   * Dequeue a packet for queues with the control queue.
   *
   * \param qIndex dequeue queue index (output)
   * \return Ptr to the dequeued packet
   */
  Ptr<Packet> Dequeue (uint32_t &qIndex);

  uint32_t m_nQueues; //!< queue count of the port (control queue not included)
  std::vector<std::queue<Ptr<Packet>>> m_queues; //!< queues of the port (with control queue)

  std::vector<bool> m_pausedStates; //!< paused state of queues

  uint32_t m_lastQueueIdx; //!< last dequeue queue index (control queue excluded)

  DeviceDequeueNotifier m_mmuCallback; //!< callback to notify mmu

  /**
   * The trace source fired for received a PFC packet.
   *
   * \param PfcType PFC type
   * \param uint32_t target queue index
   * \param vector pause state after received
   */
  TracedCallback<PfcHeader::PfcType, uint32_t, std::vector<bool>> m_pfcRxTrace;

public:
  /// Statistics

  uint64_t m_nInQueueBytes; //!< total in-queue bytes (control queue included)
  std::vector<uint64_t> m_inQueueBytesList; //!< in-queue bytes in every queue

  uint32_t m_nInQueuePackets; //!< total in-queue packet count (control queue included)
  std::vector<uint32_t> m_inQueuePacketsList; //!< in-queue packet count in every queue

  uint64_t m_nTxBytes; //!< total transmit bytes
  uint64_t m_nRxBytes; //!< total receive bytes

private:
  /**
   * Disabled method.
   */
  PfcSwitchPort &operator= (const PfcSwitchPort &o);

  /**
   * Disabled method.
   */
  PfcSwitchPort (const PfcSwitchPort &o);
};

} // namespace ns3

#endif /* PFC_SWITCH_PORT_H */
