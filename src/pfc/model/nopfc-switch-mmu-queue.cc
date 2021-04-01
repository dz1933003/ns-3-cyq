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

#include "nopfc-switch-mmu-queue.h"

#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("NoPfcSwitchMmuQueue");

NS_OBJECT_ENSURE_REGISTERED (NoPfcSwitchMmuQueue);

TypeId
NoPfcSwitchMmuQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NoPfcSwitchMmuQueue")
                          .SetParent<SwitchMmuQueue> ()
                          .SetGroupName ("Pfc")
                          .AddConstructor<NoPfcSwitchMmuQueue> ();
  return tid;
}

uint64_t
NoPfcSwitchMmuQueue::GetBufferSize ()
{
  return ingressSize;
}

uint64_t
NoPfcSwitchMmuQueue::GetBufferUsed ()
{
  return ingressUsed;
}

uint64_t
NoPfcSwitchMmuQueue::GetSharedBufferUsed ()
{
  return ingressUsed > ingressSize ? ingressUsed - ingressSize : 0;
}

void
NoPfcSwitchMmuQueue::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  SwitchMmuQueue::DoDispose ();
}

} // namespace ns3
