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

#include "rdma-tx-queue-pair.h"

#include "ns3/hash.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RdmaTxQueuePair");

NS_OBJECT_ENSURE_REGISTERED (RdmaTxQueuePair);

TypeId
RdmaTxQueuePair::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RdmaTxQueuePair")
                          .SetParent<Object> ()
                          .SetGroupName ("Rdma")
                          .AddConstructor<RdmaTxQueuePair> ();
  return tid;
}

RdmaTxQueuePair::RdmaTxQueuePair ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

RdmaTxQueuePair::RdmaTxQueuePair (Time startTime, Ipv4Address sIp, Ipv4Address dIp, uint16_t sPort,
                                  uint16_t dPort, uint64_t size, uint16_t priority)
    : m_startTime (startTime),
      m_sIp (sIp),
      m_dIp (dIp),
      m_sPort (sPort),
      m_dPort (dPort),
      m_size (size),
      m_priority (priority),
      m_sentSize (0)
{
  NS_LOG_FUNCTION (this << startTime << sIp << dIp << sPort << dPort << size << priority);
}

uint64_t
RdmaTxQueuePair::GetRemainBytes ()
{
  return (m_size >= m_sentSize) ? m_size - m_sentSize : 0;
}

uint32_t
RdmaTxQueuePair::GetHash (void)
{
  union {
    struct
    {
      uint32_t sIp;
      uint32_t dIp;
      uint16_t sPort;
      uint16_t dPort;
    };
    char bytes[12];
  } buf;
  buf.sIp = m_sIp.Get ();
  buf.dIp = m_dIp.Get ();
  buf.sPort = m_sPort;
  buf.dPort = m_dPort;
  return Hash32 (buf.bytes, sizeof (buf));
}

uint32_t
RdmaTxQueuePair::GetHash (const Ipv4Address &sIp, const Ipv4Address &dIp, const uint16_t &sPort,
                          const uint16_t &dPort)
{
  union {
    struct
    {
      uint32_t sIp;
      uint32_t dIp;
      uint16_t sPort;
      uint16_t dPort;
    };
    char bytes[12];
  } buf;
  buf.sIp = dIp.Get ();
  buf.dIp = sIp.Get ();
  buf.sPort = dPort;
  buf.dPort = sPort;
  return Hash32 (buf.bytes, sizeof (buf));
}

bool
RdmaTxQueuePair::IsFinished ()
{
  return m_sentSize >= m_size;
}

} // namespace ns3
