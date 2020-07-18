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

#ifndef PFC_SWITCH_H
#define PFC_SWITCH_H

#include "ns3/dpsk-layer.h"
#include "ns3/object.h"
#include "ns3/dpsk-net-device.h"
#include "switch-mmu.h"
#include <unordered_map>

namespace ns3 {

/**
 * \ingroup pfc
 * \brief PFC switch implementation
 */
class PfcSwitch : public DpskLayer
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a PfcSwitch
   */
  PfcSwitch ();
  virtual ~PfcSwitch ();

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
   * \brief Install DPSK to handle packets directly.
   * \param dpsk the DPSK framework
   */
  virtual void InstallDpsk (Ptr<Dpsk> dpsk);

  /**
   * \brief Install MMU to the switch.
   * \param mmu MMU of the switch
   */
  void InstallMmu (Ptr<SwitchMmu> mmu);

  /**
   * Get MMU of this switch
   *
   * \return mmu of this switch
   */
  Ptr<SwitchMmu> GetMmu (void);

  /**
   * Set ECMP seed
   *
   * \param s seed
   */
  void SetEcmpSeed (uint32_t s);

  /**
   * Set Queue count.
   * It is not checked by this method that every net device have the same number of queues.
   * So do not use different net devices in one topology.
   *
   * \param n number of queues
   */
  void SetNQueues (uint32_t n);

  /**
   * Add a route rule to the route table
   *
   * \param dest the IPv4 address of the destination
   * \param dev output device
   */
  void AddRouteTableEntry (const Ipv4Address &dest, Ptr<DpskNetDevice> dev);

  /**
   * Get route table
   *
   * \return route table
   */
  std::unordered_map<uint32_t, std::vector<Ptr<DpskNetDevice>>> GetRouteTable ();

  /**
   * Clear route table
   */
  void ClearRouteTable ();

  /**
   * Level 2 flow control type
   */
  enum L2FlowControlType { UNKNOWN = -1, PFC, CBFC };

  static L2FlowControlType DeviceToL2Type (Ptr<DpskNetDevice> dev);

protected:
  /**
   * Perform any object release functionality required to break reference
   * cycles in reference counted objects held by the device.
   */
  virtual void DoDispose (void);

private:
  /**
   * Get ouput device by ECMP
   *
   * \param p received packet
   * \return target net device
   */
  Ptr<DpskNetDevice> GetOutDev (Ptr<const Packet> p);

  /**
   * Calculate ECMP Hash
   *
   * \param key array of bytes to calculate
   * \param len length of the array
   * \return hash code
   */
  uint32_t CalcEcmpHash (const uint8_t *key, size_t len);

  /**
   * Dequeue callback for each device in the switch.
   *
   * \param outDev output device
   * \param packet output packet
   * \param qIndex output queue index
   */
  void DeviceDequeueHandler (Ptr<NetDevice> outDev, Ptr<Packet> packet, uint32_t qIndex);

  uint32_t m_ecmpSeed; //!< ECMP seed

  uint32_t m_nDevices; //!< device number

  // devices managed by installed Dpsk with index
  std::unordered_map<uint32_t, Ptr<DpskNetDevice>> m_devices;

  uint32_t m_nQueues; //!< queues of every devices

  /**
   * Map from the value of destination IPv4 address to the vector of avaliable target
   * devices.
   */
  std::unordered_map<uint32_t, std::vector<Ptr<DpskNetDevice>>> m_routeTable;

  Ptr<SwitchMmu> m_mmu; //!< mmu of this switch

public:
  /// Statistics
  std::map<Ptr<DpskNetDevice>, uint32_t> m_nIngressDropPacket;

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  PfcSwitch (const PfcSwitch &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  PfcSwitch &operator= (const PfcSwitch &);
};

} // namespace ns3

#endif /* PFC_SWITCH_H */
