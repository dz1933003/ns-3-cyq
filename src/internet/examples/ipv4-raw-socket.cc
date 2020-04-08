/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2020 Nanjing University
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
 * Author: Chen Yanqing
 * Description:
 *   Modified from examples/tutorial/first.cc
 *   Test Ipv4RawSocket and InetSocketAddress tos settings
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Ipv4RawSocketExample");

class MyApp : public Application
{
public:
  MyApp ();
  virtual ~MyApp ();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId (void);
  void Setup (Ptr<Socket> socket, Address address, bool isSend);

private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void SendPacket (void);
  void RecvPacket (Ptr<Socket> socket);

  Ptr<Socket> m_socket;
  Address m_peer;
  EventId m_sendEvent;
  bool m_running;
  bool isSend;
};

MyApp::MyApp () : m_socket (0), m_peer (), m_sendEvent (), m_running (false), isSend (true)
{
}

MyApp::~MyApp ()
{
  m_socket = 0;
}

/* static */
TypeId
MyApp::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("MyApp").SetParent<Application> ().SetGroupName ("Tutorial").AddConstructor<MyApp> ();
  return tid;
}

void
MyApp::Setup (Ptr<Socket> socket, Address address, bool isSend)
{
  m_socket = socket;
  m_peer = address;
  this->isSend = isSend;
}

void
MyApp::StartApplication (void)
{
  m_running = true;
  m_socket->Bind ();
  m_socket->Connect (m_peer);
  if (isSend)
    SendPacket ();
  else
    {
      m_socket->SetRecvCallback (MakeCallback (&MyApp::RecvPacket, this));
    }
}

void
MyApp::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
    {
      Simulator::Cancel (m_sendEvent);
    }

  if (m_socket)
    {
      m_socket->Close ();
    }
}

void
MyApp::SendPacket (void)
{
  std::string payload = "hello world";
  m_socket->Send ((uint8_t *) payload.c_str (), payload.size (), 0);
  m_sendEvent = Simulator::Schedule (Time (Seconds (1)), &MyApp::SendPacket, this);
  NS_LOG_UNCOND ("send");
}

void
MyApp::RecvPacket (Ptr<Socket> socket)
{
  Address from;
  Ptr<Packet> packet = socket->RecvFrom (from);
  Ipv4Header ipv4Header;
  packet->RemoveHeader (ipv4Header);
  uint8_t buffer[1024];
  memset (buffer, 0, 1024);
  packet->CopyData (buffer, packet->GetSize ());
  std::string payload = (char *) buffer;
  NS_LOG_UNCOND ("payload: " + payload);
  NS_LOG_UNCOND ("recv");
}

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  InetSocketAddress sendAddress = InetSocketAddress (interfaces.GetAddress (1), 1);
  sendAddress.SetTos (0xf0);
  InetSocketAddress recvAddress = InetSocketAddress (Ipv4Address::GetAny (), 1);

  Ptr<Socket> sendSocket = Socket::CreateSocket (nodes.Get (0), Ipv4RawSocketFactory::GetTypeId ());
  Ptr<Socket> recvSocket = Socket::CreateSocket (nodes.Get (1), Ipv4RawSocketFactory::GetTypeId ());

  sendSocket->Bind ();
  recvSocket->Bind ();

  sendSocket->Connect (sendAddress);
  recvSocket->Connect (recvAddress);

  Ptr<MyApp> sendApp = CreateObject<MyApp> ();
  sendApp->Setup (sendSocket, sendAddress, 1);
  nodes.Get (0)->AddApplication (sendApp);
  sendApp->SetStartTime (Seconds (2));
  sendApp->SetStopTime (Seconds (3));

  Ptr<MyApp> recvApp = CreateObject<MyApp> ();
  recvApp->Setup (recvSocket, recvAddress, 0);
  nodes.Get (0)->AddApplication (recvApp);
  recvApp->SetStartTime (Seconds (1));
  recvApp->SetStopTime (Seconds (4));

  pointToPoint.EnablePcapAll ("ipv4-raw-socket-test");

  ns3::PacketMetadata::Enable ();

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
