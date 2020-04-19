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

#ifndef DPSK_HELPER_H
#define DPSK_HELPER_H

#include "ns3/dpsk.h"
#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"

namespace ns3 {

class Node;
class AttributeValue;

/**
 * \ingroup dpsk
 * \brief Add capability to DPSK device management
 */
class DPSKHelper
{
public:
  /*
   * Construct a DPSKHelper
   */
  DPSKHelper ();

  /**
   * Set an attribute on each ns3::DPSK created by
   * DPSKHelper::Install
   *
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   */
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * This method creates an ns3::DPSK with the attributes
   * configured by DPSKHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * DPSK.
   *
   * \param node The node to install the DPSK in
   * \param c Container of NetDevices to add as DPSK managed ports
   * \returns The DPSK virtual device
   */
  Ptr<DPSK> Install (Ptr<Node> node, NetDeviceContainer c);

  /**
   * This method creates an ns3::DPSK with the attributes
   * configured by DPSKHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * DPSK.
   *
   * \param nodeName The name of the node to install the device in
   * \param c Container of NetDevices to add as DPSK managed ports
   * \returns The DPSK virtual device
   */
  Ptr<DPSK> Install (std::string nodeName, NetDeviceContainer c);

private:
  ObjectFactory m_deviceFactory; //!< Object factory
};

} // namespace ns3

#endif /* DPSK_HELPER_H */
