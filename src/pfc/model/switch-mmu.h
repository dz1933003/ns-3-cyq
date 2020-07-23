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

#ifndef SWITCH_MMU_H
#define SWITCH_MMU_H

#include <ns3/node.h>
#include <vector>
#include <map>
#include <sstream>

#include "ns3/dpsk-net-device.h"
#include "pfc-switch-mmu-queue.h"
#include "cbfc-switch-mmu-queue.h"

namespace ns3 {

class Packet;
class UniformRandomVariable;

/**
 * \ingroup pfc
 * \brief Memory management unit of a switch
 */
class SwitchMmu : public Object
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Construct a SwitchMmu
   */
  SwitchMmu ();
  ~SwitchMmu ();

  std::string Dump ();

public:
  /**
   * Add devices need to be managed to the mmu
   *
   * \param dev device.
   */
  void AggregateDevice (Ptr<DpskNetDevice> dev);

  /**
   * Configurate queue number of the switch
   *
   * \param n queue number
   */
  void ConfigNQueue (uint32_t n);

public:
  /**
   * Configurate buffer size
   *
   * \param size buffer size by byte
   */
  void ConfigBufferSize (uint64_t size);

  // ECN Functions

  /**
   * Configurate ECN parameters on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \param kMin kMin
   * \param kMax kMax
   * \param pMax pMax
   */
  void ConfigEcn (Ptr<NetDevice> port, uint32_t qIndex, uint64_t kMin, uint64_t kMax, double pMax);

  /**
   * Configurate ECN parameters on all queues of the port
   *
   * \param port target port
   * \param kMin kMin
   * \param kMax kMax
   * \param pMax pMax
   */
  void ConfigEcn (Ptr<NetDevice> port, uint64_t kMin, uint64_t kMax, double pMax);

  /**
   * Configurate ECN parameters on one queue of all the ports
   *
   * \param qIndex target queue index
   * \param kMin kMin
   * \param kMax kMax
   * \param pMax pMax
   */
  void ConfigEcn (uint32_t qIndex, uint64_t kMin, uint64_t kMax, double pMax);

  /**
   * Configurate ECN parameters on all ports in the switch
   *
   * \param kMin kMin
   * \param kMax kMax
   * \param pMax pMax
   */
  void ConfigEcn (uint64_t kMin, uint64_t kMax, double pMax);

  // PFC Functions

  /**
   * Configurate headroom on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \param size headroom size by byte
   */
  void ConfigHeadroom (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size);

  /**
   * Configurate headroom on all queues of the port
   *
   * \param port target port
   * \param size headroom size by byte
   */
  void ConfigHeadroom (Ptr<NetDevice> port, uint64_t size);

  /**
   * Configurate headroom on one queue of all the ports
   *
   * \param qIndex target queue index
   * \param size headroom size by byte
   */
  void ConfigHeadroom (uint32_t qIndex, uint64_t size);

  /**
   * Configurate headroom on all ports in the switch
   *
   * \param size headroom size by byte
   */
  void ConfigHeadroom (uint64_t size);

  /**
   * Configurate reserved buffer on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \param size reserved size by byte
   */
  void ConfigReserve (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size);

  /**
   * Configurate reserve on all queues of the port
   *
   * \param port target port
   * \param size reserve size by byte
   */
  void ConfigReserve (Ptr<NetDevice> port, uint64_t size);

  /**
   * Configurate reserved buffer on one queue of all the ports
   *
   * \param qIndex target queue index
   * \param size reserved size by byte
   */
  void ConfigReserve (uint32_t qIndex, uint64_t size);

  /**
   * Configurate reserve on all ports in the switch
   *
   * \param size reserve size by byte
   */
  void ConfigReserve (uint64_t size);

  /**
   * Configurate resume offset on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \param size resume offset size by byte
   */
  void ConfigResumeOffset (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size);

  /**
   * Configurate resume offset on all queues of the port
   *
   * \param port target port
   * \param size resume offset size by byte
   */
  void ConfigResumeOffset (Ptr<NetDevice> port, uint64_t size);

  /**
   * Configurate resume offset on one queue of all the ports
   *
   * \param qIndex target queue index
   * \param size resume offset size by byte
   */
  void ConfigResumeOffset (uint32_t qIndex, uint64_t size);

  /**
   * Configurate resume offset on all ports in the switch
   *
   * \param size resume offset size by byte
   */
  void ConfigResumeOffset (uint64_t size);

  /**
   * Get headroom size
   *
   * \param port target port
   * \param qIndex target queue index
   * \return headroom by byte
   */
  uint64_t GetHeadroomSize (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Get headroom size
   *
   * \param port target port
   * \return headroom by byte
   */
  uint64_t GetHeadroomSize (Ptr<NetDevice> port);

  /**
   * Get headroom size
   *
   * \return headroom by byte
   */
  uint64_t GetHeadroomSize ();

  /**
   * Set pause to a port and the target queue
   *
   * \param port target port
   * \param qIndex target queue index
   */
  void SetPause (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Set resume to a port and the target queue
   *
   * \param port target port
   * \param qIndex target queue index
   */
  void SetResume (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Get PFC threshold
   *
   * \param port target port
   * \param qIndex target queue index
   * \return PFC threshold by bytes
   */
  uint64_t GetPfcThreshold (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Check whether send pause to the peer of this port
   *
   * \param port target port
   * \param qIndex target queue index
   */
  bool CheckShouldSendPfcPause (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Check whether send resume to the peer of this port
   *
   * \param port target port
   * \param qIndex target queue index
   */
  bool CheckShouldSendPfcResume (Ptr<NetDevice> port, uint32_t qIndex);

  // CBFC Functions

  /**
   * Configurate CBFC buffer on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \param size CBFC buffer size by byte
   */
  void ConfigCbfcBufferSize (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size);

  /**
   * Configurate CBFC buffer on all queues of the port
   *
   * \param port target port
   * \param size CBFC buffer size by byte
   */
  void ConfigCbfcBufferSize (Ptr<NetDevice> port, uint64_t size);

  /**
   * Configurate CBFC buffer on one queue of all the ports
   *
   * \param qIndex target queue index
   * \param size CBFC buffer size by byte
   */
  void ConfigCbfcBufferSize (uint32_t qIndex, uint64_t size);

  /**
   * Configurate CBFC buffer on all ports in the switch
   *
   * \param size CBFC buffer size by byte
   */
  void ConfigCbfcBufferSize (uint64_t size);

  /**
   * Configurate CBFC feedback peroid on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \param peroid CBFC feedback peroid
   */
  void ConfigCbfcFeedbackPeroid (Ptr<NetDevice> port, uint32_t qIndex, Time peroid);

  /**
   * Configurate CBFC feedback peroid on all queues of the port
   *
   * \param port target port
   * \param peroid CBFC feedback peroid
   */
  void ConfigCbfcFeedbackPeroid (Ptr<NetDevice> port, Time peroid);

  /**
   * Configurate CBFC feedback peroid on one queue of all the ports
   *
   * \param qIndex target queue index
   * \param peroid CBFC feedback peroid
   */
  void ConfigCbfcFeedbackPeroid (uint32_t qIndex, Time peroid);

  /**
   * Configurate CBFC feedback peroid on all ports in the switch
   *
   * \param peroid CBFC feedback peroid
   */
  void ConfigCbfcFeedbackPeroid (Time peroid);

  /**
   * Get CBFC feedback peroid on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \return CBFC feedback peroid
   */
  Time GetCbfcFeedbackPeroid (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Get FCCL on one queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \return FCCL in bytes
   */
  uint64_t GetCbfcFccl (Ptr<NetDevice> port, uint32_t qIndex);

  // Common Functions for all L2 flow control algorithms

  /**
   * Check the admission of target ingress
   *
   * \param port target port
   * \param qIndex target queue index
   * \param pSize packet size in bytes
   * \return admission
   */
  bool CheckIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize);

  /**
   * Check the admission of target egress
   *
   * \param port target port
   * \param qIndex target queue index
   * \param pSize packet size in bytes
   * \return admission
   */
  bool CheckEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize);

  /**
   * Update the target ingress
   *
   * \param port target port
   * \param qIndex target queue index
   * \param pSize packet size in bytes
   */
  void UpdateIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize);

  /**
   * Update the target egress
   *
   * \param port target port
   * \param qIndex target queue index
   * \param pSize packet size in bytes
   */
  void UpdateEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize);

  /**
   * Remove from ingress
   *
   * \param port target port
   * \param qIndex target queue index
   * \param pSize packet size in bytes
   */
  void RemoveFromIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize);

  /**
   * Remove from egress
   *
   * \param port target port
   * \param qIndex target queue index
   * \param pSize packet size in bytes
   */
  void RemoveFromEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize);

  /**
   * Check whether need to set ECN bit
   *
   * \param port target port
   * \param qIndex target queue index
   */
  bool CheckShouldSetEcn (Ptr<NetDevice> port, uint32_t qIndex);

  // Statistical functions

  /**
   * Get buffer size
   *
   * \return buffer size by byte
   */
  uint64_t GetBufferSize ();

  /**
   * Get shared buffer size
   *
   * \return shared buffer size by byte
   */
  uint64_t GetSharedBufferSize ();

  /**
   * Get shared buffer remains
   *
   * \return shared buffer remains by byte
   */
  uint64_t GetSharedBufferRemain ();

  /**
   * Get shared buffer used bytes of a queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \return used bytes
   */
  uint64_t GetSharedBufferUsed (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Get shared buffer used bytes of a port
   *
   * \param port target port
   * \return used bytes
   */
  uint64_t GetSharedBufferUsed (Ptr<NetDevice> port);

  /**
   * Get total shared buffer used bytes
   *
   * \return used bytes
   */
  uint64_t GetSharedBufferUsed ();

  /**
   * Get buffer used bytes of a queue
   *
   * \param port target port
   * \param qIndex target queue index
   * \return used bytes
   */
  uint64_t GetBufferUsed (Ptr<NetDevice> port, uint32_t qIndex);

  /**
   * Get buffer used bytes of a port
   *
   * \param port target port
   * \return used bytes
   */
  uint64_t GetBufferUsed (Ptr<NetDevice> port);

  /**
   * Get total buffer used bytes
   *
   * \return used bytes
   */
  uint64_t GetBufferUsed ();

protected:
  /**
   * Perform any object release functionality required to break reference
   * cycles in reference counted objects held by the device.
   */
  virtual void DoDispose (void);

private:
  uint64_t m_bufferConfig; //!< configuration of buffer size

  // Queue configurations
  std::map<Ptr<NetDevice>, std::vector<Ptr<SwitchMmuQueue>>> m_switchMmuQueueConfig;

  struct EcnConfig // ECN configuration
  {
    uint64_t kMin;
    uint64_t kMax;
    double pMax;
    bool enable;
  };

  // Map from Ptr to net device to the queue configuration of ECN
  std::map<Ptr<NetDevice>, std::vector<EcnConfig>> m_ecnConfig;

  uint32_t m_nQueues; //!< queue number of each device
  std::vector<Ptr<NetDevice>> m_devices; //!< devices managed by this mmu

  bool m_dynamicThreshold; //!< if enabled dynamic PFC threshold

  Ptr<UniformRandomVariable> uniRand; //!< random var stream

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  SwitchMmu (const SwitchMmu &);

  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  SwitchMmu &operator= (const SwitchMmu &);
};

} /* namespace ns3 */

#endif /* SWITCH_MMU_H */
