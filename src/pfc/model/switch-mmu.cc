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

#include "switch-mmu.h"

#include "ns3/log.h"
#include "ns3/pfc-switch.h"

#include "ns3/random-variable-stream.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SwitchMmu");

NS_OBJECT_ENSURE_REGISTERED (SwitchMmu);

TypeId
SwitchMmu::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SwitchMmu")
                          .SetParent<Object> ()
                          .SetGroupName ("Pfc")
                          .AddConstructor<SwitchMmu> ();
  return tid;
}

SwitchMmu::SwitchMmu (void)
    : m_bufferConfig (12 * 1024 * 1024), m_nQueues (0), m_dynamicThreshold (false)
{
  NS_LOG_FUNCTION_NOARGS ();
  uniRand = CreateObject<UniformRandomVariable> ();
}

SwitchMmu::~SwitchMmu (void)
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
SwitchMmu::AggregateDevice (Ptr<DpskNetDevice> dev)
{
  NS_LOG_FUNCTION (dev);
  m_devices.push_back (dev);
  for (uint32_t i = 0; i <= m_nQueues; i++) // with one control queue
    {
      const auto devType = PfcSwitch::DeviceToL2Type (dev);
      if (devType == PfcSwitch::PFC)
        {
          m_switchMmuQueueConfig[dev].push_back (CreateObject<PfcSwitchMmuQueue> ());
        }
      else if (devType == PfcSwitch::CBFC)
        {
          m_switchMmuQueueConfig[dev].push_back (CreateObject<CbfcSwitchMmuQueue> ());
        }
      else if (devType == PfcSwitch::CBPFC)
        {
          m_switchMmuQueueConfig[dev].push_back (CreateObject<CbpfcSwitchMmuQueue> ());
        }
      else if (devType == PfcSwitch::PTPFC)
        {
          m_switchMmuQueueConfig[dev].push_back (CreateObject<PtpfcSwitchMmuQueue> ());
        }
      m_ecnConfig[dev].push_back ({0, 0, 0., false});
    }
}

void
SwitchMmu::ConfigNQueue (uint32_t n)
{
  NS_LOG_FUNCTION (n);
  m_nQueues = n;
}

void
SwitchMmu::ConfigBufferSize (uint64_t size)
{
  m_bufferConfig = size;
}

/*****************
 * ECN Functions *
 *****************/

void
SwitchMmu::ConfigEcn (Ptr<NetDevice> port, uint32_t qIndex, uint64_t kMin, uint64_t kMax,
                      double pMax)
{
  m_ecnConfig[port][qIndex] = {kMin, kMax, pMax, true};
}

void
SwitchMmu::ConfigEcn (Ptr<NetDevice> port, uint64_t kMin, uint64_t kMax, double pMax)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigEcn (port, i, kMin, kMax, pMax);
    }
}

void
SwitchMmu::ConfigEcn (uint32_t qIndex, uint64_t kMin, uint64_t kMax, double pMax)
{
  for (const auto &dev : m_devices)
    {
      ConfigEcn (dev, qIndex, kMin, kMax, pMax);
    }
}

void
SwitchMmu::ConfigEcn (uint64_t kMin, uint64_t kMax, double pMax)
{
  for (const auto &dev : m_devices)
    {
      ConfigEcn (dev, kMin, kMax, pMax);
    }
}

/*****************
 * PFC Functions *
 *****************/

void
SwitchMmu::ConfigHeadroom (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::PFC)
    {
      DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->headroom = size;
    }
}

void
SwitchMmu::ConfigHeadroom (Ptr<NetDevice> port, uint64_t size)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigHeadroom (port, i, size);
    }
}

void
SwitchMmu::ConfigHeadroom (uint32_t qIndex, uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigHeadroom (dev, qIndex, size);
    }
}

void
SwitchMmu::ConfigHeadroom (uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigHeadroom (dev, size);
    }
}

void
SwitchMmu::ConfigReserve (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::PFC)
    {
      DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->reserve = size;
    }
}

void
SwitchMmu::ConfigReserve (Ptr<NetDevice> port, uint64_t size)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigReserve (port, i, size);
    }
}

void
SwitchMmu::ConfigReserve (uint32_t qIndex, uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigReserve (dev, qIndex, size);
    }
}

void
SwitchMmu::ConfigReserve (uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigReserve (dev, size);
    }
}

void
SwitchMmu::ConfigResumeOffset (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::PFC)
    {
      DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->resumeOffset = size;
    }
}

void
SwitchMmu::ConfigResumeOffset (Ptr<NetDevice> port, uint64_t size)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigResumeOffset (port, i, size);
    }
}

void
SwitchMmu::ConfigResumeOffset (uint32_t qIndex, uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigResumeOffset (dev, qIndex, size);
    }
}

void
SwitchMmu::ConfigResumeOffset (uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigResumeOffset (dev, size);
    }
}

uint64_t
SwitchMmu::GetHeadroomSize (Ptr<NetDevice> port, uint32_t qIndex)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::PFC)
    {
      return DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->headroom;
    }
  else
    {
      return 0;
    }
  return 0;
}

uint64_t
SwitchMmu::GetHeadroomSize (Ptr<NetDevice> port)
{
  uint64_t size = 0;
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      size += GetHeadroomSize (port, i);
    }
  return size;
}

uint64_t
SwitchMmu::GetHeadroomSize ()
{
  uint64_t size = 0;
  for (const auto &dev : m_devices)
    {
      size += GetHeadroomSize (dev);
    }
  return size;
}

void
SwitchMmu::SetPause (Ptr<NetDevice> port, uint32_t qIndex)
{
  DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->isPaused = true;
}

void
SwitchMmu::SetResume (Ptr<NetDevice> port, uint32_t qIndex)
{
  DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->isPaused = false;
}

uint64_t
SwitchMmu::GetPfcThreshold (Ptr<NetDevice> port, uint32_t qIndex)
{
  // cyq: add dynamic PFC threshold choice if needed
  return 0;
}

bool
SwitchMmu::CheckShouldSendPfcPause (Ptr<NetDevice> port, uint32_t qIndex)
{
  NS_LOG_FUNCTION (port << qIndex);

  auto queueConfig = DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);

  if (m_dynamicThreshold)
    return (queueConfig->isPaused == false) &&
           (queueConfig->headroomUsed > 0 ||
            GetSharedBufferUsed (port, qIndex) >= GetPfcThreshold (port, qIndex));
  else
    return queueConfig->isPaused == false && queueConfig->headroomUsed > 0;
}

bool
SwitchMmu::CheckShouldSendPfcResume (Ptr<NetDevice> port, uint32_t qIndex)
{
  NS_LOG_FUNCTION (port << qIndex);

  auto queueConfig = DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);

  if (queueConfig->isPaused == false)
    return false;

  uint64_t sharedBufferUsed = GetSharedBufferUsed (port, qIndex);
  if (m_dynamicThreshold)
    return (queueConfig->headroomUsed == 0) &&
           (sharedBufferUsed == 0 ||
            sharedBufferUsed + queueConfig->resumeOffset <= GetPfcThreshold (port, qIndex));
  else
    return (queueConfig->headroomUsed == 0) &&
           (sharedBufferUsed == 0 ||
            GetSharedBufferUsed () + queueConfig->resumeOffset <= GetSharedBufferSize ());
}

/******************
 * CBFC Functions *
 ******************/

void
SwitchMmu::ConfigCbfcBufferSize (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::CBFC)
    {
      DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->ingressSize = size;
    }
}

void
SwitchMmu::ConfigCbfcBufferSize (Ptr<NetDevice> port, uint64_t size)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigCbfcBufferSize (port, i, size);
    }
}

void
SwitchMmu::ConfigCbfcBufferSize (uint32_t qIndex, uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbfcBufferSize (dev, qIndex, size);
    }
}

void
SwitchMmu::ConfigCbfcBufferSize (uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbfcBufferSize (dev, size);
    }
}

void
SwitchMmu::ConfigCbfcFeedbackPeroid (Ptr<NetDevice> port, uint32_t qIndex, Time peroid)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::CBFC)
    {
      DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->peroid = peroid;
    }
}

void
SwitchMmu::ConfigCbfcFeedbackPeroid (Ptr<NetDevice> port, Time peroid)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigCbfcFeedbackPeroid (port, i, peroid);
    }
}

void
SwitchMmu::ConfigCbfcFeedbackPeroid (uint32_t qIndex, Time peroid)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbfcFeedbackPeroid (dev, qIndex, peroid);
    }
}

void
SwitchMmu::ConfigCbfcFeedbackPeroid (Time peroid)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbfcFeedbackPeroid (dev, peroid);
    }
}

Time
SwitchMmu::GetCbfcFeedbackPeroid (Ptr<NetDevice> port, uint32_t qIndex)
{
  return DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->peroid;
}

uint64_t
SwitchMmu::GetCbfcFccl (Ptr<NetDevice> port, uint32_t qIndex)
{
  return DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->GetFccl ();
}

/*******************
 * CBPFC Functions *
 *******************/

void
SwitchMmu::ConfigCbpfcBufferSize (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::CBPFC)
    {
      DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->ingressSize = size;
    }
}

void
SwitchMmu::ConfigCbpfcBufferSize (Ptr<NetDevice> port, uint64_t size)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigCbpfcBufferSize (port, i, size);
    }
}

void
SwitchMmu::ConfigCbpfcBufferSize (uint32_t qIndex, uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbpfcBufferSize (dev, qIndex, size);
    }
}

void
SwitchMmu::ConfigCbpfcBufferSize (uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbpfcBufferSize (dev, size);
    }
}

void
SwitchMmu::ConfigCbpfcFeedbackPeroid (Ptr<NetDevice> port, uint32_t qIndex, Time peroid)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::CBPFC)
    {
      DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->peroid = peroid;
    }
}

void
SwitchMmu::ConfigCbpfcFeedbackPeroid (Ptr<NetDevice> port, Time peroid)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigCbpfcFeedbackPeroid (port, i, peroid);
    }
}

void
SwitchMmu::ConfigCbpfcFeedbackPeroid (uint32_t qIndex, Time peroid)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbpfcFeedbackPeroid (dev, qIndex, peroid);
    }
}

void
SwitchMmu::ConfigCbpfcFeedbackPeroid (Time peroid)
{
  for (const auto &dev : m_devices)
    {
      ConfigCbpfcFeedbackPeroid (dev, peroid);
    }
}

Time
SwitchMmu::GetCbpfcFeedbackPeroid (Ptr<NetDevice> port, uint32_t qIndex)
{
  return DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->peroid;
}

uint64_t
SwitchMmu::GetCbpfcFree (Ptr<NetDevice> port, uint32_t qIndex)
{
  return DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->GetFree ();
}

void
SwitchMmu::AddCbpfcReserve (Ptr<NetDevice> port, uint32_t qIndex, uint16_t time)
{
  DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->reserved += time * 64;
}

/*******************
 * CBPFC Functions *
 *******************/

void
SwitchMmu::ConfigPtpfcBufferSize (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size)
{
  const auto type = PfcSwitch::DeviceToL2Type (port);
  if (type == PfcSwitch::PTPFC)
    {
      DynamicCast<PtpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->ingressSize = size;
    }
}

void
SwitchMmu::ConfigPtpfcBufferSize (Ptr<NetDevice> port, uint64_t size)
{
  for (uint32_t i = 0; i <= m_nQueues; i++)
    {
      ConfigPtpfcBufferSize (port, i, size);
    }
}

void
SwitchMmu::ConfigPtpfcBufferSize (uint32_t qIndex, uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigPtpfcBufferSize (dev, qIndex, size);
    }
}

void
SwitchMmu::ConfigPtpfcBufferSize (uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigPtpfcBufferSize (dev, size);
    }
}

/*******************************************************
 * Common Functions for all L2 flow control algorithms *
 *******************************************************/

bool
SwitchMmu::CheckIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);

  const auto portType = PfcSwitch::DeviceToL2Type (port);
  if (portType == PfcSwitch::PFC)
    {
      auto queueConfig = DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);

      // Can not use shared buffer and headroom is full
      if (m_dynamicThreshold)
        {
          if (pSize + GetSharedBufferUsed (port, qIndex) > GetPfcThreshold (port, qIndex) &&
              pSize + queueConfig->headroomUsed > queueConfig->headroom)
            {
              return false;
            }
          return true;
        }
      else
        {
          if (pSize + GetSharedBufferUsed () > GetSharedBufferSize () &&
              pSize + queueConfig->headroomUsed > queueConfig->headroom)
            {
              return false;
            }
          return true;
        }
    }
  else if (portType == PfcSwitch::CBFC)
    {
      auto queueConfig = DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);
      if (pSize + queueConfig->ingressUsed > queueConfig->ingressSize)
        return false;
      return true;
    }
  else if (portType == PfcSwitch::CBPFC)
    {
      auto queueConfig = DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);
      if (pSize + queueConfig->ingressUsed > queueConfig->ingressSize)
        return false;
      return true;
    }
  else if (portType == PfcSwitch::PTPFC)
    {
      auto queueConfig = DynamicCast<PtpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);
      if (pSize + queueConfig->ingressUsed > queueConfig->ingressSize)
        return false;
      return true;
    }

  // Unknown port type
  return false;
}

bool
SwitchMmu::CheckEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);
  return true;
}

void
SwitchMmu::UpdateIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);

  const auto portType = PfcSwitch::DeviceToL2Type (port);
  if (portType == PfcSwitch::PFC)
    {
      auto queueConfig = DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);

      uint64_t newIngressUsed = queueConfig->ingressUsed + pSize;
      uint64_t reserve = queueConfig->reserve;

      if (newIngressUsed <= reserve) // using reserve buffer
        {
          queueConfig->ingressUsed += pSize;
        }
      else
        {
          uint64_t threshold;
          if (m_dynamicThreshold)
            threshold = GetPfcThreshold (port, qIndex);
          else
            threshold = GetSharedBufferSize ();

          uint64_t newSharedBufferUsed = newIngressUsed - reserve;
          if (newSharedBufferUsed > threshold) // using headroom
            {
              queueConfig->headroomUsed += pSize;
            }
          else // using shared buffer
            {
              queueConfig->ingressUsed += pSize;
            }
        }
    }
  else if (portType == PfcSwitch::CBFC)
    {
      auto queueConfig = DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);
      queueConfig->rxAbr += pSize;
      queueConfig->ingressUsed += pSize;
    }
  else if (portType == PfcSwitch::CBPFC)
    {
      auto queueConfig = DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);
      queueConfig->ingressUsed += pSize;
      queueConfig->reserved -= pSize;
    }
  else if (portType == PfcSwitch::PTPFC)
    {
      auto queueConfig = DynamicCast<PtpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);
      queueConfig->ingressUsed += pSize;
    }
  // Unknown port type
}

void
SwitchMmu::UpdateEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);
  m_switchMmuQueueConfig[port][qIndex]->egressUsed += pSize;
}

void
SwitchMmu::RemoveFromIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);

  const auto portType = PfcSwitch::DeviceToL2Type (port);
  if (portType == PfcSwitch::PFC)
    {
      auto queueConfig = DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex]);

      uint64_t fromHeadroom = std::min (queueConfig->headroomUsed, (uint64_t) pSize);
      queueConfig->headroomUsed -= fromHeadroom;
      queueConfig->ingressUsed -= pSize - fromHeadroom;
    }
  else if (portType == PfcSwitch::CBFC)
    {
      DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->ingressUsed -= pSize;
    }
  else if (portType == PfcSwitch::CBPFC)
    {
      DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->ingressUsed -= pSize;
    }
  else if (portType == PfcSwitch::PTPFC)
    {
      DynamicCast<PtpfcSwitchMmuQueue> (m_switchMmuQueueConfig[port][qIndex])->ingressUsed -= pSize;
    }
  // Unknown port type
}

void
SwitchMmu::RemoveFromEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);
  m_switchMmuQueueConfig[port][qIndex]->egressUsed -= pSize;
}

bool
SwitchMmu::CheckShouldSetEcn (Ptr<NetDevice> port, uint32_t qIndex)
{
  NS_LOG_FUNCTION (port << qIndex);

  if (qIndex >= m_nQueues) // control queue
    return false;

  uint64_t kMin = m_ecnConfig[port][qIndex].kMin;
  uint64_t kMax = m_ecnConfig[port][qIndex].kMax;
  double pMax = m_ecnConfig[port][qIndex].pMax;
  bool enable = m_ecnConfig[port][qIndex].enable;
  uint64_t qLen = m_switchMmuQueueConfig[port][qIndex]->egressUsed;

  if (!enable)
    return false; // ECN not enabled
  if (qLen > kMax)
    return true;
  if (qLen > kMin)
    {
      double p = pMax * double (qLen - kMin) / double (kMax - kMin);
      if (uniRand->GetValue (0, 1) < p)
        return true;
    }
  return false;
}

/*************************
 * Statistical functions *
 ************************/

uint64_t
SwitchMmu::GetBufferSize ()
{
  return m_bufferConfig;
}

uint64_t
SwitchMmu::GetSharedBufferSize ()
{
  uint64_t size = m_bufferConfig;
  for (const auto &dev : m_devices)
    {
      for (uint32_t i = 0; i <= m_nQueues; i++)
        {
          size -= m_switchMmuQueueConfig[dev][i]->GetBufferSize ();
        }
    }
  return size;
}

uint64_t
SwitchMmu::GetSharedBufferRemain ()
{
  return GetSharedBufferSize () - GetSharedBufferUsed ();
}

uint64_t
SwitchMmu::GetSharedBufferUsed (Ptr<NetDevice> port, uint32_t qIndex)
{
  return m_switchMmuQueueConfig[port][qIndex]->GetSharedBufferUsed ();
}

uint64_t
SwitchMmu::GetSharedBufferUsed (Ptr<NetDevice> port)
{
  uint64_t sum = 0;
  for (uint i = 0; i <= m_nQueues; i++)
    sum += GetSharedBufferUsed (port, i);
  return sum;
}

uint64_t
SwitchMmu::GetSharedBufferUsed ()
{
  uint64_t sum = 0;
  for (const auto &dev : m_devices)
    sum += GetSharedBufferUsed (dev);
  return sum;
}

uint64_t
SwitchMmu::GetBufferUsed (Ptr<NetDevice> port, uint32_t qIndex)
{
  return m_switchMmuQueueConfig[port][qIndex]->GetBufferUsed ();
}

uint64_t
SwitchMmu::GetBufferUsed (Ptr<NetDevice> port)
{
  uint64_t sum = 0;
  for (uint i = 0; i <= m_nQueues; i++)
    sum += GetBufferUsed (port, i);
  return sum;
}

uint64_t
SwitchMmu::GetBufferUsed ()
{
  uint64_t sum = 0;
  for (const auto &dev : m_devices)
    sum += GetBufferUsed (dev);
  return sum;
}

std::string
SwitchMmu::Dump ()
{
  std::stringstream ss;
  for (const auto &dev : m_devices)
    {
      for (uint32_t i; i <= m_nQueues; i++)
        {
          const auto portType = PfcSwitch::DeviceToL2Type (dev);
          if (portType == PfcSwitch::PFC)
            {
              auto queueConfig = DynamicCast<PfcSwitchMmuQueue> (m_switchMmuQueueConfig[dev][i]);

              ss << "Dev: " << dev << " Queue: " << i << '\n';
              ss << "Headroom: " << queueConfig->headroom << '\n';
              ss << "Reserve: " << queueConfig->reserve << '\n';
              ss << "ResumeOffset: " << queueConfig->resumeOffset << '\n';
            }
          else if (portType == PfcSwitch::CBFC)
            {
              auto queueConfig = DynamicCast<CbfcSwitchMmuQueue> (m_switchMmuQueueConfig[dev][i]);

              ss << "Dev: " << dev << " Queue: " << i << '\n';
              ss << "IngressSize: " << queueConfig->ingressSize << '\n';
              ss << "FeedbackPeriod: " << queueConfig->peroid << '\n';
            }
          else if (portType == PfcSwitch::CBPFC)
            {
              auto queueConfig = DynamicCast<CbpfcSwitchMmuQueue> (m_switchMmuQueueConfig[dev][i]);

              ss << "Dev: " << dev << " Queue: " << i << '\n';
              ss << "IngressSize: " << queueConfig->ingressSize << '\n';
              ss << "FeedbackPeriod: " << queueConfig->peroid << '\n';
            }
          else if (portType == PfcSwitch::PTPFC)
            {
              auto queueConfig = DynamicCast<PtpfcSwitchMmuQueue> (m_switchMmuQueueConfig[dev][i]);

              ss << "Dev: " << dev << " Queue: " << i << '\n';
              ss << "IngressSize: " << queueConfig->ingressSize << '\n';
            }
          // Unknown port type
          if (m_ecnConfig[dev][i].enable)
            {
              ss << "EcnConfig: " << m_ecnConfig[dev][i].kMin << ' ' << m_ecnConfig[dev][i].kMax
                 << ' ' << m_ecnConfig[dev][i].pMax << '\n';
            }
        }
    }
  return ss.str ();
}

void
SwitchMmu::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_switchMmuQueueConfig.clear ();
  m_ecnConfig.clear ();

  m_devices.clear ();

  Object::DoDispose ();
}

} // namespace ns3
