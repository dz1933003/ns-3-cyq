/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 * Author: Yanqing Chen  <shellqiqi@outlook.com>
 */

#ifndef DPSK_NET_DEVICE_IMPL_H
#define DPSK_NET_DEVICE_IMPL_H

#include "ns3/net-device.h"
#include "ns3/callback.h"
#include "ns3/packet.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"

namespace ns3 {

class DpskNetDevice;

/**
 * \ingroup dpsk
 * \class DpskNetDeviceImpl
 * \brief The DPSK Net Device Logic Implementation.
 *
 * This class is the base class of DPSK NetDevice logic.
 */
class DpskNetDeviceImpl : public Object
{
public:
  /**
   * \brief Get the TypeId
   *
   * \return The TypeId for this class
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a DpskNetDeviceImpl
   *
   * This is the constructor for the DpskNetDeviceImpl.
   */
  DpskNetDeviceImpl ();

  /**
   * Destroy a DpskNetDeviceImpl
   *
   * This is the destructor for the DpskNetDeviceImpl.
   */
  virtual ~DpskNetDeviceImpl ();

  /**
   * \brief Get the name of the implementation.
   * \return name
   */
  std::string GetName () const;

  /**
   * \brief Set the name of the implementation.
   * \param name the name of the implementation
   *
   * In fact, every instance of a implementation should have a unique name.
   */
  void SetName (std::string name);

protected:
  /**
   * Make protected functions of this class and its subclasses public to
   * DpskNetDevice.
   */
  friend class DpskNetDevice;

  /**
   * Transmit process
   *
   * \return Ptr to the packet.
   */
  virtual Ptr<Packet> Transmit ();

  /**
   * \param packet packet sent from above down to Network Device
   * \param source source mac address (so called "MAC spoofing")
   * \param dest mac address of the destination (already resolved)
   * \param protocolNumber identifies the type of payload contained in
   *        this packet. Used to call the right L3Protocol when the packet
   *        is received.
   *
   *  Called from higher layer to send packet into Network Device
   *  with the specified source and destination Addresses.
   *
   * \return whether the Send operation succeeded
   */
  virtual bool Send (Ptr<Packet> packet, const Address &source, const Address &dest,
                     uint16_t protocolNumber);

  /**
   * Receive process
   *
   * \param p Ptr to the received packet.
   * \return whether need to forward up.
   */
  virtual bool Receive (Ptr<Packet> p);

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

  std::string m_name; //!< Dpsk layer name
  Ptr<DpskNetDevice> m_dev; //!< Attached DpskNetDevice

private:
  /**
   * \brief Assign operator
   *
   * The method is private, so it is DISABLED.
   *
   * \param o Other DpskNetDeviceImpl
   * \return New instance of the DpskNetDeviceImpl
   */
  DpskNetDeviceImpl &operator= (const DpskNetDeviceImpl &o);

  /**
   * \brief Copy constructor
   *
   * The method is private, so it is DISABLED.

   * \param o Other DpskNetDeviceImpl
   */
  DpskNetDeviceImpl (const DpskNetDeviceImpl &o);
};

} // namespace ns3

#endif /* DPSK_NET_DEVICE_IMPL_H */
