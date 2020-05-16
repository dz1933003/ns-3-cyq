/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "pfc-switch-tag.h"
#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/pointer.h"
#include <bits/wordsize.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PfcSwitchTag");

NS_OBJECT_ENSURE_REGISTERED (PfcSwitchTag);

TypeId
PfcSwitchTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PfcSwitchTag")
                          .SetParent<Tag> ()
                          .SetGroupName ("Pfc")
                          .AddConstructor<PfcSwitchTag> ();
  return tid;
}

TypeId
PfcSwitchTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
PfcSwitchTag::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return sizeof (m_inDev);
}

void
PfcSwitchTag::Serialize (TagBuffer buf) const
{
  NS_LOG_FUNCTION (this << &buf);
#if __WORDSIZE == 64
  buf.WriteU64 ((uint64_t) & (*m_inDev));
#elif __WORDSIZE == 32
  buf.WriteU32 ((uint32_t) & (*m_inDev));
#else
  NS_ASSERT_MSG (false, "Unrecognized machine word length");
#endif
}

void
PfcSwitchTag::Deserialize (TagBuffer buf)
{
  NS_LOG_FUNCTION (this << &buf);
#if __WORDSIZE == 64
  m_inDev = (NetDevice *) buf.ReadU64 ();
#elif __WORDSIZE == 32
  m_inDev = (NetDevice *) buf.ReadU32 ();
#else
  NS_ASSERT_MSG (false, "Unrecognized machine word length");
#endif
}

void
PfcSwitchTag::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "Input Device: " << m_inDev;
}

PfcSwitchTag::PfcSwitchTag () : Tag (), m_inDev (0)
{
  NS_LOG_FUNCTION (this);
}

PfcSwitchTag::PfcSwitchTag (Ptr<NetDevice> device) : Tag (), m_inDev (device)
{
  NS_LOG_FUNCTION (this << device);
}

void
PfcSwitchTag::SetInDev (Ptr<NetDevice> device)
{
  NS_LOG_FUNCTION (this << device);
  m_inDev = device;
}

Ptr<NetDevice>
PfcSwitchTag::GetInDev (void) const
{
  NS_LOG_FUNCTION (this);
  return m_inDev;
}

} // namespace ns3
