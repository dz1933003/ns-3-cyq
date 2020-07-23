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
#include "cbfc-header.h"
#include "ns3/buffer.h"
#include "ns3/address-utils.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("CbfcHeader");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (CbfcHeader);

CbfcHeader::CbfcHeader (uint64_t fccl, uint32_t qIndex) : m_fccl (fccl), m_qIndex (qIndex)
{
  NS_LOG_FUNCTION (fccl << qIndex);
}

CbfcHeader::CbfcHeader () : m_fccl (0), m_qIndex (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
CbfcHeader::SetFccl (uint64_t fccl)
{
  NS_LOG_FUNCTION (fccl);
  m_fccl = fccl;
}

uint64_t
CbfcHeader::GetFccl () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_fccl;
}

void
CbfcHeader::SetQIndex (uint32_t qIndex)
{
  NS_LOG_FUNCTION (qIndex);
  m_qIndex = qIndex;
}

uint32_t
CbfcHeader::GetQIndex () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_qIndex;
}

TypeId
CbfcHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::CbfcHeader").SetParent<Header> ().AddConstructor<CbfcHeader> ();
  return tid;
}

TypeId
CbfcHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
CbfcHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (&os);
  os << "fccl=" << m_fccl << ", queue=" << m_qIndex;
}

uint32_t
CbfcHeader::GetSerializedSize (void) const
{
  return sizeof (m_fccl) + sizeof (m_qIndex);
}

void
CbfcHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (&start);
  start.WriteU64 (m_fccl);
  start.WriteU32 (m_qIndex);
}

uint32_t
CbfcHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (&start);
  m_fccl = start.ReadU64 ();
  m_qIndex = start.ReadU32 ();
  return GetSerializedSize ();
}

}; // namespace ns3
