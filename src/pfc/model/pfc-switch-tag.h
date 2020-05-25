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

#ifndef PFC_SWITCH_TAG_H
#define PFC_SWITCH_TAG_H

#include "ns3/tag.h"

namespace ns3 {

class NetDevice;

/**
 * \ingroup pfc
 *
 * \brief tag the input device in a packet
 */
class PfcSwitchTag : public Tag
{
public:
  PfcSwitchTag ();

  /**
   * Constructs a PfcSwitchTag with the given device
   *
   * \param ifIdx device index
   */
  PfcSwitchTag (uint32_t ifIdx);

  /**
   * Sets the device for the tag
   * \param ifIdx device index
   */
  void SetInDev (uint32_t ifIdx);

  /**
   * Gets the device for the tag
   * \return index to the device
   */
  uint32_t GetInDevIdx (void) const;

  /// Inherit from parent class

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer buf) const;
  virtual void Deserialize (TagBuffer buf);
  virtual void Print (std::ostream &os) const;

private:
  uint32_t m_inDevIdx; //!< input device index
};

} // namespace ns3

#endif /* PFC_SWITCH_TAG_H */
