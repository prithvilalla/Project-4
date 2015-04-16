/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "worm.h"

using namespace ns3;
using namespace std;

uint32_t Worm::firstIP = 0;
uint32_t Worm::lastIP = 0;
TypeId Worm::m_typeId = UdpSocketFactory::GetTypeId ();
vector<string> Worm::m_listInfected;
vector<double> Worm::m_time;

Worm::Worm ():
  m_node (NULL), 
  m_address (Ipv4Address ()),
  m_port (0),
  m_txSocket (NULL),
  m_rxSocket (NULL),
  m_destinationAddress (Address ()),
  m_sinkAddress (Address ()),
  m_sourceAddress (Address ()),
  m_packetSize (0), 
  m_nPackets (0), 
  m_dataRate (0), 
  m_sendEvent (), 
  m_running (false), 
  m_packetsSent (0),
  m_packetsReceived (0),
  m_infected (false),
  m_vulnerable (false),
  U (CreateObject<UniformRandomVariable> ())
{
}

Worm::~Worm ()
{
  m_txSocket = NULL;
  m_rxSocket = NULL;
}

void
Worm::Setup (Ptr<Node> node, Ipv4Address address, uint32_t port, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, TypeId typeId, bool infected, bool vulnerable, string ID)
{
  m_node = node;
  m_address = address;
  m_port = port;
  m_typeId = typeId;
  m_txSocket = Socket::CreateSocket (node, m_typeId);
  m_rxSocket = Socket::CreateSocket (node, m_typeId);
  m_sinkAddress = InetSocketAddress (address, port);
  m_sourceAddress = InetSocketAddress (address, port-1);
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
  m_infected = infected;
  m_vulnerable = vulnerable;
  m_ID = ID;
}

void
Worm::StartApplication (void)
{
  m_running = true;
  m_packetsSent = 0;
  m_packetsReceived = 0;
  m_txSocket->Bind (m_sourceAddress);
  m_rxSocket->Bind (m_sinkAddress);
  m_rxSocket->Listen ();
  m_rxSocket->ShutdownSend ();
  m_rxSocket->SetRecvCallback (MakeCallback (&Worm::HandleRead, this));
  m_rxSocket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (), MakeCallback (&Worm::HandleAccept, this));
   
  if(m_infected)
  {
    m_listInfected.push_back (m_ID);
    m_time.push_back (Simulator::Now().GetSeconds());
    CreateListIP ();
    SendPacket ();
  }
}

void 
Worm::StopApplication (void)
{
  m_running = false;

  if (m_sendEvent.IsRunning ())
  {
    Simulator::Cancel (m_sendEvent);
  }

  if (m_txSocket)
  {
    m_txSocket->Close ();
  }
    
  while(!m_rxSocketList.empty ())
  {
    Ptr<Socket> acceptedSocket = m_rxSocketList.front ();
    m_rxSocketList.pop_front ();
    acceptedSocket->Close ();
  }
  if (m_rxSocket) 
  {
    m_rxSocket->Close ();
    m_rxSocket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
  }
}

void 
Worm::SendPacket (void)
{
  m_destinationAddress = InetSocketAddress (GetDestinationIP (), m_port);
  m_txSocket->Connect (m_destinationAddress);
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_txSocket->Send (packet);

  if(m_nPackets == 0)
  {
    m_packetsSent++;
    ScheduleTx ();
  }
  else if (m_packetsSent++ < m_nPackets)
  {
    ScheduleTx ();
  }
}

void 
Worm::ScheduleTx (void)
{
  if (m_running)
  {
    Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
    m_sendEvent = Simulator::Schedule (tNext, &Worm::SendPacket, this);
  }
}

void
Worm::HandleRead (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
  {
    if (packet->GetSize () == 0)
    {
      break;
    }
    m_packetsReceived++;
    if(!m_infected && m_vulnerable)
    {
      m_infected = true;
      m_listInfected.push_back (m_ID);
      m_time.push_back (Simulator::Now().GetSeconds());
      CreateListIP ();
      SendPacket();
    }
  }
}

void
Worm::HandleAccept (Ptr<Socket> s, const Address& from)
{
  s->SetRecvCallback (MakeCallback (&Worm::HandleRead, this));
  m_rxSocketList.push_back (s);
}

uint32_t
Worm::GetTotalTx(void)
{
  return m_packetsSent;
}

uint32_t
Worm::GetTotalRx(void)
{
  return m_packetsReceived;
}

void
Worm::SetFirstIP (uint32_t first)
{
  firstIP = first;
}

void
Worm::SetLastIP (uint32_t last)
{
  lastIP = last;
}

string
Worm::GetID (void)
{
  return m_ID;
}

vector<string>
Worm::GetListInfected (void)
{
  return m_listInfected;
}

vector<double>
Worm::GetTime (void)
{
  return m_time;
}

void
Worm::CreateListIP (void)
{
  for (uint32_t i = firstIP; i <= lastIP; i++)
    m_listIP.push_back (i);
}

Ipv4Address
Worm::GetDestinationIP (void)
{
  if (!m_listIP.size ())
    CreateListIP ();
  uint32_t i = U->GetInteger (0, m_listIP.size ()-1);
  uint32_t temp = m_listIP[i];
  m_listIP.erase (m_listIP.begin () + i);
  return Ipv4Address (temp);
}
