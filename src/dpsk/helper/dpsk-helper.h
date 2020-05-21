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
 * \brief Add capability to Dpsk device management
 */
class DpskHelper
{
public:
  /*
   * Construct a DpskHelper
   */
  DpskHelper ();

  /**
   * Set an attribute on each ns3::Dpsk created by
   * DpskHelper::Install
   *
   * \param n1 the name of the attribute to set
   * \param v1 the value of the attribute to set
   */
  void SetDeviceAttribute (std::string n1, const AttributeValue &v1);

  /**
   * Install DPSK on the node with all of its interfaces
   *
   * \param node The node to install the Dpsk in
   * \returns The Dpsk virtual device
   */
  Ptr<Dpsk> Install (Ptr<Node> node);

  /**
   * This method creates an ns3::Dpsk with the attributes
   * configured by DpskHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * Dpsk.
   *
   * \param node The node to install the Dpsk in
   * \param c Container of NetDevices to add as Dpsk managed ports
   * \returns The Dpsk virtual device
   */
  Ptr<Dpsk> Install (Ptr<Node> node, NetDeviceContainer c);

  /**
   * This method creates an ns3::Dpsk with the attributes
   * configured by DpskHelper::SetDeviceAttribute, adds the device
   * to the node, and attaches the given NetDevices as ports of the
   * Dpsk.
   *
   * \param nodeName The name of the node to install the device in
   * \param c Container of NetDevices to add as Dpsk managed ports
   * \returns The Dpsk virtual device
   */
  Ptr<Dpsk> Install (std::string nodeName, NetDeviceContainer c);

private:
  ObjectFactory m_deviceFactory; //!< Object factory
};

} // namespace ns3

#endif /* DPSK_HELPER_H */
