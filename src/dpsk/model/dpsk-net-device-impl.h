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
   * Attach the implementation to a device.
   *
   * \param device Ptr to the device to which this object is being attached.
   * \return true if the operation was successful (always true actually)
   */
  virtual bool Attach (Ptr<DpskNetDevice> device);

  /**
   * Detach the implementation from the device.
   */
  virtual void Detach ();

protected:
  /**
   * Transmit process
   *
   * \return Ptr to the packet.
   */
  virtual Ptr<Packet> Tx ();

  /**
   * Receive process
   *
   * \param p Ptr to the received packet.
   */
  virtual void Rx (Ptr<Packet> p);

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

  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

  Ptr<DpskNetDevice> m_dev; //!< Attached DpskNetDevice
};

} // namespace ns3

#endif /* DPSK_NET_DEVICE_IMPL_H */
