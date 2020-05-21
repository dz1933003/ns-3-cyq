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
SwitchMmu::AggregateDevice (Ptr<NetDevice> dev)
{
  NS_LOG_FUNCTION (dev);
  m_devices.push_back (dev);
  for (uint32_t i = 0; i <= m_nQueues; i++) // with one control queue
    {
      m_headroomConfig[dev].push_back (0);
      m_reserveConfig[dev].push_back (0);
      m_resumeOffsetConfig[dev].push_back (0);
      m_ecnConfig[dev].push_back ({0, 0, 0., false});

      m_headroomUsed[dev].push_back (0);
      m_ingressUsed[dev].push_back (0);
      m_egressUsed[dev].push_back (0);

      m_pausedStates[dev].push_back (false);
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
SwitchMmu::ConfigEcn (uint64_t kMin, uint64_t kMax, double pMax)
{
  for (const auto &dev : m_devices)
    {
      ConfigEcn (dev, kMin, kMax, pMax);
    }
}

void
SwitchMmu::ConfigHeadroom (Ptr<NetDevice> port, uint32_t qIndex, uint64_t size)
{
  m_headroomConfig[port][qIndex] = size;
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
  m_reserveConfig[port][qIndex] = size;
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
  m_resumeOffsetConfig[port][qIndex] = size;
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
SwitchMmu::ConfigResumeOffset (uint64_t size)
{
  for (const auto &dev : m_devices)
    {
      ConfigResumeOffset (dev, size);
    }
}

bool
SwitchMmu::CheckIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);
  // Can not use shared buffer and headroom is full
  if (m_dynamicThreshold)
    {
      if (pSize + GetSharedBufferUsed (port, qIndex) > GetPfcThreshold (port, qIndex) &&
          pSize + m_headroomUsed[port][qIndex] > m_headroomConfig[port][qIndex])
        {
          return false;
        }
      return true;
    }
  else
    {
      if (pSize + GetSharedBufferUsed () > GetSharedBufferSize () &&
          pSize + m_headroomUsed[port][qIndex] > m_headroomConfig[port][qIndex])
        {
          return false;
        }
      return true;
    }
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

  uint64_t newIngressUsed = m_ingressUsed[port][qIndex] + pSize;
  uint64_t reserve = m_reserveConfig[port][qIndex];

  if (newIngressUsed <= reserve) // using reserve buffer
    {
      m_ingressUsed[port][qIndex] += pSize;
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
          m_headroomUsed[port][qIndex] += pSize;
        }
      else // using shared buffer
        {
          m_ingressUsed[port][qIndex] += pSize;
        }
    }
}

void
SwitchMmu::UpdateEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);
  m_egressUsed[port][qIndex] += pSize;
}

void
SwitchMmu::RemoveFromIngressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);

  uint64_t fromHeadroom = std::min (m_headroomUsed[port][qIndex], (uint64_t) pSize);
  m_headroomUsed[port][qIndex] -= fromHeadroom;
  m_ingressUsed[port][qIndex] -= pSize - fromHeadroom;
}

void
SwitchMmu::RemoveFromEgressAdmission (Ptr<NetDevice> port, uint32_t qIndex, uint32_t pSize)
{
  NS_LOG_FUNCTION (port << qIndex << pSize);
  m_egressUsed[port][qIndex] -= pSize;
}

bool
SwitchMmu::CheckShouldSendPfcPause (Ptr<NetDevice> port, uint32_t qIndex)
{
  NS_LOG_FUNCTION (port << qIndex);

  if (m_dynamicThreshold)
    return (m_pausedStates[port][qIndex] == false) &&
           (m_headroomUsed[port][qIndex] > 0 ||
            GetSharedBufferUsed (port, qIndex) >= GetPfcThreshold (port, qIndex));
  else
    return m_pausedStates[port][qIndex] == false && m_headroomUsed[port][qIndex] > 0;
}

bool
SwitchMmu::CheckShouldSendPfcResume (Ptr<NetDevice> port, uint32_t qIndex)
{
  NS_LOG_FUNCTION (port << qIndex);

  if (m_pausedStates[port][qIndex] == false)
    return false;

  uint64_t sharedBufferUsed = GetSharedBufferUsed (port, qIndex);
  if (m_dynamicThreshold)
    return (m_headroomUsed[port][qIndex] == 0) &&
           (sharedBufferUsed == 0 || sharedBufferUsed + m_resumeOffsetConfig[port][qIndex] <=
                                         GetPfcThreshold (port, qIndex));
  else
    return (m_headroomUsed[port][qIndex] == 0) &&
           (sharedBufferUsed == 0 ||
            GetSharedBufferUsed () + m_resumeOffsetConfig[port][qIndex] <= GetSharedBufferSize ());
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
  uint64_t qLen = m_egressUsed[port][qIndex];

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

void
SwitchMmu::SetPause (Ptr<NetDevice> port, uint32_t qIndex)
{
  m_pausedStates[port][qIndex] = true;
}

void
SwitchMmu::SetResume (Ptr<NetDevice> port, uint32_t qIndex)
{
  m_pausedStates[port][qIndex] = false;
}

uint64_t
SwitchMmu::GetPfcThreshold (Ptr<NetDevice> port, uint32_t qIndex)
{
  // XXX cyq: add dynamic PFC threshold choice if needed
  return 0;
}

uint64_t
SwitchMmu::GetBufferSize ()
{
  return m_bufferConfig;
}

uint64_t
SwitchMmu::GetHeadroomSize (Ptr<NetDevice> port, uint32_t qIndex)
{
  return m_headroomConfig[port][qIndex];
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

uint64_t
SwitchMmu::GetSharedBufferSize ()
{
  uint64_t size = m_bufferConfig;
  for (const auto &dev : m_devices)
    {
      for (uint32_t i = 0; i <= m_nQueues; i++)
        {
          size -= m_headroomConfig[dev][i];
          size -= m_reserveConfig[dev][i];
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
  uint64_t used = m_ingressUsed[port][qIndex];
  uint64_t reserve = m_reserveConfig[port][qIndex];
  return used > reserve ? used - reserve : 0;
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

void
SwitchMmu::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_headroomConfig.clear ();
  m_reserveConfig.clear ();
  m_resumeOffsetConfig.clear ();
  m_ecnConfig.clear ();

  m_headroomUsed.clear ();
  m_ingressUsed.clear ();
  m_egressUsed.clear ();

  m_devices.clear ();
  m_pausedStates.clear ();

  Object::DoDispose ();
}

} // namespace ns3
