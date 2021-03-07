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

#ifndef QBB_HEADER_H
#define QBB_HEADER_H

#include <stdint.h>
#include "ns3/header.h"
#include "ns3/buffer.h"

namespace ns3 {

/**
 * \ingroup pfc
 * \brief Header for the 802.1qbb frame
 *
 * Add seq and flags to UDP Header.
 */
class QbbHeader : public Header
{
public:
  /**
   * \brief Construct a null Qbb header
   */
  QbbHeader (void);

  /**
   * \return The source port for this header
   */
  uint16_t GetSourcePort (void) const;

  /**
   * \param port The source port for this header
   */
  void SetSourcePort (uint16_t port);

  /**
   * \return the destination port for this header
   */
  uint16_t GetDestinationPort (void) const;

  /**
   * \param port the destination port for this header
   */
  void SetDestinationPort (uint16_t port);

  // TODO cyq: Add comments
  uint32_t GetSequenceNumber (void) const;
  void SetSequenceNumber (uint32_t sequenceNumber);

  /**
   * \return the sequence number for this header
   */
  uint32_t GetIrnAckNumber (void) const;

  /**
   * \brief Set the sequence Number
   * \param sequenceNumber the sequence number for this header
   */
  void SetIrnAckNumber (uint32_t sequenceNumber);

  /**
   * \return the ACK number for this header
   */
  uint32_t GetIrnNackNumber (void) const;

  /**
   * \brief Set the ACK Number
   * \param ackNumber the ACK number for this header
   */
  void SetIrnNackNumber (uint32_t ackNumber);

  /**
   * Flag types.
   */
  enum QbbFlag { NONE, ACK, SACK };

  /**
   * \brief Get the flags
   * \return the flags for this header
   */
  uint8_t GetFlags (void) const;

  /**
   * \brief Set flags of the header
   * \param flags the flags for this header
   */
  void SetFlags (uint8_t flags);

  /**
   * \brief Qbb Flag to string.
   * \param flags the flags for this header
   * \return name of the flags
   */
  static std::string FlagsToString (const uint8_t &flags);

  // TODO cyq: Add comments
  bool GetCnp (void) const;
  void SetCnp (bool cnp);

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

private:
  uint16_t m_sourcePort = 0; //!< Source port
  uint16_t m_destinationPort = 0; //!< Destination port

  uint32_t m_sequenceNumber = 0; //!< Sequence Number (for DCQCN)

  uint32_t m_irnAckNumber = 0; //!< IRN ACK/SACK sequence Number
  uint32_t m_irnNackNumber = 0; //!< IRN NACK sequence number

  uint8_t m_flags = NONE; //!< NONE/ACK/SACK
  uint8_t m_cnp = 0; //!< 0 or 1
};

}; // namespace ns3

#endif /* QBB_HEADER_H */
