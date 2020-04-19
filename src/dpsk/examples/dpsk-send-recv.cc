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

// Network topology
//
// n0 --- n1

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/dpsk-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DpskSendRecvExample");

class DpskSendApp : public DpskApplication
{
public:
  DpskSendApp ()
      : m_dpsk (NULL),
        m_running (false),
        m_packetsSent (0),
        m_nPackets (0),
        m_interval (1),
        m_sendEvent ()
  {
  }

  virtual ~DpskSendApp ()
  {
    m_dpsk = NULL;
  }

  void
  Setup (Ptr<Dpsk> dpsk, uint32_t nPackets, uint32_t interval)
  {
    m_dpsk = dpsk;
    m_nPackets = nPackets;
    NS_ASSERT_MSG (interval > 0, "Interval must greater than zero");
    m_interval = interval;
  }

private:
  virtual void
  StartApplication (void)
  {
    m_running = true;
    m_packetsSent = 0;
    SendPacket ();
  }

  virtual void
  StopApplication (void)
  {
    m_running = false;
    if (m_sendEvent.IsRunning ())
      {
        Simulator::Cancel (m_sendEvent);
      }
  }

  void
  SendPacket (void)
  {
    const uint8_t msg[] = "hello world!";
    Ptr<Packet> packet = Create<Packet> (msg, sizeof (msg));
    m_dpsk->SendFromDevice (NULL, packet->Copy (), 0x0800, Mac48Address (), Mac48Address ());
    NS_LOG_UNCOND (Simulator::Now () << ": Send Packet");

    if (++m_packetsSent < m_nPackets)
      HandleTx ();
  }

  void
  HandleTx (void)
  {
    if (m_running)
      {
        Time tNext (Seconds (m_interval));
        m_sendEvent = Simulator::Schedule (tNext, &DpskSendApp::SendPacket, this);
      }
  }

  Ptr<Dpsk> m_dpsk;
  bool m_running;
  uint32_t m_packetsSent;
  uint32_t m_nPackets;
  uint32_t m_interval;
  EventId m_sendEvent;
};

class DpskRecvApp : public DpskApplication
{
public:
  DpskRecvApp () : m_dpsk (NULL), m_running (false)
  {
  }

  virtual ~DpskRecvApp ()
  {
    m_dpsk = NULL;
  }

  void
  Setup (Ptr<Dpsk> dpsk)
  {
    m_dpsk = dpsk;
  }

private:
  virtual void
  StartApplication (void)
  {
    m_running = true;
    m_dpsk->RegisterReceiveFromDeviceHandler (MakeCallback (&DpskRecvApp::HandleRx, this));
  }

  virtual void
  StopApplication (void)
  {
    m_running = false;
    m_dpsk->UnregisterReceiveFromDeviceHandler (MakeCallback (&DpskRecvApp::HandleRx, this));
  }

  virtual void
  HandleRx (Ptr<NetDevice> incomingPort, Ptr<const Packet> packet, uint16_t protocol,
            Address const &src, Address const &dst, NetDevice::PacketType packetType)
  {
    uint8_t buffer[2048] = {0};
    packet->CopyData (buffer, packet->GetSize ());
    NS_LOG_UNCOND (Simulator::Now () << ": Receive Packet -- " << buffer);
  }

  Ptr<Dpsk> m_dpsk;
  bool m_running;
};

int
main (int argc, char *argv[])
{
  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Explicitly create the nodes required by the topology (shown above).
  NodeContainer nodePair;
  nodePair.Create (2);
  Ptr<Node> senderNode = nodePair.Get (0);
  Ptr<Node> recverNode = nodePair.Get (1);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer link = csma.Install (nodePair);
  Ptr<NetDevice> senderDevice = link.Get (0);
  Ptr<NetDevice> recverDevice = link.Get (1);

  // Install Dpsk on both nodes
  DpskHelper dpskHelper;
  Ptr<Dpsk> senderDpsk = dpskHelper.Install (senderNode, senderDevice);
  Ptr<Dpsk> recverDpsk = dpskHelper.Install (recverNode, recverDevice);

  Ptr<DpskSendApp> senderApp = CreateObject<DpskSendApp> ();
  senderApp->Setup (senderDpsk, 10, 1);
  senderApp->SetStartTime (Seconds (0));
  senderApp->SetStopTime (Seconds (10));
  senderNode->AddApplication (senderApp);

  Ptr<DpskRecvApp> recverApp = CreateObject<DpskRecvApp> ();
  recverApp->Setup (recverDpsk);
  recverApp->SetStartTime (Seconds (0));
  recverApp->SetStopTime (Seconds (10));
  recverNode->AddApplication (recverApp);

  Simulator::Stop (Seconds (10));
  Simulator::Run ();
  Simulator::Destroy ();
}
