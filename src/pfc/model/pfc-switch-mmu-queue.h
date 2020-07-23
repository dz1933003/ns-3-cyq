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

#ifndef PFC_SWITCH_MMU_QUEUE_H
#define PFC_SWITCH_MMU_QUEUE_H

#include <ns3/object.h>

#include "switch-mmu-queue.h"

namespace ns3 {

/**
 * \ingroup pfc
 * \brief PFC queue configuration of switch memory management unit
 */
class PfcSwitchMmuQueue : public SwitchMmuQueue
{
public:
  uint64_t headroom = 0; //!< headroom buffer
  uint64_t reserve = 0; //!< reserve buffer

  uint64_t resumeOffset = 0;

  uint64_t ingressUsed = 0;
  uint64_t headroomUsed = 0;

  bool isPaused = false; //!< true: pause

public:
  static TypeId GetTypeId ();

  virtual uint64_t GetBufferSize ();
  virtual uint64_t GetBufferUsed ();
  virtual uint64_t GetSharedBufferUsed ();

  virtual void DoDispose ();
};

} /* namespace ns3 */

#endif /* PFC_SWITCH_MMU_QUEUE_H */
