/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
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

#ifndef DPSK_APPLICATION_H
#define DPSK_APPLICATION_H

#include "ns3/application.h"
#include "ns3/net-device.h"
#include "ns3/ptr.h"
#include "ns3/node.h"

namespace ns3 {

class DPSK;

/**
 * \ingroup applications
 * \defgroup dpskApplication DPSKApplication
 *
 * This application can be used as a base class for ns3 DPSK applications.
 * DPSK applications can get packets directly from devices.
 */

/**
 * \ingroup dpsk
 *
 * \brief The base class for all DPSK applications
 */
class DPSKApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  DPSKApplication ();
  virtual ~DPSKApplication ();

  /**
   * \brief Application specific seting up logic
   *
   * This method is called to initialize the DPSK application.
   * This method should be overridden by all or most DPSK application
   * subclasses.
   */
  virtual void SetupDPSK (Ptr<DPSK> dpsk);

private:
  /**
   * \brief Application specific scheduling transmitting
   *
   * This method is called accroding to the scheduling algorithm.
   * This method should be overridden by all or most DPSK application
   * subclasses.
   */
  void HandleTx (void);

  /**
   * \brief Application specific receiving logic
   *
   * This method is called when one device received one packet.
   * This method should be overridden by all or most application
   * subclasses.
   */
  void HandleRx (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
                 Address const &src, Address const &dst, NetDevice::PacketType packetType);

  /**
   * \brief Application specific startup code
   *
   * The StartApplication method is called at the start time specified by Start
   * This method should be overridden by all or most application
   * subclasses.
   */
  virtual void StartApplication (void);

  /**
   * \brief Application specific shutdown code
   *
   * The StopApplication method is called at the stop time specified by Stop
   * This method should be overridden by all or most application
   * subclasses.
   */
  virtual void StopApplication (void);

  Ptr<DPSK> m_dpsk; //!< DPSK framework
};

} // namespace ns3

#endif /* DPSK_APPLICATION_H */
