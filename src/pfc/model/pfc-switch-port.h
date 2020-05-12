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
#include <vector>
#include <queue>

namespace ns3 {

/**
 * \ingroup pfc
 * \class PfcSwitchPort
 * \brief The Priority Flow Control Net Device Logic Implementation.
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

protected:
  /**
   * PFC switch port transmitting logic.
   *
   * \return Ptr to the packet.
   */
  virtual Ptr<Packet> Tx ();

  /**
   * PFC switch port receiving logic.
   *
   * \param p Ptr to the received packet.
   * \return whether need to forward up.
   */
  virtual bool Rx (Ptr<Packet> p);

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:
  /**
   * Dequeue by round robin for queues of the port
   * except the control queue.
   *
   * \return Ptr to the dequeued packet
   */
  Ptr<Packet> DequeueRoundRobin ();

  /**
   * Dequeue a packet for queues with the control queue.
   *
   * \return Ptr to the dequeued packet
   */
  Ptr<Packet> Dequeue ();

  uint32_t m_nQueues; //!< queue count of the port (control queue not included)
  std::queue<Ptr<Packet>> m_controlQueue; //!< highest priority queue
  std::vector<std::queue<Ptr<Packet>>> m_queues; //!< queues of the port

  bool m_controlPaused; //!< paused state of the control queue
  std::vector<bool> m_pausedStates; //!< paused state of queues

  uint32_t m_lastQueueIdx; //!< last dequeue queue index

public:
  /// Statistics

  uint64_t m_nInControlQueueBytes; //!< control queue in-queue byte
  uint64_t m_nInQueueBytes; //!< total in-queue byte (control queue excluded)
  std::vector<uint64_t> m_inQueueNBytesList; //!< other in-queue byte

  uint32_t m_nInControlQueuePackets; //!< control queue in-queue packet count
  uint32_t m_nInQueuePackets; //!< total in-queue packet count (control queue excluded)
  std::vector<uint32_t> m_inQueueNPacketsList; //!< other in-queue packet count

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
