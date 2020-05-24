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

#include "pfc-host-port.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/pointer.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/pfc-header.h"
#include "ns3/udp-header.h"
#include "ns3/dpsk-net-device.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PfcHostPort");

NS_OBJECT_ENSURE_REGISTERED (PfcHostPort);

TypeId
PfcHostPort::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::PfcHostPort")
          .SetParent<Object> ()
          .SetGroupName ("Pfc")
          .AddConstructor<PfcHostPort> ()
          .AddTraceSource ("PfcRx", "Receive a PFC packet",
                           MakeTraceSourceAccessor (&PfcHostPort::m_pfcRxTrace),
                           "PfcHeader::PfcType, uint32_t, std::vector<bool>")
          .AddTraceSource ("QueuePairTxComplete", "Completing sending a queue pair",
                           MakeTraceSourceAccessor (&PfcHostPort::m_queuePairTxCompleteTrace),
                           "Ptr<RdmaTxQueuePair>")
          .AddTraceSource ("QueuePairRxComplete", "Completing receiving a queue pair",
                           MakeTraceSourceAccessor (&PfcHostPort::m_queuePairRxCompleteTrace),
                           "Ptr<RdmaRxQueuePair>");
  return tid;
}

PfcHostPort::PfcHostPort ()
{
  NS_LOG_FUNCTION (this);
}

PfcHostPort::~PfcHostPort ()
{
  NS_LOG_FUNCTION (this);
}

void
PfcHostPort::SetupQueues (uint32_t n)
{
  NS_LOG_FUNCTION (n);
  CleanQueues ();
  m_nQueues = n;
  for (uint32_t i = 0; i < m_nQueues; i++)
    {
      m_pausedStates.push_back (false);
    }
}

void
PfcHostPort::CleanQueues ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_nQueues = 0;
  m_pausedStates.clear ();
}

void
PfcHostPort::AddRdmaTxQueuePair (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);
  m_txQueuePairs.push_back (qp);
  Simulator::Schedule (qp->m_startTime, &DpskNetDevice::TriggerTransmit, m_dev);
}

std::vector<Ptr<RdmaTxQueuePair>>
PfcHostPort::GetRdmaTxQueuePairs ()
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_txQueuePairs;
}

void
PfcHostPort::AddRdmaRxQueuePair (Ptr<RdmaRxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);
  uint32_t key = qp->GetHash ();
  m_rxQueuePairs[key] = qp;
}

std::map<uint32_t, Ptr<RdmaRxQueuePair>>
PfcHostPort::GetRdmaRxQueuePairs ()
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_rxQueuePairs;
}

Ptr<Packet>
PfcHostPort::Transmit ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if (m_pausedStates[m_nQueues] == false &&
      m_controlQueue.empty () == false) // can send control packets
    {
      Ptr<Packet> p = m_controlQueue.front ();
      m_controlQueue.pop ();
      return p;
    }

  uint32_t flowCnt = m_txQueuePairs.size ();
  for (uint32_t i = 0; i < flowCnt; i++)
    {
      uint32_t qIdx = (m_lastQpIndex + i) % flowCnt;
      auto qp = m_txQueuePairs[qIdx];
      if (m_pausedStates[qp->m_priority] == false && qp->IsFinished () == false)
        {
          m_lastQpIndex = qIdx;
          auto p = GenData (qp);
          if (qp->IsFinished ())
            m_queuePairTxCompleteTrace (qp);
          return p;
        }
    }

  // No packet to send
  return 0;
}

bool
PfcHostPort::Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                   uint16_t protocolNumber)
{
  // cyq: No active send now
  return false;
}

bool
PfcHostPort::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (p);

  // Pop Ethernet header
  EthernetHeader ethHeader;
  p->RemoveHeader (ethHeader);

  if (ethHeader.GetLengthType () == PfcHeader::PROT_NUM) // PFC protocol number
    {
      // Pop PFC header
      PfcHeader pfcHeader;
      p->RemoveHeader (pfcHeader);

      if (pfcHeader.GetType () == PfcHeader::Pause) // PFC Pause
        {
          // Update paused state
          uint32_t pfcQIndex = pfcHeader.GetQIndex ();
          uint32_t qIndex = (pfcQIndex >= m_nQueues) ? m_nQueues : pfcQIndex;
          m_pausedStates[qIndex] = true;
          m_pfcRxTrace (PfcHeader::Pause, qIndex, m_pausedStates);
          return false; // Do not forward up to node
        }
      else if (pfcHeader.GetType () == PfcHeader::Resume) // PFC Resume
        {
          // Update paused state
          uint32_t pfcQIndex = pfcHeader.GetQIndex ();
          uint32_t qIndex = (pfcQIndex >= m_nQueues) ? m_nQueues : pfcQIndex;
          m_pausedStates[qIndex] = false;
          m_pfcRxTrace (PfcHeader::Resume, qIndex, m_pausedStates);
          m_dev->TriggerTransmit (); // Trigger device transmitting
          return false; // Do not forward up to node
        }
      else
        {
          NS_ASSERT_MSG (false, "PfcSwitchPort::Rx: Invalid PFC type");
          return false; // Drop this packet
        }
    }
  else // Not PFC
    {
      Ipv4Header ip;
      UdpHeader udp;
      p->RemoveHeader (ip);
      p->RemoveHeader (udp);
      auto sIp = ip.GetSource ();
      auto dIp = ip.GetDestination ();
      uint16_t sPort = udp.GetSourcePort ();
      uint16_t dPort = udp.GetDestinationPort ();
      uint16_t dscp = ip.GetDscp ();
      uint32_t payloadSize = p->GetSize ();

      uint32_t key = RdmaRxQueuePair::GetHash (sIp, dIp, sPort, dPort);
      auto qpItr = m_rxQueuePairs.find (key);
      Ptr<RdmaRxQueuePair> qp;
      if (qpItr == m_rxQueuePairs.end ()) // new flow
        {
          // cyq: init rx qp with definite size
          qp = CreateObject<RdmaRxQueuePair> (sIp, dIp, sPort, dPort, UINT64_MAX, dscp);
          qp->m_receivedSize += payloadSize;
          m_rxQueuePairs.insert ({key, qp});
        }
      else // existed flow
        {
          qp = qpItr->second;
          qp->m_receivedSize += payloadSize;
        }
      if (qp->IsFinished ())
        m_queuePairRxCompleteTrace (qp);
      return true; // Forward up to node
    }
}

Ptr<Packet>
PfcHostPort::GenData (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);

  const uint32_t remainSize = qp->GetRemainBytes ();
  const uint32_t mtu = m_dev->GetMtu ();
  uint32_t payloadSize = (remainSize > mtu) ? mtu : remainSize;

  qp->m_sentSize += payloadSize;

  Ptr<Packet> p = Create<Packet> (payloadSize);

  UdpHeader udp;
  udp.SetSourcePort (qp->m_sPort);
  udp.SetDestinationPort (qp->m_dPort);
  p->AddHeader (udp);

  Ipv4Header ip;
  ip.SetSource (qp->m_sIp);
  ip.SetDestination (qp->m_dIp);
  ip.SetProtocol (0x11); // UDP
  ip.SetPayloadSize (p->GetSize ());
  ip.SetTtl (64);
  ip.SetDscp (Ipv4Header::DscpType (qp->m_priority));
  p->AddHeader (ip);

  EthernetHeader eth;
  eth.SetSource (Mac48Address::ConvertFrom (m_dev->GetAddress ()));
  eth.SetDestination (Mac48Address::ConvertFrom (m_dev->GetRemote ()));
  eth.SetLengthType (0x0800); // IPv4
  p->AddHeader (eth);

  return p;
}

void
PfcHostPort::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_pausedStates.clear ();
  while (m_controlQueue.empty () == false)
    m_controlQueue.pop ();
  m_txQueuePairs.clear ();
  m_rxQueuePairs.clear ();
  DpskNetDeviceImpl::DoDispose ();
}

} // namespace ns3
