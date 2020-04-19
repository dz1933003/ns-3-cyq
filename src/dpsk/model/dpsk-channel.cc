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

#include "dpsk-channel.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DpskChannel");

NS_OBJECT_ENSURE_REGISTERED (DpskChannel);

TypeId
DpskChannel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DpskChannel")
                          .SetParent<Channel> ()
                          .SetGroupName ("Dpsk")
                          .AddConstructor<DpskChannel> ();
  return tid;
}

DpskChannel::DpskChannel () : Channel ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

DpskChannel::~DpskChannel ()
{
  NS_LOG_FUNCTION_NOARGS ();

  for (std::vector<Ptr<Channel>>::iterator iter = m_dpskChannels.begin ();
       iter != m_dpskChannels.end (); iter++)
    {
      *iter = NULL;
    }
  m_dpskChannels.clear ();
}

void
DpskChannel::AddChannel (Ptr<Channel> channel)
{
  NS_LOG_FUNCTION (channel);
  m_dpskChannels.push_back (channel);
}

std::vector<Ptr<Channel>>
DpskChannel::GetChannels (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_dpskChannels;
}

std::size_t
DpskChannel::GetNDevices (void) const
{
  uint32_t nDevices = 0;
  for (std::vector<Ptr<Channel>>::const_iterator iter = m_dpskChannels.begin ();
       iter != m_dpskChannels.end (); iter++)
    {
      nDevices += (*iter)->GetNDevices ();
    }
  return nDevices;
}

Ptr<NetDevice>
DpskChannel::GetDevice (std::size_t i) const
{
  return NULL;
}

} // namespace ns3