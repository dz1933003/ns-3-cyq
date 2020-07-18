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

#include <ns3/node.h>
#include <vector>
#include <map>
#include <sstream>

namespace ns3 {

class Packet;

/**
 * \ingroup pfc
 * \brief Queue configuration of switch memory management unit
 */
class SwitchMmuQueue
{
public:
  virtual uint64_t GetBufferSize () = 0;
  virtual uint64_t GetBufferUsed () = 0;
  virtual uint64_t GetSharedBufferUsed () = 0;
};

class PfcSwitchMmuQueue : public SwitchMmuQueue
{
public:
  uint64_t headroom = 0; //!< headroom buffer
  uint64_t reserve = 0; //!< reserve buffer

  uint64_t resumeOffset = 0;

  uint64_t ingressUsed = 0;
  uint64_t headroomUsed = 0;
  uint64_t egressUsed = 0;

public:
  uint64_t
  GetBufferSize ()
  {
    return headroom + reserve;
  }

  uint64_t
  GetBufferUsed ()
  {
    return ingressUsed + headroomUsed;
  }

  uint64_t
  GetSharedBufferUsed ()
  {
    return ingressUsed > reserve ? ingressUsed - reserve : 0;
  }
};

class CbfcSwitchMmuQueue : public SwitchMmuQueue
{
public:
  uint64_t bufferSize = 0; //!< Buffer size of a queue

  uint64_t rxFccl = 0; //!< Receiver FCCL
  uint64_t rxAbr = 0; //!< Receiver ABR

  uint64_t txFccl = 0; //!< Transmitter FCCL
  uint64_t txFctbs = 0; //!< Transmitter FCTBS

  uint64_t bufferUsed = 0; //!< Buffer used of a queue

public:
  uint64_t
  GetBufferSize ()
  {
    return bufferSize;
  }

  uint64_t
  GetBufferUsed ()
  {
    return bufferUsed;
  }

  uint64_t
  GetSharedBufferUsed ()
  {
    return 0;
  }
};

} /* namespace ns3 */

#endif /* SWITCH_MMU_QUEUE_H */
