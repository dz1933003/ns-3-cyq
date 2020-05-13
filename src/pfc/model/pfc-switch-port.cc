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
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/pfc-header.h"
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
PfcSwitchPort::Transmit ()
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Packet> p = Dequeue ();

  if (p == 0)
    return 0;

  // TODO cyq: notify switch dequeue event (dev, queue)

  return p;
}

bool
PfcSwitchPort::Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                     uint16_t protocolNumber)
{
  // Assembly Ethernet header
  EthernetHeader ethHeader;
  ethHeader.SetSource (Mac48Address::ConvertFrom (source));
  ethHeader.SetDestination (Mac48Address::ConvertFrom (dest));
  ethHeader.SetLengthType (protocolNumber);

  if (protocolNumber == 0x8808) // PFC protocol number
    {
      // Add Ethernet header
      packet->AddHeader (ethHeader);
      // Enqueue control queue
      m_controlQueue.push (packet);
    }
  else // Not PFC
    {
      // Get queue index
      Ipv4Header ipHeader;
      packet->PeekHeader (ipHeader);
      auto dscp = ipHeader.GetDscp ();
      // Add Ethernet header
      packet->AddHeader (ethHeader);
      // Enqueue
      m_queues[dscp].push (packet);
    }

  return true; // Enqueue packet successfully
}

bool
PfcSwitchPort::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Pop Ethernet header
  EthernetHeader ethHeader;
  p->RemoveHeader (ethHeader);

  if (ethHeader.GetLengthType () == 0x8808) // PFC protocol number
    {
      // Pop PFC header
      PfcHeader pfcHeader;
      p->RemoveHeader (pfcHeader);

      if (pfcHeader.GetType () == PfcHeader::Pause) // PFC Pause
        {
          // Update paused state
          if (pfcHeader.GetQIndex () >= m_nQueues)
            {
              m_controlPaused = true;
            }
          else
            {
              m_pausedStates[pfcHeader.GetQIndex ()] = true;
            }
          return false; // Do not forward up to node
        }
      else if (pfcHeader.GetType () == PfcHeader::Resume) // PFC Resume
        {
          // Update paused state
          if (pfcHeader.GetQIndex () >= m_nQueues)
            {
              m_controlPaused = false;
            }
          else
            {
              m_pausedStates[pfcHeader.GetQIndex ()] = false;
            }
          m_dev->TriggerTransmit (); // Trigger device transmitting
          return false; // Do not forward up to node
        }
      else
        {
          NS_ASSERT_MSG (false, "PfcSwitchPort::Rx: Invalid PFC type");
          return false; // Drop this packet
        }
      // TODO cyq: trace PFC
    }
  else // Not PFC
    {
      return true; // Forward up to node
    }
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
