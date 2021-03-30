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
      m_rtxQueuingCnt (0),
      m_l2RetransmissionMode (L2_RTX_MODE::NONE_RTX),
      m_nTxBytes (0),
      m_nRxBytes (0),
      m_irnRtxBytes (0),
      m_irnRtxRxBytes (0)
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

  if (m_ccMode == CC_MODE::DCQCN)
    {
      const auto rate = m_dev->GetDataRate ();
      qp->SetupRate (rate, rate);
    }

  m_txQueuePairs.push_back (qp);
  m_txQueuePairTable.insert ({qp->GetHash (), m_txQueuePairs.size () - 1});

  m_rtxSeqQueues.push_back (std::queue<uint32_t> ());

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

Ptr<RdmaRxQueuePair>
PfcHostPort::GetRdmaRxQueuePair (uint32_t hash)
{
  NS_LOG_FUNCTION (hash);
  if (m_rxQueuePairs.find (hash) != m_rxQueuePairs.end ())
    return m_rxQueuePairs[hash];
  else
    return 0;
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
PfcHostPort::SetupB2x (uint32_t chunk, uint32_t ackInterval, Time nackInterval)
{
  NS_LOG_FUNCTION (chunk << ackInterval << nackInterval);
  m_b2x.chunk = chunk;
  m_b2x.ackInterval = ackInterval;
  m_b2x.nackInterval = nackInterval;
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

void
PfcHostPort::SetupDcqcn (PfcHostPort::Dcqcn dcqcn)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dcqcn = dcqcn;
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

  // Notice that IRN is not enabled with PFC
  if (m_l2RetransmissionMode == L2_RTX_MODE::IRN && m_rtxQueuingCnt > 0)
    {
      uint32_t flowCnt = m_txQueuePairs.size ();
      for (uint32_t i = 0; i < flowCnt; i++)
        {
          const uint32_t qIdx = (m_lastQpIndex + i + 1) % flowCnt;
          if (m_rtxSeqQueues[qIdx].empty ())
            continue;

          const auto qp = m_txQueuePairs[qIdx];
          if (m_rtxSeqQueues[qIdx].empty () == false)
            {
              const auto irnSeq = m_rtxSeqQueues[qIdx].front ();
              const auto state = qp->m_irn.GetIrnState (irnSeq);

              if (m_ccMode == CC_MODE::DCQCN && qp->m_nextAvail > Simulator::Now ())
                continue;

              m_rtxSeqQueues[qIdx].pop ();
              m_rtxQueuingCnt--;

              if (state == RdmaTxQueuePair::NACK ||
                  state == RdmaTxQueuePair::UNACK) // filter out ACKed packets
                {
                  // Round robin
                  m_lastQpIndex = qIdx;
                  // Set up IRN timer
                  const auto id = IrnTimer (qp, irnSeq);
                  qp->m_irn.SetRtxEvent (irnSeq, id);
                  const auto p = ReGenData (qp, irnSeq, qp->m_irn.GetPayloadSize (irnSeq));
                  // DCQCN schedule next send by rate limiter
                  if (m_ccMode == CC_MODE::DCQCN)
                    UpdateNextAvail (qp, Time (0), p->GetSize ());
                  // Statistic Tx
                  m_nTxBytes += p->GetSize ();
                  m_irnRtxBytes += p->GetSize ();
                  return p;
                }
            }
          else
            {
              // DCQCN avoid next avail earlier than now
              // Otherwise no new transmission for this qp
              if (m_ccMode == CC_MODE::DCQCN)
                {
                  if (qp->m_nextAvail <= Simulator::Now ())
                    UpdateNextAvail (qp, Time ("1ns"), 0);
                }
            }
        }
    }

  // Transmit data packet
  uint32_t flowCnt = m_txQueuePairs.size ();
  for (uint32_t i = 0; i < flowCnt; i++)
    {
      uint32_t qIdx = (m_lastQpIndex + i + 1) % flowCnt;
      auto qp = m_txQueuePairs[qIdx];
      // PFC RESUME
      if (m_pfcEnabled && m_pausedStates[qp->m_priority])
        continue;
      // QP is not finished
      if (qp->IsTxFinished ())
        continue;
      // QP is started
      if (qp->m_startTime > Simulator::Now ())
        continue;
      if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
        {
          // In IRN mode the sending window is not full
          if (qp->m_irn.GetWindowSize () >= m_irn.maxBitmapSize)
            continue;
        }
      else if (m_l2RetransmissionMode == L2_RTX_MODE::B20 ||
               m_l2RetransmissionMode == L2_RTX_MODE::B2N)
        {
          // DCQCN B2X window bound (disabled in IRN)
          if (qp->B2xIsWinBound ())
            continue;
        }
      // DCQCN is ready to transmit
      if (m_ccMode == CC_MODE::DCQCN && qp->m_nextAvail > Simulator::Now ())
        continue;

      // Round robin
      m_lastQpIndex = qIdx;
      // get sequence number out for logging retransmission
      uint32_t irnSeq;
      auto p = GenData (qp, irnSeq);
      // Finished trace (only send all packets but not acked)
      if (qp->IsTxFinished ())
        m_queuePairTxCompleteTrace (qp);
      // Set up IRN timer
      if (m_l2RetransmissionMode == L2_RTX_MODE::IRN)
        {
          auto id = IrnTimer (qp, irnSeq);
          qp->m_irn.SetRtxEvent (irnSeq, id);
        }
      // DCQCN schedule next send by rate limiter
      if (m_ccMode == CC_MODE::DCQCN)
        UpdateNextAvail (qp, Time (0), p->GetSize ());
      // Statistic Tx
      m_nTxBytes += p->GetSize ();
      return p;
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
          if (m_l2RetransmissionMode != L2_RTX_MODE::IRN && qp->IsTxFinished ())
            continue;
          if (m_l2RetransmissionMode == L2_RTX_MODE::IRN && qp->IsAckedFinished ())
            continue; // Schedule IRN retransmission because rate limiter
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

              if (qp->IsFinished ())
                m_queuePairRxCompleteTrace (qp);
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

                      if (qp->IsFinished ())
                        m_queuePairRxCompleteTrace (qp);
                    }
                  else // Duplicated packet
                    {
                      m_irnRtxRxBytes += payloadSize;
                    }
                  // Send ACK and trigger transmit
                  m_controlQueue.push (GenACK (qp, qp->m_receivedSize, irnAck, isCe));
                  m_dev->TriggerTransmit ();
                }
              else if (irnAck == expectedAck) // expected new packet
                {
                  qp->m_receivedSize += payloadSize;
                  qp->m_irn.UpdateIrnState (irnAck);

                  if (qp->IsFinished ())
                    m_queuePairRxCompleteTrace (qp);

                  // Send ACK by retransmit mode and trigger transmit
                  m_controlQueue.push (GenACK (qp, qp->m_receivedSize, irnAck, isCe));
                  m_dev->TriggerTransmit ();
                }
              else // out of order
                {
                  qp->m_receivedSize += payloadSize;
                  qp->m_irn.UpdateIrnState (irnAck);
                  // Send SACK and trigger transmit
                  m_controlQueue.push (GenSACK (qp, qp->m_receivedSize, irnAck, expectedAck, isCe));
                  m_dev->TriggerTransmit ();
                }
            }
          else if (m_l2RetransmissionMode == L2_RTX_MODE::B20 ||
                   m_l2RetransmissionMode == L2_RTX_MODE::B2N)
            {
              qp->m_b2x.m_rxMilestone = m_b2x.ackInterval;
              const uint32_t expectedSeq = qp->m_receivedSize;
              if (seq == expectedSeq)
                {
                  qp->m_receivedSize = expectedSeq + payloadSize;
                  if (qp->m_receivedSize >= qp->m_b2x.m_rxMilestone)
                    {
                      qp->m_b2x.m_rxMilestone += m_b2x.ackInterval;
                      m_controlQueue.push (GenACK (qp, qp->m_receivedSize, 0, isCe));
                      m_dev->TriggerTransmit ();
                    }
                  else if (qp->m_receivedSize % m_b2x.chunk == 0)
                    {
                      m_controlQueue.push (GenACK (qp, qp->m_receivedSize, 0, isCe));
                      m_dev->TriggerTransmit ();
                    }
                }
              else if (seq > expectedSeq)
                {
                  const Time now = Simulator::Now ();
                  if (now >= qp->m_b2x.m_nackTimer || qp->m_b2x.m_lastNack != expectedSeq)
                    {
                      qp->m_b2x.m_nackTimer = now + m_b2x.nackInterval;
                      qp->m_b2x.m_lastNack = expectedSeq;
                      if (m_l2RetransmissionMode == L2_RTX_MODE::B20)
                        qp->m_receivedSize = qp->m_receivedSize / m_b2x.chunk * m_b2x.chunk;
                      m_controlQueue.push (GenSACK (qp, qp->m_receivedSize, 0, 0, isCe));
                      m_dev->TriggerTransmit ();
                    }
                }

              if (qp->IsFinished ())
                m_queuePairRxCompleteTrace (qp);
            }

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
              // Log ack for IRN no matter using dcqcn
              qp->B2xAck (seq);
              qp->m_irn.AckIrnState (irnAck);

              // Cleanup timer for DCQCN
              if (m_ccMode == CC_MODE::DCQCN && qp->IsAckedFinished ())
                qp->m_dcqcn.CleanupTimer ();

              if (m_ccMode == CC_MODE::DCQCN && cnp && !qp->IsAckedFinished ())
                DcqcnCnpReceived (qp); // Avoid excessive DCQCN events

              m_dev->TriggerTransmit (); // Because BDP-FC needs to check new bitmap and send
              return false; // Not data so no need to send to node
            }
          else if (m_l2RetransmissionMode == L2_RTX_MODE::B20 ||
                   m_l2RetransmissionMode == L2_RTX_MODE::B2N)
            {
              if (m_b2x.ackInterval == 0)
                {
                  NS_ASSERT_MSG (false, "PfcHostPort::Receive: "
                                        "Shouldn't receive ACK");
                }
              else
                {
                  if (m_l2RetransmissionMode == L2_RTX_MODE::B2N)
                    qp->B2xAck (seq);
                  else
                    qp->B2xAck (seq / m_b2x.chunk * m_b2x.chunk);

                  if (qp->IsAckedFinished ())
                    qp->m_dcqcn.CleanupTimer ();
                }

              if (cnp && !qp->IsAckedFinished ())
                DcqcnCnpReceived (qp); // Avoid excessive DCQCN events

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
              // Log ack for IRN no matter using dcqcn
              qp->B2xAck (seq);
              // Add retransmission packets and trigger transmitting
              qp->m_irn.SackIrnState (irnAck, irnNack);

              for (auto i = irnNack; i < irnAck; i++)
                {
                  const auto qpIdx = m_txQueuePairTable[qp->GetHash ()];
                  m_rtxSeqQueues[qpIdx].push (i);
                  m_rtxQueuingCnt++;
                }

              if (m_ccMode == CC_MODE::DCQCN && cnp && !qp->IsAckedFinished ())
                DcqcnCnpReceived (qp); // Avoid excessive DCQCN events

              m_dev->TriggerTransmit ();
              return false; // Not data so no need to send to node
            }
          else if (m_l2RetransmissionMode == L2_RTX_MODE::B20 ||
                   m_l2RetransmissionMode == L2_RTX_MODE::B2N)
            {
              if (m_b2x.ackInterval == 0)
                {
                  NS_ASSERT_MSG (false, "PfcHostPort::Receive: "
                                        "Shouldn't receive ACK");
                }
              else
                {
                  if (m_l2RetransmissionMode == L2_RTX_MODE::B2N)
                    qp->B2xAck (seq);
                  else
                    qp->B2xAck (seq / m_b2x.chunk * m_b2x.chunk);

                  if (qp->IsAckedFinished ())
                    qp->m_dcqcn.CleanupTimer ();
                }

              qp->B2xRecover ();

              if (cnp && !qp->IsAckedFinished ())
                DcqcnCnpReceived (qp); // Avoid excessive DCQCN events

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
      const auto qpIdx = m_txQueuePairTable[qp->GetHash ()];
      m_rtxSeqQueues[qpIdx].push (irnSeq);
      m_rtxQueuingCnt++;

      m_dev->TriggerTransmit ();
    }
}

void
PfcHostPort::UpdateNextAvail (Ptr<RdmaTxQueuePair> qp, const Time &interframeGap,
                              const uint32_t &size)
{
  Time sendingTime;
  if (m_dcqcn.isRateBound)
    sendingTime = interframeGap + qp->m_rate.CalculateBytesTxTime (size);
  else
    sendingTime = interframeGap + qp->m_maxRate.CalculateBytesTxTime (size);
  qp->m_nextAvail = Simulator::Now () + sendingTime;
}

void
PfcHostPort::DcqcnCnpReceived (Ptr<RdmaTxQueuePair> qp)
{
  qp->m_dcqcn.m_alphaCnpArrived = true; // set CNP_arrived bit for alpha update
  qp->m_dcqcn.m_decreaseCnpArrived = true; // set CNP_arrived bit for rate decrease
  if (qp->m_dcqcn.m_firstCnp)
    {
      // init alpha
      qp->m_dcqcn.m_alpha = 1;
      qp->m_dcqcn.m_alphaCnpArrived = false;
      // schedule alpha update
      DcqcnScheduleUpdateAlpha (qp);
      // schedule rate decrease (add 1 ns to make sure rate decrease is after alpha update)
      DcqcnScheduleDecRate (qp, Time ("1ns"));
      // set rate on first CNP
      qp->m_dcqcn.m_targetRate = qp->m_rate =
          DataRate (m_dcqcn.rateFracOnFirstCnp * qp->m_rate.GetBitRate ());
      qp->m_dcqcn.m_firstCnp = false;
    }
}

void
PfcHostPort::DcqcnUpdateAlpha (Ptr<RdmaTxQueuePair> qp)
{
  if (qp->m_dcqcn.m_alphaCnpArrived)
    qp->m_dcqcn.m_alpha = (1 - m_dcqcn.g) * qp->m_dcqcn.m_alpha + m_dcqcn.g; // binary feedback
  else
    qp->m_dcqcn.m_alpha = (1 - m_dcqcn.g) * qp->m_dcqcn.m_alpha; // binary feedback
  qp->m_dcqcn.m_alphaCnpArrived = false; // clear the CNP_arrived bit
  DcqcnScheduleUpdateAlpha (qp);
}

void
PfcHostPort::DcqcnScheduleUpdateAlpha (Ptr<RdmaTxQueuePair> qp)
{
  qp->m_dcqcn.m_eventUpdateAlpha =
      Simulator::Schedule (m_dcqcn.alphaResumeInterval, &PfcHostPort::DcqcnUpdateAlpha, this, qp);
}

void
PfcHostPort::DcqcnDecRate (Ptr<RdmaTxQueuePair> qp)
{
  DcqcnScheduleDecRate (qp, Time (0));
  if (qp->m_dcqcn.m_decreaseCnpArrived)
    {
      if (m_dcqcn.clampTargetRate || qp->m_dcqcn.m_rpTimeStage != 0)
        qp->m_dcqcn.m_targetRate = qp->m_rate;
      qp->m_rate = std::max (m_dcqcn.minRate,
                             DataRate (qp->m_rate.GetBitRate () * (1 - qp->m_dcqcn.m_alpha / 2)));
      // reset rate increase related things
      qp->m_dcqcn.m_rpTimeStage = 0;
      qp->m_dcqcn.m_decreaseCnpArrived = false;
      Simulator::Cancel (qp->m_dcqcn.m_rpTimer);
      qp->m_dcqcn.m_rpTimer = Simulator::Schedule (m_dcqcn.incRateInterval,
                                                   &PfcHostPort::DcqcnScheduleIncRate, this, qp);
    }
}

void
PfcHostPort::DcqcnScheduleDecRate (Ptr<RdmaTxQueuePair> qp, const Time &delta)
{
  qp->m_dcqcn.m_eventDecreaseRate =
      Simulator::Schedule (m_dcqcn.decRateInterval + delta, &PfcHostPort::DcqcnDecRate, this, qp);
}

void
PfcHostPort::DcqcnScheduleIncRate (Ptr<RdmaTxQueuePair> qp)
{
  qp->m_dcqcn.m_rpTimer =
      Simulator::Schedule (m_dcqcn.incRateInterval, &PfcHostPort::DcqcnScheduleIncRate, this, qp);
  DcqcnIncRate (qp);
  qp->m_dcqcn.m_rpTimeStage++;
}

void
PfcHostPort::DcqcnIncRate (Ptr<RdmaTxQueuePair> qp)
{
  // check which increase phase: fast recovery, active increase, hyper increase
  if (qp->m_dcqcn.m_rpTimeStage < m_dcqcn.fastRecTimes)
    DcqcnFastRecovery (qp); // fast recovery
  else if (qp->m_dcqcn.m_rpTimeStage == m_dcqcn.fastRecTimes)
    DcqcnActiveIncrease (qp); // active increase
  else
    DcqcnHyperIncrease (qp); // hyper increase
}

void
PfcHostPort::DcqcnFastRecovery (Ptr<RdmaTxQueuePair> qp)
{
  qp->m_rate =
      DataRate ((qp->m_rate.GetBitRate () / 2) + (qp->m_dcqcn.m_targetRate.GetBitRate () / 2));
}

void
PfcHostPort::DcqcnActiveIncrease (Ptr<RdmaTxQueuePair> qp)
{
  // increate rate
  qp->m_dcqcn.m_targetRate =
      DataRate (qp->m_dcqcn.m_targetRate.GetBitRate () + m_dcqcn.rai.GetBitRate ());
  if (qp->m_dcqcn.m_targetRate > m_dev->GetDataRate ())
    qp->m_dcqcn.m_targetRate = m_dev->GetDataRate ();
  qp->m_rate =
      DataRate ((qp->m_rate.GetBitRate () / 2) + (qp->m_dcqcn.m_targetRate.GetBitRate () / 2));
}

void
PfcHostPort::DcqcnHyperIncrease (Ptr<RdmaTxQueuePair> qp)
{
  // increate rate
  qp->m_dcqcn.m_targetRate =
      DataRate (qp->m_dcqcn.m_targetRate.GetBitRate () + m_dcqcn.rhai.GetBitRate ());
  if (qp->m_dcqcn.m_targetRate > m_dev->GetDataRate ())
    qp->m_dcqcn.m_targetRate = m_dev->GetDataRate ();
  qp->m_rate =
      DataRate ((qp->m_rate.GetBitRate () / 2) + (qp->m_dcqcn.m_targetRate.GetBitRate () / 2));
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
