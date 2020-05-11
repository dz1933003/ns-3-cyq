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

#ifndef DPSK_H
#define DPSK_H

#include "ns3/net-device.h"

namespace ns3 {

class Node;

/**
 * \defgroup dpsk Dataplane Simulation Kit
 */

/**
 * \ingroup dpsk
 *
 * \brief Dataplane Simulation Kit
 *
 * The Dpsk object is a "virtual" netdevice that aggregates
 * multiple "real" netdevices and provides the data plane packet
 * operation APIs.  By adding a Dpsk to a Node, it will act as a DPDK
 * program, or a smart switch.
 *
 * \attention dpsk is designed to work only with NetDevices
 * modelling IEEE 802-style technologies, such as CsmaNetDevice.
 */
class Dpsk : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  Dpsk ();
  virtual ~Dpsk ();

  // Added public functions on Dpsk

  /**
   * \brief Add a device to Dpsk for managing
   * \param device the NetDevice to add
   *
   * This method adds an existed device to a Dpsk, so that
   * the NetDevice is managed by Dpsk and L2 frames start
   * being forwarded to/from this NetDevice by our programming.
   *
   * \param device NetDevice
   * \attention The netdevice that is being added must _not_ have an
   * IP address.  In order to add IP connectivity to a bridging node
   * you must enable IP on the Dpsk itself, by programing L3 logic etc,
   * never on the netdevices Dpsk managing.
   */
  void AddDevice (Ptr<NetDevice> device);

  /**
   * \brief Gets the number of devices, i.e., the NetDevices
   * managed by Dpsk.
   *
   * \return the number of devices.
   */
  uint32_t GetNDevices (void) const;

  /**
   * \brief Gets the n-th device.
   * \param n the device index
   * \return the n-th NetDevice
   */
  Ptr<NetDevice> GetDevice (uint32_t n) const;

  /**
   * \brief Gets all devices.
   * \return All NetDevices Dpsk is managing
   */
  std::vector<Ptr<NetDevice>> GetDevices (void) const;

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

  /**
   * \param handler the handler to register
   */
  void RegisterReceiveFromDeviceHandler (ReceiveFromDeviceHandler handler);

  /**
   * \param handler the handler to unregister
   *
   * After this call returns, the input handler will never
   * be invoked anymore.
   */
  void UnregisterReceiveFromDeviceHandler (ReceiveFromDeviceHandler handler);

  // Inherited from NetDevice base class

  virtual void SetIfIndex (const uint32_t index);
  virtual uint32_t GetIfIndex (void) const;

  virtual Ptr<Channel> GetChannel (void) const;

  virtual void SetAddress (Address address);
  virtual Address GetAddress (void) const;

  virtual bool SetMtu (const uint16_t mtu);
  virtual uint16_t GetMtu (void) const;

  virtual bool IsLinkUp (void) const;
  virtual void AddLinkChangeCallback (Callback<void> callback);

  virtual bool IsBroadcast (void) const;
  virtual Address GetBroadcast (void) const;

  virtual bool IsMulticast (void) const;
  virtual Address GetMulticast (Ipv4Address multicastGroup) const;
  virtual Address GetMulticast (Ipv6Address addr) const;

  virtual bool IsBridge (void) const;

  virtual bool IsPointToPoint (void) const;

  virtual bool Send (Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  virtual bool SendFrom (Ptr<Packet> packet, const Address &source, const Address &dest,
                         uint16_t protocolNumber);

  virtual Ptr<Node> GetNode (void) const;
  virtual void SetNode (Ptr<Node> node);

  virtual bool NeedsArp (void) const;

  virtual void SetReceiveCallback (ReceiveCallback cb);
  virtual void SetPromiscReceiveCallback (PromiscReceiveCallback cb);

  virtual bool SupportsSendFrom (void) const;

protected:
  /**
   * Perform any object release functionality required to break reference
   * cycles in reference counted objects held by the device.
   */
  virtual void DoDispose (void);

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
                                  const Address &destination, PacketType packetType);

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  Dpsk (const Dpsk &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  Dpsk &operator= (const Dpsk &);

  /// receive callback (Not used)
  NetDevice::ReceiveCallback m_rxCallback;
  /// promiscuous receive callback (Not used)
  NetDevice::PromiscReceiveCallback m_promiscRxCallback;

  Mac48Address m_address; //!< MAC address of the Dpsk device (Not used)
  Ptr<Node> m_node; //!< node Dpsk installs on
  std::vector<Ptr<NetDevice>> m_ports; //!< devices managed by Dpsk
  uint32_t m_ifIndex; //!< Interface index of Dpsk (Not used)
  uint16_t m_mtu; //!< MTU of the Dpsk (Not used)

  /// Typedef for receive-from-device handlers container
  typedef std::vector<ReceiveFromDeviceHandler> ReceiveFromDeviceHandlerList;
  ReceiveFromDeviceHandlerList m_handlers; //!< Protocol handlers in the node
};

} // namespace ns3

#endif /* DPSK_H */
