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
#ifndef DPSK_CHANNEL_H
#define DPSK_CHANNEL_H

#include "ns3/net-device.h"
#include "ns3/channel.h"
#include <vector>

namespace ns3 {

/**
 * \ingroup dpsk
 *
 * \brief Channel manager for DPSK.
 *
 * Just like DPSK aggregates multiple NetDevices,
 * DPSKChannel aggregates multiple channels.
 */
class DPSKChannel : public Channel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  DPSKChannel ();
  virtual ~DPSKChannel ();

  /**
   * Adds a channel to the DPSK channel pool
   * \param channel the channel to add to the pool
   */
  void AddChannel (Ptr<Channel> channel);

  /**
   * Get channels in the DPSK channel pool
   * \param channels in the DPSK channel pool
   */
  std::vector<Ptr<Channel>> GetChannels (void);

  // Inherited from Channel base class

  virtual std::size_t GetNDevices (void) const;
  virtual Ptr<NetDevice> GetDevice (std::size_t i) const;

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  DPSKChannel (const DPSKChannel &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  DPSKChannel &operator= (const DPSKChannel &);

  std::vector<Ptr<Channel>> m_dpskChannels; //!< pool of DPSK managed channels
};

} // namespace ns3

#endif /* DPSK_CHANNEL_H */
