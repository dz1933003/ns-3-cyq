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
#include "pfc-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("PfcHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (PfcHeader);

PfcHeader::PfcHeader (uint32_t type, uint32_t qIndex) : PfcHeader (type, qIndex, 0)
{
  NS_LOG_FUNCTION (type << qIndex);
}

PfcHeader::PfcHeader (uint32_t type, uint32_t qIndex, uint16_t time)
    : m_type (type), m_qIndex (qIndex), m_time (time)
{
  NS_LOG_FUNCTION (type << qIndex << time);
}

PfcHeader::PfcHeader () : PfcHeader (0, 0, 0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
PfcHeader::SetType (uint32_t type)
{
  NS_LOG_FUNCTION (type);
  m_type = type;
}

uint32_t
PfcHeader::GetType () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_type;
}

void
PfcHeader::SetQIndex (uint32_t qIndex)
{
  NS_LOG_FUNCTION (qIndex);
  m_qIndex = qIndex;
}

uint32_t
PfcHeader::GetQIndex () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_qIndex;
}

void
PfcHeader::SetTime (uint16_t time)
{
  NS_LOG_FUNCTION (time);
  m_time = time;
}

uint16_t
PfcHeader::GetTime (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_time;
}

TypeId
PfcHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PfcHeader").SetParent<Header> ().AddConstructor<PfcHeader> ();
  return tid;
}

TypeId
PfcHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
PfcHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (&os);
  os << "pause=" << PfcTypeToString (m_type) << ", queue=" << m_qIndex << ", time=" << m_time;
}

uint32_t
PfcHeader::GetSerializedSize (void) const
{
  return sizeof (m_type) + sizeof (m_qIndex) + sizeof (m_time);
}

void
PfcHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (&start);
  start.WriteU32 (m_type);
  start.WriteU32 (m_qIndex);
  start.WriteU16 (m_time);
}

uint32_t
PfcHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (&start);
  m_type = start.ReadU32 ();
  m_qIndex = start.ReadU32 ();
  m_time = start.ReadU16 ();
  return GetSerializedSize ();
}

std::string
PfcHeader::PfcTypeToString (const uint32_t &type) const
{
  switch (type)
    {
    case PfcType::Pause:
      return "Pause";
    case PfcType::Resume:
      return "Resume";
    default:
      NS_ASSERT_MSG (false, "PfcHeader::PfcTypeToString: Invalid type");
      return "";
    }
}

}; // namespace ns3
