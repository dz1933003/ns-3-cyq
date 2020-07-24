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

#ifndef PFC_HEADER_H
#define PFC_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"

namespace ns3 {

/**
 * \ingroup pfc
 * \brief Header for the PFC frame
 *
 * Referenced to IEEE 802.1 Qbb but implement only queue index with
 * the type of the PFC.
 */
class PfcHeader : public Header
{
public:
  /**
   * \brief Construct a null PFC header
   * \param type Pause or Resume
   * \param qIndex target queue index
   */
  PfcHeader (uint32_t type, uint32_t qIndex);

  /**
   * \brief Construct a null PFC header
   * \param type Pause or Resume
   * \param qIndex target queue index
   * \param time pause time
   */
  PfcHeader (uint32_t type, uint32_t qIndex, uint16_t time);

  /**
   * \brief Construct a null PFC header
   */
  PfcHeader (void);

  /**
   * \param type Pause or Resume
   */
  void SetType (uint32_t type);

  /**
   * \return PFC type
   */
  uint32_t GetType (void) const;

  /**
   * \param qIndex target queue index
   */
  void SetQIndex (uint32_t qIndex);

  /**
   * \return target queue index
   */
  uint32_t GetQIndex (void) const;

  /**
   * \param time pause time
   */
  void SetTime (uint16_t time);

  /**
   * \return pause time
   */
  uint16_t GetTime (void) const;

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

  /**
   * \brief PFC type enumeration.
   */
  enum PfcType { Pause, Resume };

  const static uint16_t PROT_NUM = 0x8808;

  /**
   * Get string of PFC type
   *
   * \return string of PFC type.
   */
  std::string PfcTypeToString (const uint32_t &type) const;

private:
  uint32_t m_type; //!< PFC type
  uint32_t m_qIndex; //!< target queue index
  uint16_t m_time; //!< pause time on this queue
};

}; // namespace ns3

#endif /* PFC_HEADER_H */
