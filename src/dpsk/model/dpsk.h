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
#include "ns3/dpsk-channel.h"

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
 * The DPSK object is a "virtual" netdevice that aggregates
 * multiple "real" netdevices and provides the data plane packet
 * operation APIs.  By adding a DPSK to a Node, it will act as a DPDK
 * program, or a smart switch.
 *
 * \attention dpsk is designed to work only with NetDevices
 * modelling IEEE 802-style technologies, such as CsmaNetDevice.
 */
class DPSK : public NetDevice
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  DPSK ();
  virtual ~DPSK ();

  // Added public functions on DPSK

  /**
   * \brief Add a device to DPSK for managing
   * \param device the NetDevice to add
   *
   * This method adds an existed device to a DPSK, so that
   * the NetDevice is managed by DPSK and L2 frames start
   * being forwarded to/from this NetDevice by our programming.
   *
   * \param bridgePort NetDevice
   * \attention The netdevice that is being added must _not_ have an
   * IP address.  In order to add IP connectivity to a bridging node
   * you must enable IP on the DPSK itself, by programing L3 logic etc,
   * never on the netdevices DPSK managing.
   */
  void RegisterDevice (Ptr<NetDevice> device);

  /**
   * \brief Gets the number of devices, i.e., the NetDevices
   * managed by DPSK.
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
   * \brief Receives a packet from one bridged port.
   * \param device the originating port
   * \param packet the received packet
   * \param protocol the packet protocol (e.g., Ethertype)
   * \param source the packet source
   * \param destination the packet destination
   * \param packetType the packet type (e.g., host, broadcast, etc.)
   */
  void ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                          Address const &source, Address const &destination, PacketType packetType);

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  DPSK (const DPSK &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  DPSK &operator= (const DPSK &);

  NetDevice::ReceiveCallback m_rxCallback; //!< receive callback
  NetDevice::PromiscReceiveCallback m_promiscRxCallback; //!< promiscuous receive callback

  Mac48Address m_address; //!< MAC address of the DPSK device
  Ptr<Node> m_node; //!< node DPSK installs on
  Ptr<DPSKChannel> m_channel; //!< DPSK channel manager
  std::vector<Ptr<NetDevice>> m_ports; //!< devices managed by DPSK
  uint32_t m_ifIndex; //!< Interface index of DPSK
  uint16_t m_mtu; //!< MTU of the DPSK (Not used)
};

} // namespace ns3

#endif /* DPSK_H */
