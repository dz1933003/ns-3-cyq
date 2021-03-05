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

#include <algorithm>

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
    : m_pfcEnabled (true),
      m_l2RetransmissionMode (L2_RTX_MODE::NONE_RTX),
      m_nTxBytes (0),
      m_nRxBytes (0)
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
  m_txQueuePairTable.insert ({qp->GetHash (), m_txQueuePairs.size () - 1});
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
  m_l2RetransmissionMode = mode;
}

uint32_t
PfcHostPort::L2RtxModeStringToNum (const std::string &mode)
{
  NS_LOG_FUNCTION (mode);
  if (mode == "NONE")
    return L2_RTX_MODE::NONE_RTX;
  else if (mode == "IRN")
    return L2_RTX_MODE::IRN;
  else if (mode == "B20")
    return L2_RTX_MODE::B20;
  else if (mode == "B2N")
    return L2_RTX_MODE::B2N;
  else
    NS_ASSERT_MSG (false, "PfcHostPort::L2RtxModeStringToNum: "
                          "Unknown L2 retransmission mode");
}

void
PfcHostPort::SetupIrn (uint32_t size, Time rtoh, Time rtol, uint32_t n)
{
  NS_LOG_FUNCTION (size << rtoh << rtol << n);
  m_irn.maxBitmapSize = size;
  m_irn.rtoHigh = rtoh;
  m_irn.rtoLow = rtol;
  m_irn.rtoLowThreshold = n;
}

void
PfcHostPort::SetCcMode (uint32_t mode)
{
  NS_LOG_FUNCTION (mode);
  m_ccMode = mode;
}

uint32_t
PfcHostPort::CcModeStringToNum (const std::string &mode)
{
  NS_LOG_FUNCTION (mode);
  if (mode == "NONE")
    return CC_MODE::NONE_CC;
  else if (mode == "DCQCN")
    return CC_MODE::DCQCN;
  else
    NS_ASSERT_MSG (false, "PfcHostPort::CcModeStringToNum: "
                          "Unknown congestion control mode");
}

Ptr<Packet>
PfcHostPort::Transmit ()
{
  NS_LOG_FUNCTION_NOARGS ();

  // Check and send control packet
  if ((m_pausedStates[m_nQueues] == false || m_pfcEnabled == false) &&
      m_controlQueue.empty () == false)
    {
      Ptr<Packet> p = m_controlQueue.front ();
      m_controlQueue.pop ();
      m_nTxBytes += p->GetSize ();
      return p;
    }

  // Retransmit packet (for IRN only now)
  if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
    {
      while (m_rtxPacketQueue.empty () == false)
        {
          const auto qp = m_rtxPacketQueue.front ().first;
          const auto irnSeq = m_rtxPacketQueue.front ().second;
          const auto state = qp->m_irn.GetIrnState (irnSeq);
          m_rtxPacketQueue.pop_front ();
          // Set up IRN timer
          if (state == RdmaTxQueuePair::NACK ||
              state == RdmaTxQueuePair::UNACK) // filter out ACKed packets
            {
              auto id = IrnTimer (qp, irnSeq);
              qp->m_irn.SetRtxEvent (irnSeq, id);
              return ReGenData (qp, irnSeq, qp->m_irn.GetPayloadSize (irnSeq));
            }
        }
    }

  // Transmit data packet
  uint32_t flowCnt = m_txQueuePairs.size ();
  for (uint32_t i = 0; i < flowCnt; i++)
    {
      uint32_t qIdx = (m_lastQpIndex + i + 1) % flowCnt;
      auto qp = m_txQueuePairs[qIdx];
      // Sending constrains:
      // 1. PFC RESUME.
      // 2. QP is not finished.
      // 3. QP is started.
      // 4. In IRN mode the sending window is not full.
      if ((m_pausedStates[qp->m_priority] == false || m_pfcEnabled == false) &&
          qp->IsTxFinished () == false && qp->m_startTime <= Simulator::Now () &&
          (m_l2RetransmissionMode != L2_RTX_MODE::IRN ||
           qp->m_irn.GetWindowSize () < m_irn.maxBitmapSize))
        {
          if (m_ccMode == CC_MODE::NONE_CC)
            {
              m_lastQpIndex = qIdx;
              uint32_t irnSeq;
              auto p = GenData (qp, irnSeq); // get sequence number out
              if (qp->IsTxFinished ())
                m_queuePairTxCompleteTrace (qp);
              m_nTxBytes += p->GetSize ();
              // Set up IRN timer
              if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
                {
                  auto id = IrnTimer (qp, irnSeq);
                  qp->m_irn.SetRtxEvent (irnSeq, id);
                }
              return p;
            }
          else if (m_ccMode == CC_MODE::DCQCN)
            {
              // TODO cyq: In IRN mode, the DCQCN window is disabled because IRN have one.
              if (qp->GetRemainBytes () > 0 && !qp->IsWinBound () && !qp->IsTxFinished ())
                {
                  if (qp->m_nextAvail > Simulator::Now ()) // Not available now
                    continue;
                  m_lastQpIndex = qIdx;
                  auto p = GenData (qp);
                  if (qp->IsTxFinished ())
                    m_queuePairTxCompleteTrace (qp);
                  m_nTxBytes += p->GetSize ();
                  UpdateNextAvail (qp, Time (0), p->GetSize ());
                  return p;
                }
            }
          else
            {
              NS_ASSERT_MSG (false, "PfcHostPort::Transmit: "
                                    "CC mode not valid");
            }
        }
    }

  // No packet to send
  if (m_ccMode == CC_MODE::DCQCN)
    {
      // For DCQCN schedule next transmit
      const Time now = Simulator::Now ();
      const Time maxInfTime = Simulator::GetMaximumSimulationTime ();
      Time minAvailTime = maxInfTime;
      // Find minimum next avail packet transmission time
      for (auto qp : m_txQueuePairs)
        {
          if (qp->IsTxFinished ())
            continue;
          minAvailTime = Min (qp->m_nextAvail, minAvailTime);
        }
      // Schedule next transmission if no previous scheduling transmission event
      if (m_nextTransmitEvent.IsExpired () && minAvailTime < maxInfTime && minAvailTime > now)
        {
          m_nextTransmitEvent =
              Simulator::Schedule (minAvailTime - now, &DpskNetDevice::TriggerTransmit, m_dev);
        }
    }

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
      uint32_t irnAck = qbb.GetIrnAckNumber ();
      uint32_t irnNack = qbb.GetIrnNackNumber ();
      uint8_t flags = qbb.GetFlags ();
      bool cnp = qbb.GetCnp ();
      uint16_t dscp = ip.GetDscp ();
      auto ecn = ip.GetEcn ();
      const bool isCe = ecn == Ipv4Header::EcnType::ECN_CE;
      uint32_t payloadSize = p->GetSize ();

      // Data packet
      if (flags == QbbHeader::NONE)
        {
          // Find qp in rx qp table
          uint32_t key = RdmaRxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          auto qpItr = m_rxQueuePairs.find (key);
          Ptr<RdmaRxQueuePair> qp;
          if (qpItr == m_rxQueuePairs.end ()) // new flow
            {
              // Adding new flow in rx qp table
              const auto dpskLayer = m_dev->GetNode ()->GetObject<PfcHost> ();
              const auto size = dpskLayer->GetRdmaRxQueuePairSize (key);
              qp = CreateObject<RdmaRxQueuePair> (sIp, dIp, sPort, dPort, size, dscp);
              m_rxQueuePairs.insert ({key, qp});
            }
          else // existed flow
            {
              qp = qpItr->second;
            }

          // retransmission and rx byte count
          if (m_l2RetransmissionMode == L2_RTX_MODE::NONE_RTX)
            {
              qp->m_receivedSize += payloadSize;
            }
          else if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
            {
              const uint32_t expectedAck = qp->m_irn.GetNextSequenceNumber ();
              if (irnAck < expectedAck) // in window
                {
                  if (!qp->m_irn.IsReceived (irnAck)) // Not duplicated packet
                    {
                      qp->m_receivedSize += payloadSize;
                      qp->m_irn.UpdateIrnState (irnAck);
                    }
                  // Send ACK and trigger transmit
                  m_controlQueue.push (GenACK (qp, seq, irnAck, isCe));
                  m_dev->TriggerTransmit ();
                }
              else if (irnAck == expectedAck) // expected new packet
                {
                  qp->m_receivedSize += payloadSize;
                  qp->m_irn.UpdateIrnState (irnAck);
                  // Send ACK by retransmit mode and trigger transmit
                  m_controlQueue.push (GenACK (qp, seq, irnAck, isCe));
                  m_dev->TriggerTransmit ();
                }
              else // out of order
                {
                  qp->m_receivedSize += payloadSize;
                  qp->m_irn.UpdateIrnState (irnAck);
                  // Send SACK and trigger transmit
                  m_controlQueue.push (GenSACK (qp, seq, irnAck, expectedAck, isCe));
                  m_dev->TriggerTransmit ();
                }
            }
          else if (m_l2RetransmissionMode == L2_RTX_MODE::B20 ||
                   m_l2RetransmissionMode == L2_RTX_MODE::B2N)
            {
              qp->m_b2n_0.m_milestone_rx = m_ack_interval;
              const uint32_t expectedSeq = qp->m_receivedSize;
              if (seq == expectedSeq)
                {
                  qp->m_receivedSize = expectedSeq + payloadSize;
                  if (qp->m_receivedSize >= qp->m_b2n_0.m_milestone_rx)
                    {
                      qp->m_b2n_0.m_milestone_rx += m_ack_interval;
                      m_controlQueue.push (GenACK (qp, seq, 0, isCe));
                      m_dev->TriggerTransmit ();
                    }
                  else if (qp->m_receivedSize % m_chunk == 0)
                    {
                      m_controlQueue.push (GenACK (qp, seq, 0, isCe));
                      m_dev->TriggerTransmit ();
                    }
                }
              else if (seq > expectedSeq)
                {
                  const Time now = Simulator::Now ();
                  if (now >= qp->m_b2n_0.m_nackTimer || qp->m_b2n_0.m_lastNACK != expectedSeq)
                    {
                      qp->m_b2n_0.m_nackTimer = now + MicroSeconds (m_nack_interval);
                      qp->m_b2n_0.m_lastNACK = expectedSeq;
                      if (m_l2RetransmissionMode == L2_RTX_MODE::B20)
                        qp->m_receivedSize = qp->m_receivedSize / m_chunk * m_chunk;
                      m_controlQueue.push (GenSACK (qp, seq, 0, 0, isCe));
                      m_dev->TriggerTransmit ();
                    }
                }
            }

          if (qp->IsFinished ())
            m_queuePairRxCompleteTrace (qp);

          return true; // Forward up to node
        }
      else if (flags == QbbHeader::ACK)
        {
          // Handle ACK
          uint32_t key = RdmaTxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          uint32_t index = m_txQueuePairTable[key];
          Ptr<RdmaTxQueuePair> qp = m_txQueuePairs[index];

          if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
            {
              qp->m_irn.AckIrnState (irnAck);

              // Cleanup timer for DCQCN
              if (m_ccMode == CC_MODE::DCQCN && qp->IsTxFinished ())
                QpComplete (qp);

              m_dev->TriggerTransmit (); // Because BDP-FC needs to check new bitmap and send
              return false; // Not data so no need to send to node
            }
          else if (m_l2RetransmissionMode == L2_RTX_MODE::B20 ||
                   m_l2RetransmissionMode == L2_RTX_MODE::B2N)
            {
              if (m_ack_interval == 0)
                {
                  NS_ASSERT_MSG (false, "PfcHostPort::Receive: "
                                        "Shouldn't receive ACK");
                }
              else
                {
                  if (m_l2RetransmissionMode == L2_RTX_MODE::B2N)
                    qp->Acknowledge (seq);
                  else
                    qp->Acknowledge (seq / m_chunk * m_chunk);

                  if (qp->IsAckedFinished ())
                    QpComplete (qp);
                }

              if (cnp)
                m_dcqcn.cnp_received_mlx (qp);

              m_dev->TriggerTransmit (); // Because ACK may advance the on-the-fly window
              return false;
            }
          else
            {
              return false;
            }
        }
      else if (flags == QbbHeader::SACK)
        {
          // Handle SACK
          uint32_t key = RdmaTxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          uint32_t index = m_txQueuePairTable[key];
          Ptr<RdmaTxQueuePair> qp = m_txQueuePairs[index];

          if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
            {
              // Add retransmission packets and trigger transmitting
              qp->m_irn.SackIrnState (irnAck, irnNack);
              for (auto i = irnNack; i < irnAck; i++)
                {
                  m_rtxPacketQueue.push_back ({qp, i});
                }
              m_dev->TriggerTransmit ();
              return false; // Not data so no need to send to node
            }
          else if (m_l2RetransmissionMode == L2_RTX_MODE::B20 ||
                   m_l2RetransmissionMode == L2_RTX_MODE::B2N)
            {
              if (m_ack_interval == 0)
                {
                  NS_ASSERT_MSG (false, "PfcHostPort::Receive: "
                                        "Shouldn't receive ACK");
                }
              else
                {
                  if (m_l2RetransmissionMode == L2_RTX_MODE::B2N)
                    qp->Acknowledge (seq);
                  else
                    qp->Acknowledge (seq / m_chunk * m_chunk);

                  if (qp->IsAckedFinished ())
                    QpComplete (qp);
                }

              RecoverQueue (qp);

              if (cnp)
                m_dcqcn.cnp_received_mlx (qp);

              m_dev->TriggerTransmit (); // Because ACK may advance the on-the-fly window
              return false;
            }
          else
            {
              return false;
            }
        }
      else
        {
          return false; // Drop because unknown flags
        }
    }
}

Ptr<Packet>
PfcHostPort::GenData (Ptr<RdmaTxQueuePair> qp)
{
  uint32_t tmp;
  return GenData (qp, tmp);
}

Ptr<Packet>
PfcHostPort::GenData (Ptr<RdmaTxQueuePair> qp, uint32_t &o_irnSeq)
{
  NS_LOG_FUNCTION (qp);

  const uint32_t remainSize = qp->GetRemainBytes ();
  const uint32_t mtu = m_dev->GetMtu ();
  const uint32_t maxPayloadSize = mtu - QbbHeader ().GetSerializedSize () -
                                  Ipv4Header ().GetSerializedSize () -
                                  EthernetHeader ().GetSerializedSize ();
  uint32_t payloadSize = (remainSize > maxPayloadSize) ? maxPayloadSize : remainSize;

  Ptr<Packet> p = Create<Packet> (payloadSize);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_sPort);
  qbb.SetDestinationPort (qp->m_dPort);
  if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
    {
      const auto seq = qp->m_irn.GetNextSequenceNumber ();
      o_irnSeq = seq;
      qbb.SetIrnAckNumber (seq);
      qbb.SetIrnNackNumber (0);
      qbb.SetFlags (QbbHeader::NONE);
      qp->m_irn.SendNewPacket (payloadSize); // Update IRN infos
    }
  if (m_ccMode == CC_MODE::DCQCN)
    {
      qbb.SetSequenceNumber (qp->m_txSize);
      qbb.SetFlags (QbbHeader::NONE);
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

  qp->m_txSize += payloadSize;

  return p;
}

Ptr<Packet>
PfcHostPort::GenACK (Ptr<RdmaRxQueuePair> qp, uint32_t seq, uint32_t irnAck, bool cnp)
{
  NS_LOG_FUNCTION (qp);

  Ptr<Packet> p = Create<Packet> (0);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_dPort); // exchange ports
  qbb.SetDestinationPort (qp->m_sPort);
  qbb.SetSequenceNumber (seq);
  qbb.SetIrnAckNumber (irnAck);
  qbb.SetIrnNackNumber (0);
  qbb.SetFlags (QbbHeader::ACK);
  qbb.SetCnp (cnp);
  p->AddHeader (qbb);

  Ipv4Header ip;
  ip.SetSource (qp->m_dIp); // exchange IPs
  ip.SetDestination (qp->m_sIp);
  ip.SetProtocol (0x11); // UDP
  ip.SetPayloadSize (p->GetSize ());
  ip.SetTtl (64);
  ip.SetDscp (Ipv4Header::DscpType (m_nQueues)); // highest priority
  p->AddHeader (ip);

  EthernetHeader eth;
  eth.SetSource (Mac48Address::ConvertFrom (m_dev->GetAddress ()));
  eth.SetDestination (Mac48Address::ConvertFrom (m_dev->GetRemote ()));
  eth.SetLengthType (0x0800); // IPv4
  p->AddHeader (eth);

  return p;
}

Ptr<Packet>
PfcHostPort::GenSACK (Ptr<RdmaRxQueuePair> qp, uint32_t seq, uint32_t irnAck, uint32_t irnNack,
                      bool cnp)
{
  NS_LOG_FUNCTION (qp);

  Ptr<Packet> p = Create<Packet> (0);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_dPort); // exchange ports
  qbb.SetDestinationPort (qp->m_sPort);
  qbb.SetSequenceNumber (seq);
  qbb.SetIrnAckNumber (irnAck);
  qbb.SetIrnNackNumber (irnNack);
  qbb.SetFlags (QbbHeader::SACK);
  qbb.SetCnp (cnp);
  p->AddHeader (qbb);

  Ipv4Header ip;
  ip.SetSource (qp->m_dIp); // exchange IPs
  ip.SetDestination (qp->m_sIp);
  ip.SetProtocol (0x11); // UDP
  ip.SetPayloadSize (p->GetSize ());
  ip.SetTtl (64);
  ip.SetDscp (Ipv4Header::DscpType (m_nQueues)); // highest priority
  p->AddHeader (ip);

  EthernetHeader eth;
  eth.SetSource (Mac48Address::ConvertFrom (m_dev->GetAddress ()));
  eth.SetDestination (Mac48Address::ConvertFrom (m_dev->GetRemote ()));
  eth.SetLengthType (0x0800); // IPv4
  p->AddHeader (eth);

  return p;
}

Ptr<Packet>
PfcHostPort::ReGenData (Ptr<RdmaTxQueuePair> qp, uint32_t irnSeq, uint32_t size)
{
  NS_LOG_FUNCTION (qp);

  uint32_t payloadSize = size;

  Ptr<Packet> p = Create<Packet> (payloadSize);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_sPort);
  qbb.SetDestinationPort (qp->m_dPort);
  qbb.SetIrnAckNumber (irnSeq);
  qbb.SetIrnNackNumber (0);
  qbb.SetFlags (QbbHeader::NONE);
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

EventId
PfcHostPort::IrnTimer (Ptr<RdmaTxQueuePair> qp, uint32_t irnSeq)
{
  if (qp->m_irn.GetWindowSize () <= m_irn.rtoLowThreshold)
    {
      return Simulator::Schedule (m_irn.rtoLow, &PfcHostPort::IrnTimerHandler, this, qp, irnSeq);
    }
  else
    {
      return Simulator::Schedule (m_irn.rtoHigh, &PfcHostPort::IrnTimerHandler, this, qp, irnSeq);
    }
}

void
PfcHostPort::IrnTimerHandler (Ptr<RdmaTxQueuePair> qp, uint32_t irnSeq)
{
  const auto state = qp->m_irn.GetIrnState (irnSeq);
  if (state == RdmaTxQueuePair::NACK || state == RdmaTxQueuePair::UNACK)
    {
      m_rtxPacketQueue.push_back ({qp, irnSeq});
      m_dev->TriggerTransmit ();
    }
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

void
PfcHostPort::UpdateNextAvail (Ptr<RdmaTxQueuePair> qp, Time interframeGap, uint32_t pkt_size)
{
  Time sendingTime;
  if (m_rateBound)
    sendingTime = interframeGap + qp->m_rate.CalculateBytesTxTime (pkt_size);
  else
    sendingTime = interframeGap + qp->m_max_rate.CalculateBytesTxTime (pkt_size);
  qp->m_nextAvail = Simulator::Now () + sendingTime;
}

void
PfcHostPort::RecoverQueue (Ptr<RdmaTxQueuePair> qp)
{
  qp->m_txSize = qp->m_unackSize;
}

void
PfcHostPort::QpComplete (Ptr<RdmaTxQueuePair> qp)
{
  Simulator::Cancel (qp->mlx.m_eventUpdateAlpha);
  Simulator::Cancel (qp->mlx.m_eventDecreaseRate);
  Simulator::Cancel (qp->mlx.m_rpTimer);
}

/******************************
 * Mellanox's version of DCQCN
 *****************************/

void
PfcHostPort::Dcqcn::UpdateAlphaMlx (Ptr<RdmaTxQueuePair> qp)
{
  if (qp->mlx.m_alpha_cnp_arrived)
    {
      qp->mlx.m_alpha = (1 - m_g) * qp->mlx.m_alpha + m_g; //binary feedback
    }
  else
    {
      qp->mlx.m_alpha = (1 - m_g) * qp->mlx.m_alpha; //binary feedback
    }
  qp->mlx.m_alpha_cnp_arrived = false; // clear the CNP_arrived bit
  ScheduleUpdateAlphaMlx (qp);
}

void
PfcHostPort::Dcqcn::ScheduleUpdateAlphaMlx (Ptr<RdmaTxQueuePair> qp)
{
  qp->mlx.m_eventUpdateAlpha = Simulator::Schedule (MicroSeconds (m_alpha_resume_interval),
                                                    &PfcHostPort::Dcqcn::UpdateAlphaMlx, this, qp);
}

void
PfcHostPort::Dcqcn::cnp_received_mlx (Ptr<RdmaTxQueuePair> qp)
{
  qp->mlx.m_alpha_cnp_arrived = true; // set CNP_arrived bit for alpha update
  qp->mlx.m_decrease_cnp_arrived = true; // set CNP_arrived bit for rate decrease
  if (qp->mlx.m_first_cnp)
    {
      // init alpha
      qp->mlx.m_alpha = 1;
      qp->mlx.m_alpha_cnp_arrived = false;
      // schedule alpha update
      ScheduleUpdateAlphaMlx (qp);
      // schedule rate decrease
      ScheduleDecreaseRateMlx (qp, 1); // add 1 ns to make sure rate decrease is after alpha update
      // set rate on first CNP
      qp->mlx.m_targetRate = qp->m_rate = DataRate (m_rateOnFirstCNP * qp->m_rate.GetBitRate ());
      qp->mlx.m_first_cnp = false;
    }
}

void
PfcHostPort::Dcqcn::CheckRateDecreaseMlx (Ptr<RdmaTxQueuePair> qp)
{
  ScheduleDecreaseRateMlx (qp, 0);
  if (qp->mlx.m_decrease_cnp_arrived)
    {
      bool clamp = true;
      if (!m_EcnClampTgtRate)
        {
          if (qp->mlx.m_rpTimeStage == 0)
            clamp = false;
        }
      if (clamp)
        qp->mlx.m_targetRate = qp->m_rate;
      qp->m_rate =
          std::max (m_minRate, DataRate (qp->m_rate.GetBitRate () * (1 - qp->mlx.m_alpha / 2)));
      // reset rate increase related things
      qp->mlx.m_rpTimeStage = 0;
      qp->mlx.m_decrease_cnp_arrived = false;
      Simulator::Cancel (qp->mlx.m_rpTimer);
      qp->mlx.m_rpTimer = Simulator::Schedule (MicroSeconds (m_rpgTimeReset),
                                               &PfcHostPort::Dcqcn::RateIncEventTimerMlx, this, qp);
    }
}

void
PfcHostPort::Dcqcn::ScheduleDecreaseRateMlx (Ptr<RdmaTxQueuePair> qp, uint32_t delta)
{
  qp->mlx.m_eventDecreaseRate =
      Simulator::Schedule (MicroSeconds (m_rateDecreaseInterval) + NanoSeconds (delta),
                           &PfcHostPort::Dcqcn::CheckRateDecreaseMlx, this, qp);
}

void
PfcHostPort::Dcqcn::RateIncEventTimerMlx (Ptr<RdmaTxQueuePair> qp)
{
  qp->mlx.m_rpTimer = Simulator::Schedule (MicroSeconds (m_rpgTimeReset),
                                           &PfcHostPort::Dcqcn::RateIncEventTimerMlx, this, qp);
  RateIncEventMlx (qp);
  qp->mlx.m_rpTimeStage++;
}

void
PfcHostPort::Dcqcn::RateIncEventMlx (Ptr<RdmaTxQueuePair> qp)
{
  // check which increase phase: fast recovery, active increase, hyper increase
  if (qp->mlx.m_rpTimeStage < m_rpgThreshold)
    { // fast recovery
      FastRecoveryMlx (qp);
    }
  else if (qp->mlx.m_rpTimeStage == m_rpgThreshold)
    { // active increase
      ActiveIncreaseMlx (qp);
    }
  else
    { // hyper increase
      HyperIncreaseMlx (qp);
    }
}

void
PfcHostPort::Dcqcn::FastRecoveryMlx (Ptr<RdmaTxQueuePair> qp)
{
  qp->m_rate = DataRate ((qp->m_rate.GetBitRate () / 2) + (qp->mlx.m_targetRate.GetBitRate () / 2));
}

void
PfcHostPort::Dcqcn::ActiveIncreaseMlx (Ptr<RdmaTxQueuePair> qp)
{
  // increate rate
  qp->mlx.m_targetRate = DataRate (qp->mlx.m_targetRate.GetBitRate () + m_rai.GetBitRate ());
  if (qp->mlx.m_targetRate > m_dev_rate)
    qp->mlx.m_targetRate = m_dev_rate;
  qp->m_rate = DataRate ((qp->m_rate.GetBitRate () / 2) + (qp->mlx.m_targetRate.GetBitRate () / 2));
}

void
PfcHostPort::Dcqcn::HyperIncreaseMlx (Ptr<RdmaTxQueuePair> qp)
{
  // increate rate
  qp->mlx.m_targetRate = DataRate (qp->mlx.m_targetRate.GetBitRate () + m_rhai.GetBitRate ());
  if (qp->mlx.m_targetRate > m_dev_rate)
    qp->mlx.m_targetRate = m_dev_rate;
  qp->m_rate = DataRate ((qp->m_rate.GetBitRate () / 2) + (qp->mlx.m_targetRate.GetBitRate () / 2));
}

} // namespace ns3
