/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
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
 * Modified: Yanqing Chen  <shellqiqi@outlook.com>
 */

#include <stdint.h>
#include <iostream>
#include "qbb-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("QbbHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (QbbHeader);

QbbHeader::QbbHeader ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

uint16_t
QbbHeader::GetSourcePort (void) const
{
  return m_sourcePort;
}

void
QbbHeader::SetSourcePort (uint16_t port)
{
  m_sourcePort = port;
}

uint16_t
QbbHeader::GetDestinationPort (void) const
{
  return m_destinationPort;
}

void
QbbHeader::SetDestinationPort (uint16_t port)
{
  m_destinationPort = port;
}

uint32_t
QbbHeader::GetIrnAckNumber (void) const
{
  return m_irnAckNumber;
}

void
QbbHeader::SetIrnAckNumber (uint32_t sequenceNumber)
{
  m_irnAckNumber = sequenceNumber;
}

uint32_t
QbbHeader::GetIrnNackNumber (void) const
{
  return m_irnNackNumber;
}

void
QbbHeader::SetIrnNackNumber (uint32_t ackNumber)
{
  m_irnNackNumber = ackNumber;
}

uint32_t
QbbHeader::GetSeqNumber (void) const
{
  return m_sequenceNumber;
}

void
QbbHeader::SetSeqNumber (uint32_t seq)
{
  m_sequenceNumber = seq;
}

uint8_t
QbbHeader::GetFlags (void) const
{
  return m_flags;
}

void
QbbHeader::SetFlags (uint8_t flags)
{
  m_flags = flags;
}

uint8_t
QbbHeader::GetCnpFlags (void) const
{
  return m_cnpFlags;
}

void
QbbHeader::SetCnpFlags (uint8_t flags)
{
  m_cnpFlags = flags;
}

std::string
QbbHeader::FlagsToString (const uint8_t &flags)
{
  switch (flags)
    {
    case NONE:
      return "NONE";
    case ACK:
      return "ACK";
    case SACK:
      return "SACK";
    case NACK:
      return "NACK";
    default:
      return "UNKNOWN";
    }
}

std::string
QbbHeader::CnpFlagsToString (const uint8_t &flags)
{
  switch (flags)
    {
    case UNCNP:
      return "UNCNP";
    case CNP:
      return "CNP";
    default:
      return "UNKNOWN";
    }
}

TypeId
QbbHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QbbHeader").SetParent<Header> ().AddConstructor<QbbHeader> ();
  return tid;
}

TypeId
QbbHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
QbbHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (&os);
  os << FlagsToString (m_flags) << CnpFlagsToString (m_cnpFlags) << " IrnAck=" << m_irnAckNumber
     << " IrnNack=" << m_irnNackNumber << " " << m_sourcePort << " > " << m_destinationPort;
}

uint32_t
QbbHeader::GetSerializedSize (void) const
{
  return sizeof (m_sourcePort) + sizeof (m_destinationPort) + sizeof (m_irnAckNumber) +
         sizeof (m_irnNackNumber) + sizeof (m_sequenceNumber) + sizeof (m_flags) +
         sizeof (m_cnpFlags);
}

void
QbbHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (&start);
  start.WriteU16 (m_sourcePort);
  start.WriteU16 (m_destinationPort);
  start.WriteU32 (m_irnAckNumber);
  start.WriteU32 (m_irnNackNumber);
  start.WriteU32 (m_sequenceNumber);
  start.WriteU8 (m_flags);
  start.WriteU8 (m_cnpFlags);
}

uint32_t
QbbHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (&start);
  m_sourcePort = start.ReadU16 ();
  m_destinationPort = start.ReadU16 ();
  m_irnAckNumber = start.ReadU32 ();
  m_irnNackNumber = start.ReadU32 ();
  m_sequenceNumber = start.ReadU32 ();
  m_flags = start.ReadU8 ();
  m_cnpFlags = start.ReadU8 ();
  return GetSerializedSize ();
}

}; // namespace ns3
