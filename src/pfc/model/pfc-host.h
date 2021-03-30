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

#ifndef PFC_HOST_H
#define PFC_HOST_H

#include "pfc-host-port.h"

#include "ns3/dpsk-layer.h"
#include "ns3/object.h"
#include "ns3/net-device.h"
#include "ns3/rdma-tx-queue-pair.h"

#include <unordered_map>
#include <map>
#include <set>

namespace ns3 {

class PfcHostPort;

/**
 * \ingroup pfc
 * \brief PFC host implementation
 */
class PfcHost : public DpskLayer
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a PfcHost
   */
  PfcHost ();
  virtual ~PfcHost ();

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
   * Add RDMA queue pair for transmitting
   *
   * \param qp queue pair to send
   */
  void AddRdmaTxQueuePair (Ptr<RdmaTxQueuePair> qp);

  /**
   * Get RDMA queue pair for transmitting
   *
   * \return queue pairs to send
   */
  std::vector<Ptr<RdmaTxQueuePair>> GetRdmaTxQueuePairs ();

  /**
   * Get RDMA queue pair for receiving
   *
   * \return queue pairs to receive
   */
  std::map<uint32_t, Ptr<RdmaRxQueuePair>> GetRdmaRxQueuePairs ();

  /**
   * Get RDMA queue pair for receiving
   *
   * \param hash hash code of qp
   * \return queue pair to receive
   */
  Ptr<RdmaRxQueuePair> GetRdmaRxQueuePair (uint32_t hash);

  /**
   * Add RDMA queue pair size for receiving port
   *
   * \param key hash key of the queue pair
   * \param size size of the queue pair
   */
  void AddRdmaRxQueuePairSize (uint32_t key, uint64_t size);

  /**
   * Get RDMA queue pair size for receiving port
   *
   * \param key hash key of the queue pair
   * \return size of the queue pair
   */
  uint64_t GetRdmaRxQueuePairSize (uint32_t key);

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
   * \param qp transimitted queue pair
   * \return target net device
   */
  Ptr<DpskNetDevice> GetOutDev (Ptr<RdmaTxQueuePair> qp);

  uint32_t m_nDevices; //!< device number
  std::set<Ptr<DpskNetDevice>> m_devices; //!< devices managed by installed Dpsk

  /**
   * Map from the value of destination IPv4 address to the vector of avaliable target
   * devices.
   */
  std::unordered_map<uint32_t, std::vector<Ptr<DpskNetDevice>>> m_routeTable;

  std::map<uint32_t, uint64_t> m_rxQueuePairSize; //!< hash and received queue pair size

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  PfcHost (const PfcHost &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  PfcHost &operator= (const PfcHost &);
};

} // namespace ns3

#endif /* PFC_HOST_H */
