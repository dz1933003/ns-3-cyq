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

#ifndef DPSK_LAYER_H
#define DPSK_LAYER_H

#include "ns3/dpsk.h"
#include "ns3/object.h"
#include "ns3/net-device.h"

namespace ns3 {

/**
 * \ingroup dpsk
 * \brief Add capability to Dpsk device management
 */
class DpskLayer : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a DpskLayer
   */
  DpskLayer ();
  virtual ~DpskLayer ();

  /**
   * \brief Sends a packet from one device.
   * \param device the originating port (if NULL then broadcasts to all ports)
   * \param packet the sended packet
   * \param protocol the packet protocol (e.g., Ethertype)
   * \param source the packet source
   * \param destination the packet destination
   */
  virtual bool SendFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                               const Address &source, const Address &destination);

  /**
   * \brief Receives a packet from one device.
   * \param device the originating port
   * \param packet the received packet
   * \param protocol the packet protocol (e.g., Ethertype)
   * \param source the packet source
   * \param destination the packet destination
   * \param packetType the packet type (e.g., host, broadcast, etc.)
   */
  virtual void ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet,
                                  uint16_t protocol, const Address &source,
                                  const Address &destination, NetDevice::PacketType packetType);

protected:
  /**
   * Perform any object release functionality required to break reference
   * cycles in reference counted objects held by the device.
   */
  virtual void DoDispose (void);

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  DpskLayer (const DpskLayer &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  DpskLayer &operator= (const DpskLayer &);

  /// Typedef for Dpsk layer container
  typedef std::vector<Ptr<DpskLayer>> DpskLayerList;
  DpskLayerList m_layers; //!< Dpsk layers in the layer

  Ptr<Dpsk> m_dpsk; //!< Dpsk framework

  /**
   * A receive handler
   *
   * \param device a pointer to the net device which received the packet
   * \param packet the packet received
   * \param protocol the 16 bit protocol number associated with this packet.
   *        This protocol number is expected to be the same protocol number
   *        given to the Send method by the user on the sender side.
   * \param sender the address of the sender
   * \param receiver the address of the receiver
   * \param packetType type of packet received
   *                   (broadcast/multicast/unicast/otherhost); Note:
   *                   this value is only valid for promiscuous mode
   *                   protocol handlers.
   */
  typedef Callback<void, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address &,
                   const Address &, NetDevice::PacketType>
      ReceiveFromDeviceHandler;

  /// Typedef for receive-from-device handlers container
  typedef std::vector<ReceiveFromDeviceHandler> ReceiveFromDeviceHandlerList;
  ReceiveFromDeviceHandlerList m_handlers; //!< Protocol handlers in the node
};

} // namespace ns3

#endif /* DPSK_LAYER_H */
