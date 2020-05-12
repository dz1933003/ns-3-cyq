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

PfcHeader::PfcHeader (uint32_t type, uint32_t qIndex) : m_type (type), m_qIndex (qIndex)
{
  NS_LOG_FUNCTION (type << qIndex);
}

PfcHeader::PfcHeader () : m_type (0), m_qIndex (0)
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
  os << "pause=" << PfcTypeToString (m_type) << ", queue=" << m_qIndex;
}

uint32_t
PfcHeader::GetSerializedSize (void) const
{
  return sizeof (m_type) + sizeof (m_qIndex);
}

void
PfcHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (&start);
  start.WriteU32 (m_type);
  start.WriteU32 (m_qIndex);
}

uint32_t
PfcHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (&start);
  m_type = start.ReadU32 ();
  m_qIndex = start.ReadU32 ();
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
