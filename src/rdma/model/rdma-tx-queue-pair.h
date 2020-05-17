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

namespace ns3 {

class RdmaTxQueuePair : public Object
{
public:
  Time m_startTime;
  Ipv4Address m_sIp, m_dIp;
  uint16_t m_sPort, m_dPort;
  uint64_t m_size;
  uint16_t m_priority;

  uint64_t m_sentSize;

  static TypeId GetTypeId (void);
  RdmaTxQueuePair ();
  RdmaTxQueuePair (Time startTime, Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort, uint16_t dPort,
                   uint64_t size, uint16_t priority);

  uint64_t GetRemainBytes ();
  uint32_t GetHash (void);
  bool IsFinished ();
};

} // namespace ns3

#endif /* RDMA_TX_QUEUE_PAIR_H */
