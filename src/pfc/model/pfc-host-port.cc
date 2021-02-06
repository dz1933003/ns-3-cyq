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
#include "ns3/qbb-header.h"
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
                           "Ptr<DpskNetDevice>, uint32_t, "
                           "PfcHeader::PfcType, uint16_t")
          .AddTraceSource ("QueuePairTxComplete", "Completing sending a queue pair",
                           MakeTraceSourceAccessor (&PfcHostPort::m_queuePairTxCompleteTrace),
                           "Ptr<RdmaTxQueuePair>")
          .AddTraceSource ("QueuePairRxComplete", "Completing receiving a queue pair",
                           MakeTraceSourceAccessor (&PfcHostPort::m_queuePairRxCompleteTrace),
                           "Ptr<RdmaRxQueuePair>");
  return tid;
}

PfcHostPort::PfcHostPort ()
    : m_pfcEnabled (true), m_l2RetrasmissionMode (L2_RTX_MODE::NONE), m_nTxBytes (0), m_nRxBytes (0)
{
  NS_LOG_FUNCTION (this);
  m_name = "PfcHostPort";
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
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      m_pausedStates.push_back (false);
    }
}

void
PfcHostPort::EnablePfc (bool flag)
{
  NS_LOG_FUNCTION (flag);
  m_pfcEnabled = flag;
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

std::map<uint32_t, Ptr<RdmaRxQueuePair>>
PfcHostPort::GetRdmaRxQueuePairs ()
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_rxQueuePairs;
}

void
PfcHostPort::SetL2RetransmissionMode (uint32_t mode)
{
  NS_LOG_FUNCTION (mode);
  m_l2RetrasmissionMode = mode;
}

void
PfcHostPort::SetupIrn (uint32_t size, Time rtoh, Time rtol)
{
  NS_LOG_FUNCTION (size << rtoh << rtol);
  m_irn.maxBitmapSize = size;
  m_irn.rtoHigh = rtoh;
  m_irn.rtoLow = rtol;
}

Ptr<Packet>
PfcHostPort::Transmit ()
{
  NS_LOG_FUNCTION_NOARGS ();

  if ((m_pausedStates[m_nQueues] == false || m_pfcEnabled == false) &&
      m_controlQueue.empty () == false) // can send control packets
    {
      Ptr<Packet> p = m_controlQueue.front ();
      m_controlQueue.pop ();
      m_nTxBytes += p->GetSize ();
      return p;
    }

  if (m_retranPkg.empty () == false)
    {
      Ptr<Packet> p = m_retranPkg[0];
      m_retranPkg.erase (m_retranPkg.begin ());
      return p;
    }

  uint32_t flowCnt = m_txQueuePairs.size ();
  for (uint32_t i = 0; i < flowCnt; i++) //TODO
    {
      uint32_t qIdx = (m_lastQpIndex + i) % flowCnt;
      auto qp = m_txQueuePairs[qIdx];
      if ((m_pausedStates[qp->m_priority] == false || m_pfcEnabled == false) &&
          qp->IsFinished () == false && qp->m_startTime <= Simulator::Now () &&
          (m_l2RetrasmissionMode != IRN || qp->m_irn.pkg_state.size () < m_irn.maxBitmapSize))
        {
          m_lastQpIndex = qIdx;
          Ptr<Packet> p;
          if (m_l2RetrasmissionMode == IRN)
            p = IRN_GenData (qp);
          else
            p = GenData (qp);
          uint32_t key = qp->GetHash ();
          if (m_transmittedQP.find (key) == m_transmittedQP.end ())
            {
              m_transmittedQP.insert ({key, qIdx});
            }
          if (qp->IsFinished ())
            m_queuePairTxCompleteTrace (qp);
          m_nTxBytes += p->GetSize ();
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
  return false;
}

bool
PfcHostPort::Receive (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (p);

  m_nRxBytes += p->GetSize ();

  // Pop Ethernet header
  EthernetHeader ethHeader;
  p->RemoveHeader (ethHeader);

  if (ethHeader.GetLengthType () == PfcHeader::PROT_NUM) // PFC protocol number
    {
      if (m_pfcEnabled == false)
        {
          return false; // Drop because PFC disabled
        }
      // Pop PFC header
      PfcHeader pfcHeader;
      p->RemoveHeader (pfcHeader);

      if (pfcHeader.GetType () == PfcHeader::Pause) // PFC Pause
        {
          // Update paused state
          uint32_t pfcQIndex = pfcHeader.GetQIndex ();
          uint32_t qIndex = (pfcQIndex >= m_nQueues) ? m_nQueues : pfcQIndex;
          m_pausedStates[qIndex] = true;
          m_pfcRxTrace (m_dev, qIndex, PfcHeader::Pause, pfcHeader.GetTime ());
          return false; // Do not forward up to node
        }
      else if (pfcHeader.GetType () == PfcHeader::Resume) // PFC Resume
        {
          // Update paused state
          uint32_t pfcQIndex = pfcHeader.GetQIndex ();
          uint32_t qIndex = (pfcQIndex >= m_nQueues) ? m_nQueues : pfcQIndex;
          m_pausedStates[qIndex] = false;
          m_pfcRxTrace (m_dev, qIndex, PfcHeader::Resume, pfcHeader.GetTime ());
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
      QbbHeader qbb;
      p->RemoveHeader (ip);
      p->RemoveHeader (qbb);
      auto sIp = ip.GetSource ();
      auto dIp = ip.GetDestination ();
      uint16_t sPort = qbb.GetSourcePort ();
      uint16_t dPort = qbb.GetDestinationPort ();
      uint32_t seq = qbb.GetSequenceNumber ();
      uint8_t flags = qbb.GetFlags ();
      uint16_t dscp = ip.GetDscp ();
      uint32_t payloadSize = p->GetSize ();

      if (flags == QbbHeader::NONE)
        {
          //NS_LOG_UNCOND("\nReceive a packet");
          uint32_t key = RdmaRxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          auto qpItr = m_rxQueuePairs.find (key);
          Ptr<RdmaRxQueuePair> qp;
          if (qpItr == m_rxQueuePairs.end ()) // new flow
            {
              const auto dpskLayer = m_dev->GetNode ()->GetObject<PfcHost> ();
              const auto size = dpskLayer->GetRdmaRxQueuePairSize (key);
              qp = CreateObject<RdmaRxQueuePair> (sIp, dIp, sPort, dPort, size, dscp);
              qp->m_receivedSize += payloadSize;
              m_rxQueuePairs.insert ({key, qp});
            }
          else // existed flow
            {
              qp = qpItr->second;
              qp->m_receivedSize += payloadSize;
            }
          if (m_l2RetrasmissionMode == IRN)
            {
              while (seq >= qp->m_irn.GetNextSequenceNumber ())
                {
                  qp->m_irn.pkg_state.push_back (RdmaRxQueuePair::IRN_STATE::NACK);
                }
              // Already received or expected new packet
              if (seq <= qp->m_irn.base_seq)
                {
                  Ptr<Packet> p = GenACK (qp, seq, true);
                  m_controlQueue.push (p);
                  if (seq < qp->m_irn.base_seq ||
                      qp->m_irn.pkg_state[seq - qp->m_irn.base_seq] ==
                          RdmaRxQueuePair::IRN_STATE::
                              ACK) //TODO lc:handle packets recerved repeatedly
                    qp->m_receivedSize -= payloadSize;

                  qp->m_irn.pkg_state[seq - qp->m_irn.base_seq] = RdmaRxQueuePair::IRN_STATE::ACK;
                  while (qp->m_irn.pkg_state.empty () == false &&
                         qp->m_irn.pkg_state[0] == RdmaRxQueuePair::IRN_STATE::ACK)
                    {
                      qp->m_irn.pkg_state.pop_front ();
                      qp->m_irn.base_seq++;
                    }
                  m_dev->TriggerTransmit ();
                }
              else // out of order
                {
                  Ptr<Packet> p = GenACK (qp, seq, false);
                  m_controlQueue.push (p);
                  if (qp->m_irn.pkg_state[seq - qp->m_irn.base_seq] ==
                      RdmaRxQueuePair::IRN_STATE::ACK) //TODO lc:handle packets recerved repeatedly
                    qp->m_receivedSize -= payloadSize;
                  qp->m_irn.pkg_state[seq - qp->m_irn.base_seq] = RdmaRxQueuePair::IRN_STATE::ACK;
                  while (qp->m_irn.pkg_state.empty () == false &&
                         qp->m_irn.pkg_state[0] == RdmaRxQueuePair::IRN_STATE::ACK)
                    {
                      qp->m_irn.pkg_state.pop_front ();
                      qp->m_irn.base_seq++;
                    }
                  m_dev->TriggerTransmit ();
                }
            }
          if (qp->IsFinished ())
            m_queuePairRxCompleteTrace (qp);
          return true; // Forward up to node
        }
      else if (flags == QbbHeader::ACK)
        {
          // TODO cyq: Handle ACK
          uint32_t key = RdmaTxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          uint32_t index = m_transmittedQP[key];
          Ptr<RdmaTxQueuePair> qp = m_txQueuePairs[index];
          if (seq >= qp->m_irn.base_seq)
            qp->m_irn.pkg_state[seq - (qp->m_irn.base_seq)] = RdmaTxQueuePair::IRN_STATE::ACK;
          if (qp->m_irn.retransmit_mode == true && (seq > qp->m_irn.m_seq))
            {
              qp->m_irn.retransmit_mode = false;
            }
          while (qp->m_irn.pkg_state.size () > 0 &&
                 qp->m_irn.pkg_state[0] == RdmaTxQueuePair::IRN_STATE::ACK)
            {
              qp->m_irn.pkg_state.pop_front ();
              qp->m_irn.pkg_payload.pop_front ();
              qp->m_irn.base_seq++;
            }
          m_dev->TriggerTransmit ();
          return false; // Not data so no need to send to node
        }
      else if (flags == QbbHeader::SACK)
        {
          // TODO cyq: Handle SACK
          uint32_t key = RdmaTxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          uint32_t index = m_transmittedQP[key];
          Ptr<RdmaTxQueuePair> qp = m_txQueuePairs[index];
          qp->m_irn.pkg_state[seq - (qp->m_irn.base_seq)] = RdmaTxQueuePair::IRN_STATE::ACK;
          if (qp->m_irn.retransmit_mode == false)
            {
              NS_LOG_UNCOND ("\nReceive and handle SACK" << seq << " " << qp->m_irn.base_seq);
              for (uint32_t i = 0; i < (seq - (qp->m_irn.base_seq)); i++)
                {
                  if (qp->m_irn.pkg_state[i] == RdmaTxQueuePair::IRN_STATE::UNACK)
                    {
                      uint32_t siz = qp->m_irn.pkg_payload[i];
                      uint32_t rtseq = i + (qp->m_irn.base_seq);
                      Ptr<Packet> p = ReGenData (qp, siz, rtseq);
                      if (FindRetranPkg (p) == false)
                        m_retranPkg.push_back (p);
                    }
                }
              qp->m_irn.m_seq = seq;
              qp->m_irn.retransmit_mode = true;
            }
          m_dev->TriggerTransmit ();
          return false; // Not data so no need to send to node
        }
      else
        {
          return false; // Drop because unknown flags
        }
    }
}

Ptr<Packet>
PfcHostPort::IRN_GenData (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);

  const uint32_t remainSize = qp->GetRemainBytes ();
  const uint32_t mtu = m_dev->GetMtu ();
  uint32_t payloadSize = (remainSize > mtu) ? mtu : remainSize;

  qp->m_sentSize += payloadSize;

  Ptr<Packet> p = Create<Packet> (payloadSize);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_sPort);
  qbb.SetDestinationPort (qp->m_dPort);

  qbb.SetSequenceNumber (qp->m_irn.GetNextSequenceNumber ());
  qbb.SetFlags (QbbHeader::NONE);
  qp->m_irn.pkg_state.push_back (RdmaTxQueuePair::IRN_STATE::UNACK);
  qp->m_irn.pkg_payload.push_back (payloadSize);
  if (qp->m_irn.pkg_state.size () > 3)
    Simulator::Schedule (m_irn.rtoHigh, &PfcHostPort::TimeOutRetran, this, qp, payloadSize,
                         qp->m_irn.GetNextSequenceNumber ());
  else
    Simulator::Schedule (m_irn.rtoLow, &PfcHostPort::TimeOutRetran, this, qp, payloadSize,
                         qp->m_irn.GetNextSequenceNumber ());

  p->AddHeader (qbb);

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

Ptr<Packet>
PfcHostPort::GenData (Ptr<RdmaTxQueuePair> qp)
{
  NS_LOG_FUNCTION (qp);

  const uint32_t remainSize = qp->GetRemainBytes ();
  const uint32_t mtu = m_dev->GetMtu ();
  uint32_t payloadSize = (remainSize > mtu) ? mtu : remainSize;

  qp->m_sentSize += payloadSize;

  Ptr<Packet> p = Create<Packet> (payloadSize);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_sPort);
  qbb.SetDestinationPort (qp->m_dPort);
  p->AddHeader (qbb);

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

Ptr<Packet>
PfcHostPort::GenACK (Ptr<RdmaRxQueuePair> qp, uint32_t seq, bool ack)
{
  NS_LOG_FUNCTION (qp);

  Ptr<Packet> p = Create<Packet> (0);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_dPort); // exchange ports
  qbb.SetDestinationPort (qp->m_sPort);
  qbb.SetSequenceNumber (seq);
  qbb.SetFlags (ack ? QbbHeader::ACK : QbbHeader::SACK);
  p->AddHeader (qbb);

  Ipv4Header ip;
  ip.SetSource (qp->m_dIp); // exchange IPs
  ip.SetDestination (qp->m_sIp);
  ip.SetProtocol (0x11); // UDP
  ip.SetPayloadSize (p->GetSize ());
  ip.SetTtl (64);
  ip.SetDscp (Ipv4Header::DscpType (qp->m_priority)); // XXX cyq: highest prio?
  p->AddHeader (ip);

  EthernetHeader eth;
  eth.SetSource (Mac48Address::ConvertFrom (m_dev->GetAddress ()));
  eth.SetDestination (Mac48Address::ConvertFrom (m_dev->GetRemote ()));
  eth.SetLengthType (0x0800); // IPv4
  p->AddHeader (eth);

  return p;
}

Ptr<Packet>
PfcHostPort::ReGenData (Ptr<RdmaTxQueuePair> qp, uint32_t siz, uint32_t seq)
{
  NS_LOG_FUNCTION (qp);

  uint32_t payloadSize = siz;

  Ptr<Packet> p = Create<Packet> (payloadSize);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_sPort);
  qbb.SetDestinationPort (qp->m_dPort);
  qbb.SetSequenceNumber (seq);
  qbb.SetFlags (QbbHeader::NONE);

  if (m_l2RetrasmissionMode == IRN)
    {
      if (qp->m_irn.pkg_state.size () > 3)
        Simulator::Schedule (m_irn.rtoHigh, &PfcHostPort::TimeOutRetran, this, qp, payloadSize,
                             seq);
      else
        Simulator::Schedule (m_irn.rtoLow, &PfcHostPort::TimeOutRetran, this, qp, payloadSize, seq);
    }

  p->AddHeader (qbb);

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
PfcHostPort::TimeOutRetran (Ptr<RdmaTxQueuePair> qp, uint32_t siz, uint32_t seq)
{
  if (qp->m_irn.retransmit_mode == false && seq >= qp->m_irn.base_seq &&
      qp->m_irn.pkg_state[seq - qp->m_irn.base_seq] == RdmaTxQueuePair::IRN_STATE::UNACK)
    {
      NS_LOG_UNCOND ("\ntime out retransmit" << seq);
      Ptr<Packet> p = ReGenData (qp, siz, seq);
      if (FindRetranPkg (p) == false)
        {
          m_retranPkg.push_back (p);
          m_dev->TriggerTransmit ();
        }
    }

  return;
}

bool
PfcHostPort::FindRetranPkg (Ptr<Packet> p)
{
  for (uint32_t i = 0; i < m_retranPkg.size (); i++)
    {
      if (p == m_retranPkg[i])
        return true;
    }
  return false;
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
