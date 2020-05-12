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

#include "pfc-switch-port.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/dpsk-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PfcSwitchPort");

NS_OBJECT_ENSURE_REGISTERED (PfcSwitchPort);

TypeId
PfcSwitchPort::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PfcSwitchPort")
                          .SetParent<Object> ()
                          .SetGroupName ("PfcSwitchPort")
                          .AddConstructor<PfcSwitchPort> ();
  return tid;
}

PfcSwitchPort::PfcSwitchPort ()
{
  NS_LOG_FUNCTION (this);
}

PfcSwitchPort::~PfcSwitchPort ()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Packet>
PfcSwitchPort::Tx ()
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Packet> p = Dequeue ();

  if (p == 0)
    return 0;

  // TODO cyq: Add L2 header with protocol by tag
  // TODO cyq: notify dequeue

  return p;
}

bool
PfcSwitchPort::Rx (Ptr<Packet> p)
{
  NS_LOG_FUNCTION_NOARGS ();

  // TODO cyq: Pop L2 header
  /**
   * if (is PFC)
   *   if (is Pause)
   *     update paused state or control paused
   *   if (is Resume)
   *     update paused state or control paused
   *     do Resume process
   *   trace PFC
   * else
   *   send to upper layer
   */

  return true;
}

Ptr<Packet>
PfcSwitchPort::DequeueRoundRobin ()
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_nInQueuePackets == 0 || m_nQueues == 0)
    {
      NS_LOG_LOGIC ("Queues empty");
      return 0;
    }
  else
    {
      for (uint32_t i = 0; i < m_nQueues; i++)
        {
          uint32_t qIdx = (m_lastQueueIdx + i) % m_nQueues;
          if (m_pausedStates[qIdx] == false && m_inQueueNPacketsList[qIdx] > 0)
            {
              Ptr<Packet> p = m_queues[qIdx].front ();
              m_queues[qIdx].pop ();

              m_nInQueueBytes -= p->GetSize ();
              m_nInQueuePackets--;

              m_inQueueNBytesList[qIdx] -= p->GetSize ();
              m_inQueueNPacketsList[qIdx]--;

              m_lastQueueIdx = qIdx;

              return p;
            }
        }
      return 0;
    }
}

Ptr<Packet>
PfcSwitchPort::Dequeue ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_nInControlQueuePackets == 0 || m_controlPaused)
    {
      return DequeueRoundRobin ();
    }
  else
    {
      Ptr<Packet> p = m_controlQueue.front ();
      m_controlQueue.pop ();

      m_nInControlQueueBytes -= p->GetSize ();
      m_nInControlQueuePackets--;

      return p;
    }
}

void
PfcSwitchPort::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  while (m_controlQueue.empty () == false)
    m_controlQueue.pop ();
  m_queues.clear ();
  DpskNetDeviceImpl::DoDispose ();
}

} // namespace ns3
