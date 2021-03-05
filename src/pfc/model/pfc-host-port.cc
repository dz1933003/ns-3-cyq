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
      m_l2RetransmissionMode (L2_RTX_MODE::NONE),
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
    return NONE;
  else if (mode == "IRN")
    return IRN;
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
  while (m_rtxPacketQueue.empty () == false)
    {
      const auto qp = m_rtxPacketQueue.front ().first;
      const auto irnSeq = m_rtxPacketQueue.front ().second;
      const auto state = qp->m_irn.GetIrnState (irnSeq);
      m_rtxPacketQueue.pop_front ();
      // Set up IRN timer
      if (m_l2RetransmissionMode == IRN)
        {
          // filter out ACKed packets
          if (state == RdmaTxQueuePair::NACK || state == RdmaTxQueuePair::UNACK)
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
          qp->IsFinished () == false && qp->m_startTime <= Simulator::Now () &&
          (m_l2RetransmissionMode != IRN || qp->m_irn.GetWindowSize () < m_irn.maxBitmapSize))
        {
          if (isDcqcn)
            {
              if (qp->GetRemainBytes () > 0 && !qp->IsWinBound () && !qp->IsFinishedSend ())
                {
                  if (qp->m_nextAvail > Simulator::Now ()) // Not available now
                    continue;
                  m_lastQpIndex = qIdx;
                  auto p = GenData (qp);
                  // TODO cyq: Not finished here!
                  if (qp->IsFinishedSend ())
                    m_queuePairTxCompleteTrace (qp);
                  m_nTxBytes += p->GetSize ();
                  PktSent (qp, p, Time (0));
                  return p;
                }
            }
          else
            {
              m_lastQpIndex = qIdx;
              uint32_t irnSeq;
              auto p = GenData (qp, irnSeq); // get sequence number out
              if (qp->IsFinished ())
                m_queuePairTxCompleteTrace (qp);
              m_nTxBytes += p->GetSize ();
              // Set up IRN timer
              if (m_l2RetransmissionMode == IRN)
                {
                  auto id = IrnTimer (qp, irnSeq);
                  qp->m_irn.SetRtxEvent (irnSeq, id);
                }
              return p;
            }
        }
    }

  // No packet to send

  Time t = Simulator::GetMaximumSimulationTime ();
  for (uint32_t i = 0; i < m_txQueuePairs.size (); i++)
    {
      Ptr<RdmaTxQueuePair> qp = m_txQueuePairs[i];
      if (qp->IsFinished ())
        continue;
      t = Min (qp->m_nextAvail, t);
    }
  if (m_nextSend.IsExpired () && t < Simulator::GetMaximumSimulationTime () &&
      t > Simulator::Now ())
    {
      m_nextSend =
          Simulator::Schedule (t - Simulator::Now (), &DpskNetDevice::TriggerTransmit, m_dev);
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

          if (isDcqcn)
            {
              qp->m_b2n_0.m_milestone_rx = m_ack_interval;
              int x = ReceiverCheckSeq (seq, qp, payloadSize);
              if (x == 1 || x == 2)
                {
                  Ptr<Packet> p = Create<Packet> (0);

                  QbbHeader qbb;
                  qbb.SetSourcePort (qp->m_dPort); // exchange ports
                  qbb.SetDestinationPort (qp->m_sPort);
                  qbb.SetSequenceNumber (qp->m_receivedSize);
                  qbb.SetCnp (ecn == Ipv4Header::EcnType::ECN_CE);
                  qbb.SetFlags (x == 1 ? QbbHeader::ACK : QbbHeader::SACK);
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

                  m_controlQueue.push (p);
                  m_dev->TriggerTransmit ();
                }
            }
          else
            {
              // retransmission and rx byte count
              if (m_l2RetransmissionMode == NONE)
                {
                  qp->m_receivedSize += payloadSize;
                }
              else if (m_l2RetransmissionMode == IRN)
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
                      m_controlQueue.push (GenACK (qp, irnAck));
                      m_dev->TriggerTransmit ();
                    }
                  else if (irnAck == expectedAck) // expected new packet
                    {
                      qp->m_receivedSize += payloadSize;
                      qp->m_irn.UpdateIrnState (irnAck);
                      // Send ACK by retransmit mode and trigger transmit
                      m_controlQueue.push (GenACK (qp, irnAck));
                      m_dev->TriggerTransmit ();
                    }
                  else // out of order
                    {
                      qp->m_receivedSize += payloadSize;
                      qp->m_irn.UpdateIrnState (irnAck);
                      // Send SACK and trigger transmit
                      m_controlQueue.push (GenSACK (qp, irnAck, expectedAck));
                      m_dev->TriggerTransmit ();
                    }
                }
              else
                {
                  NS_ASSERT_MSG (false, "PfcSwitchPort::Rx: Invalid Qbb packet type");
                  return false; // Drop this packet
                }
            }
          // TODO cyq: Reduce duplicate fin check
          if (qp->IsFinished ())
            m_queuePairRxCompleteTrace (qp);
          return true; // Forward up to node
        }
      // ACK packet
      else if (flags == QbbHeader::ACK)
        {
          // Handle ACK
          uint32_t key = RdmaTxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          uint32_t index = m_txQueuePairTable[key];
          Ptr<RdmaTxQueuePair> qp = m_txQueuePairs[index];
          if (isDcqcn)
            {
              if (m_ack_interval == 0)
                std::cout << "ERROR: shouldn't receive ack\n";
              else
                {
                  if (!m_backto0)
                    {
                      qp->Acknowledge (seq);
                    }
                  else
                    {
                      uint32_t goback_seq = seq / m_chunk * m_chunk;
                      qp->Acknowledge (goback_seq);
                    }
                  if (qp->IsFinished ())
                    {
                      QpComplete (qp);
                    }
                }
              // if (ch.l3Prot == 0xFD) // NACK
              //   RecoverQueue (qp);
              // handle cnp
              if (cnp)
                {
                  // mlx version
                  cnp_received_mlx (qp);
                }
              // ACK may advance the on-the-fly window, allowing more packets to send
              m_dev->TriggerTransmit ();
              return false;
            }
          else
            {
              qp->m_irn.AckIrnState (irnAck);
              m_dev->TriggerTransmit (); // Because BDP-FC needs to check new bitmap and send
              return false; // Not data so no need to send to node
            }
        }
      else if (flags == QbbHeader::SACK)
        {
          // Handle SACK
          uint32_t key = RdmaTxQueuePair::GetHash (sIp, dIp, sPort, dPort);
          uint32_t index = m_txQueuePairTable[key];
          Ptr<RdmaTxQueuePair> qp = m_txQueuePairs[index];
          if (isDcqcn)
            {
              if (m_ack_interval == 0)
                std::cout << "ERROR: shouldn't receive ack\n";
              else
                {
                  if (!m_backto0)
                    {
                      qp->Acknowledge (seq);
                    }
                  else
                    {
                      uint32_t goback_seq = seq / m_chunk * m_chunk;
                      qp->Acknowledge (goback_seq);
                    }
                  if (qp->IsFinished ())
                    {
                      QpComplete (qp);
                    }
                }
              // if (ch.l3Prot == 0xFD) // NACK
              RecoverQueue (qp);
              // handle cnp
              if (cnp)
                {
                  // mlx version
                  cnp_received_mlx (qp);
                }
              // ACK may advance the on-the-fly window, allowing more packets to send
              m_dev->TriggerTransmit ();
              return false;
            }
          else
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

  qp->m_sentSize += payloadSize;

  Ptr<Packet> p = Create<Packet> (payloadSize);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_sPort);
  qbb.SetDestinationPort (qp->m_dPort);
  if (m_l2RetransmissionMode == IRN)
    {
      const auto seq = qp->m_irn.GetNextSequenceNumber ();
      o_irnSeq = seq;
      qbb.SetIrnAckNumber (seq);
      qbb.SetIrnNackNumber (0);
      qbb.SetFlags (QbbHeader::NONE);
      qp->m_irn.SendNewPacket (payloadSize); // Update IRN infos
    }
  if (isDcqcn)
    qbb.SetSequenceNumber (qp->snd_nxt);
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

  if (isDcqcn)
    qp->snd_nxt += payloadSize;

  return p;
}

Ptr<Packet>
PfcHostPort::GenACK (Ptr<RdmaRxQueuePair> qp, uint32_t irnAck)
{
  NS_LOG_FUNCTION (qp);

  Ptr<Packet> p = Create<Packet> (0);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_dPort); // exchange ports
  qbb.SetDestinationPort (qp->m_sPort);
  qbb.SetIrnAckNumber (irnAck);
  qbb.SetIrnNackNumber (0);
  qbb.SetFlags (QbbHeader::ACK);
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
PfcHostPort::GenSACK (Ptr<RdmaRxQueuePair> qp, uint32_t irnAck, uint32_t irnNack)
{
  NS_LOG_FUNCTION (qp);

  Ptr<Packet> p = Create<Packet> (0);

  QbbHeader qbb;
  qbb.SetSourcePort (qp->m_dPort); // exchange ports
  qbb.SetDestinationPort (qp->m_sPort);
  qbb.SetIrnAckNumber (irnAck);
  qbb.SetIrnNackNumber (irnNack);
  qbb.SetFlags (QbbHeader::SACK);
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

int
PfcHostPort::ReceiverCheckSeq (uint32_t seq, Ptr<RdmaRxQueuePair> q, uint32_t size)
{
  uint32_t expected = q->m_receivedSize;
  if (seq == expected)
    {
      q->m_receivedSize = expected + size;
      if (q->m_receivedSize >= q->m_b2n_0.m_milestone_rx)
        {
          q->m_b2n_0.m_milestone_rx += m_ack_interval;
          return 1; //Generate ACK
        }
      else if (q->m_receivedSize % m_chunk == 0)
        {
          return 1;
        }
      else
        {
          return 5;
        }
    }
  else if (seq > expected)
    {
      // Generate NACK
      if (Simulator::Now () >= q->m_b2n_0.m_nackTimer || q->m_b2n_0.m_lastNACK != expected)
        {
          q->m_b2n_0.m_nackTimer = Simulator::Now () + MicroSeconds (m_nack_interval);
          q->m_b2n_0.m_lastNACK = expected;
          if (m_backto0)
            {
              q->m_receivedSize = q->m_receivedSize / m_chunk * m_chunk;
            }
          return 2;
        }
      else
        return 4;
    }
  else
    {
      // Duplicate.
      return 3;
    }
}

void
PfcHostPort::PktSent (Ptr<RdmaTxQueuePair> qp, Ptr<Packet> pkt, Time interframeGap)
{
  qp->lastPktSize = pkt->GetSize ();
  UpdateNextAvail (qp, interframeGap, pkt->GetSize ());
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
  qp->snd_nxt = qp->snd_una;
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
PfcHostPort::UpdateAlphaMlx (Ptr<RdmaTxQueuePair> q)
{
  if (q->mlx.m_alpha_cnp_arrived)
    {
      q->mlx.m_alpha = (1 - m_g) * q->mlx.m_alpha + m_g; //binary feedback
    }
  else
    {
      q->mlx.m_alpha = (1 - m_g) * q->mlx.m_alpha; //binary feedback
    }
  q->mlx.m_alpha_cnp_arrived = false; // clear the CNP_arrived bit
  ScheduleUpdateAlphaMlx (q);
}

void
PfcHostPort::ScheduleUpdateAlphaMlx (Ptr<RdmaTxQueuePair> q)
{
  q->mlx.m_eventUpdateAlpha = Simulator::Schedule (MicroSeconds (m_alpha_resume_interval),
                                                   &PfcHostPort::UpdateAlphaMlx, this, q);
}

void
PfcHostPort::cnp_received_mlx (Ptr<RdmaTxQueuePair> q)
{
  q->mlx.m_alpha_cnp_arrived = true; // set CNP_arrived bit for alpha update
  q->mlx.m_decrease_cnp_arrived = true; // set CNP_arrived bit for rate decrease
  if (q->mlx.m_first_cnp)
    {
      // init alpha
      q->mlx.m_alpha = 1;
      q->mlx.m_alpha_cnp_arrived = false;
      // schedule alpha update
      ScheduleUpdateAlphaMlx (q);
      // schedule rate decrease
      ScheduleDecreaseRateMlx (q, 1); // add 1 ns to make sure rate decrease is after alpha update
      // set rate on first CNP
      q->mlx.m_targetRate = q->m_rate = DataRate (m_rateOnFirstCNP * q->m_rate.GetBitRate ());
      q->mlx.m_first_cnp = false;
    }
}

void
PfcHostPort::CheckRateDecreaseMlx (Ptr<RdmaTxQueuePair> q)
{
  ScheduleDecreaseRateMlx (q, 0);
  if (q->mlx.m_decrease_cnp_arrived)
    {
      bool clamp = true;
      if (!m_EcnClampTgtRate)
        {
          if (q->mlx.m_rpTimeStage == 0)
            clamp = false;
        }
      if (clamp)
        q->mlx.m_targetRate = q->m_rate;
      q->m_rate =
          std::max (m_minRate, DataRate (q->m_rate.GetBitRate () * (1 - q->mlx.m_alpha / 2)));
      // reset rate increase related things
      q->mlx.m_rpTimeStage = 0;
      q->mlx.m_decrease_cnp_arrived = false;
      Simulator::Cancel (q->mlx.m_rpTimer);
      q->mlx.m_rpTimer = Simulator::Schedule (MicroSeconds (m_rpgTimeReset),
                                              &PfcHostPort::RateIncEventTimerMlx, this, q);
    }
}

void
PfcHostPort::ScheduleDecreaseRateMlx (Ptr<RdmaTxQueuePair> q, uint32_t delta)
{
  q->mlx.m_eventDecreaseRate =
      Simulator::Schedule (MicroSeconds (m_rateDecreaseInterval) + NanoSeconds (delta),
                           &PfcHostPort::CheckRateDecreaseMlx, this, q);
}

void
PfcHostPort::RateIncEventTimerMlx (Ptr<RdmaTxQueuePair> q)
{
  q->mlx.m_rpTimer = Simulator::Schedule (MicroSeconds (m_rpgTimeReset),
                                          &PfcHostPort::RateIncEventTimerMlx, this, q);
  RateIncEventMlx (q);
  q->mlx.m_rpTimeStage++;
}

void
PfcHostPort::RateIncEventMlx (Ptr<RdmaTxQueuePair> q)
{
  // check which increase phase: fast recovery, active increase, hyper increase
  if (q->mlx.m_rpTimeStage < m_rpgThreshold)
    { // fast recovery
      FastRecoveryMlx (q);
    }
  else if (q->mlx.m_rpTimeStage == m_rpgThreshold)
    { // active increase
      ActiveIncreaseMlx (q);
    }
  else
    { // hyper increase
      HyperIncreaseMlx (q);
    }
}

void
PfcHostPort::FastRecoveryMlx (Ptr<RdmaTxQueuePair> q)
{
  q->m_rate = DataRate ((q->m_rate.GetBitRate () / 2) + (q->mlx.m_targetRate.GetBitRate () / 2));
}

void
PfcHostPort::ActiveIncreaseMlx (Ptr<RdmaTxQueuePair> q)
{
  // increate rate
  q->mlx.m_targetRate = DataRate (q->mlx.m_targetRate.GetBitRate () + m_rai.GetBitRate ());
  if (q->mlx.m_targetRate > m_dev->GetDataRate ())
    q->mlx.m_targetRate = m_dev->GetDataRate ();
  q->m_rate = DataRate ((q->m_rate.GetBitRate () / 2) + (q->mlx.m_targetRate.GetBitRate () / 2));
}

void
PfcHostPort::HyperIncreaseMlx (Ptr<RdmaTxQueuePair> q)
{
  // increate rate
  q->mlx.m_targetRate = DataRate (q->mlx.m_targetRate.GetBitRate () + m_rhai.GetBitRate ());
  if (q->mlx.m_targetRate > m_dev->GetDataRate ())
    q->mlx.m_targetRate = m_dev->GetDataRate ();
  q->m_rate = DataRate ((q->m_rate.GetBitRate () / 2) + (q->mlx.m_targetRate.GetBitRate () / 2));
}

} // namespace ns3
