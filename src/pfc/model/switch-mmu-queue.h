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

#ifndef SWITCH_MMU_QUEUE_H
#define SWITCH_MMU_QUEUE_H

#include <ns3/object.h>

namespace ns3 {

/**
 * \ingroup pfc
 * \brief Queue configuration of switch memory management unit
 */
class SwitchMmuQueue : public Object
{
public:
  uint64_t egressUsed = 0;

public:
  static TypeId GetTypeId ();

  virtual uint64_t GetBufferSize ();
  virtual uint64_t GetBufferUsed ();
  virtual uint64_t GetSharedBufferUsed ();

  virtual void DoDispose ();
};

} /* namespace ns3 */

#endif /* SWITCH_MMU_QUEUE_H */
