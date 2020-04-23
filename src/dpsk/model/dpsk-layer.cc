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

#include "dpsk-layer.h"
#include "ns3/packet.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DpskLayer");

NS_OBJECT_ENSURE_REGISTERED (DpskLayer);

TypeId
DpskLayer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DpskLayer")
                          .SetParent<Object> ()
                          .SetGroupName ("Dpsk")
                          .AddConstructor<DpskLayer> ();
  return tid;
}

DpskLayer::DpskLayer ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dpsk = NULL;
  m_name = "";
}

DpskLayer::~DpskLayer ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

std::string
DpskLayer::GetName () const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_name;
}

void
DpskLayer::SetName (std::string name)
{
  NS_LOG_FUNCTION (name);
  m_name = name;
}

bool
DpskLayer::SendFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                           const Address &source, const Address &destination)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination);

  if (m_dpsk != NULL)
    {
      m_dpsk->SendFromDevice (device, packet->Copy (), protocol, source, destination);
    }

  return true;
}

void
DpskLayer::ReceiveFromDevice (Ptr<NetDevice> device, Ptr<const Packet> packet, uint16_t protocol,
                              const Address &source, const Address &destination,
                              NetDevice::PacketType packetType)
{
  NS_LOG_FUNCTION (device << packet << protocol << &source << &destination << packetType);

  for (auto iter = m_appHandlers.begin (); iter != m_appHandlers.end (); iter++)
    {
      (*iter) (device, packet->Copy (), protocol, source, destination, packetType);
    }
}

void
DpskLayer::RegisterReceiveFromDeviceHandler (ReceiveFromDeviceHandler handler)
{
  NS_LOG_FUNCTION (&handler);
  m_appHandlers.push_back (handler);
}

void
DpskLayer::UnregisterReceiveFromDeviceHandler (ReceiveFromDeviceHandler handler)
{
  NS_LOG_FUNCTION (&handler);

  for (auto iter = m_appHandlers.begin (); iter != m_appHandlers.end (); iter++)
    {
      if (iter->IsEqual (handler))
        {
          m_appHandlers.erase (iter);
          break;
        }
    }
}

void
DpskLayer::RegisterLayerSendDownward (Ptr<DpskLayer> layer)
{
  NS_LOG_FUNCTION (layer);
  m_dpskLayers[layer->GetName ()] = layer;
}

void
DpskLayer::RegisterLayerReceiveUpward (Ptr<DpskLayer> layer)
{
  NS_LOG_FUNCTION (layer);
  m_layerHandlers[layer->GetName ()] = MakeCallback (&DpskLayer::ReceiveFromDevice, layer);
}

void
DpskLayer::InstallUpperLayer (Ptr<DpskLayer> layer)
{
  NS_LOG_FUNCTION (layer);
  layer->RegisterLayerSendDownward (this);
  RegisterLayerReceiveUpward (layer);
}

void
DpskLayer::InstallLowerLayer (Ptr<DpskLayer> layer)
{
  NS_LOG_FUNCTION (layer);
  layer->RegisterLayerReceiveUpward (this);
  RegisterLayerSendDownward (layer);
}

void
DpskLayer::InstallDpsk (Ptr<Dpsk> dpsk)
{
  NS_LOG_FUNCTION (dpsk);
  m_dpsk = dpsk;
  m_dpsk->RegisterReceiveFromDeviceHandler (MakeCallback (&DpskLayer::ReceiveFromDevice, this));
}

void
DpskLayer::DoDispose ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_dpskLayers.clear ();
  m_dpsk = NULL;
  m_layerHandlers.clear ();
  m_appHandlers.clear ();
  Object::DoDispose ();
}

} // namespace ns3
