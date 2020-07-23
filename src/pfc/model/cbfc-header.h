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

#ifndef CBFC_HEADER_H
#define CBFC_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"

namespace ns3 {

/**
 * \ingroup pfc
 * \brief Header for the CBFC frame
 *
 * Referenced to credit based flow control of Infiniband.
 */
class CbfcHeader : public Header
{
public:
  /**
   * \brief Construct a null CBFC header
   * \param fccl feedback message
   * \param qIndex target queue index
   */
  CbfcHeader (uint64_t fccl, uint32_t qIndex);

  /**
   * \brief Construct a null CBFC header
   */
  CbfcHeader (void);

  /**
   * \param fccl FCCL in bytes
   */
  void SetFccl (uint64_t fccl);

  /**
   * \param qIndex target queue index
   */
  void SetQIndex (uint32_t qIndex);

  /**
   * \return FCCL in bytes
   */
  uint64_t GetFccl (void) const;

  /**
   * \return target queue index
   */
  uint32_t GetQIndex (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  const static uint16_t PROT_NUM = 0x8808;

private:
  uint64_t m_fccl; //!< FCCL in bytes
  uint32_t m_qIndex; //!< target queue index
};

}; // namespace ns3

#endif /* CBFC_HEADER_H */
