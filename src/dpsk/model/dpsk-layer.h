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
#include <map>

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
   * \brief Get the name of the Dpsk layer.
   * \return name
   */
  std::string GetName () const;

  /**
   * \brief Set the name of the Dpsk layer.
   * \param name the name of the Dpsk layer
   *
   * In fact, every instance of a DPSK layer should have a unique name.
   */
  void SetName (std::string name);

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

  /**
   * \brief Register an associated layer to send to.
   * \param layer the layer to register
   */
  void RegisterLayerSendDownward (Ptr<DpskLayer> layer);

  /**
   * \brief Register an associated layer to call receive callback after received.
   * \param layer the layer to register
   */
  void RegisterLayerReceiveUpward (Ptr<DpskLayer> layer);

  /**
   * \brief Install the upper layer.
   * \param layer the layer to install
   */
  virtual void InstallUpperLayer (Ptr<DpskLayer> layer);

  /**
   * \brief Install the lower layer.
   * \param layer the layer to install
   */
  virtual void InstallLowerLayer (Ptr<DpskLayer> layer);

  /**
   * \brief Install DPSK to handle packets directly.
   * \param dpsk the DPSK framework
   */
  virtual void InstallDpsk (Ptr<Dpsk> dpsk);

protected:
  /**
   * Perform any object release functionality required to break reference
   * cycles in reference counted objects held by the device.
   */
  virtual void DoDispose (void);

  std::string m_name; //!< Dpsk layer name

  /// Typedef for Dpsk layer container
  typedef std::map<const std::string, Ptr<DpskLayer>> DpskLayerMap;
  DpskLayerMap m_dpskLayers; //!< Dpsk layers in the layer

  /// Typedef for ReceiveFromDevice handlers container
  typedef std::map<const std::string, ReceiveFromDeviceHandler> LayerReceiveHandlerMap;
  LayerReceiveHandlerMap m_layerHandlers; //!< Dpsk layer receive handlers in the layer

  Ptr<Dpsk> m_dpsk; //!< Dpsk framework

  /// Typedef for ReceiveFromDevice handlers container
  typedef std::vector<ReceiveFromDeviceHandler> AppReceiveHandlerList;
  AppReceiveHandlerList m_appHandlers; //!< Application receive handlers in the layer
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
};

} // namespace ns3

#endif /* DPSK_LAYER_H */
