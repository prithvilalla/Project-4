/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include <iostream>
#include <time.h>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "worm.h"


bool succeeded = false; 

using namespace ns3;
using namespace std;

uint32_t Worm::firstIP = 0;
uint32_t Worm::lastIP = 0;
TypeId Worm::m_typeId = UdpSocketFactory::GetTypeId ();
vector<string> Worm::m_listInfected;
vector<double> Worm::m_time;
uint32_t Worm::m_connections = 1;
uint32_t Worm::m_packetSize = 256;
DataRate Worm::m_dataRate = DataRate ("0.5Mbps");

Worm::Worm ():
  m_node (NULL), 
  m_address (Ipv4Address ()),
  m_port (0),
  m_rxSocket (NULL),
  m_sinkAddress (Address ()),
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
  for (uint32_t i = 0; i < m_connections; i++)
  m_txSocket[i] = NULL;
  m_rxSocket = NULL;
}

void
Worm::Setup (Ptr<Node> node, Ipv4Address address, uint32_t port, bool infected, bool vulnerable, string ID)
{
  m_node = node;
  m_address = address;
  m_port = port;
  
  for (uint32_t i = 0; i < m_connections; i++)
  {
    m_txSocket.push_back (Socket::CreateSocket (node, m_typeId));
    m_sourceAddress.push_back (InetSocketAddress (m_address, m_port-1-i));
  }
  
  m_rxSocket = Socket::CreateSocket (node, m_typeId);
  m_sinkAddress = InetSocketAddress (m_address, m_port);
  m_infected = infected;
  m_vulnerable = vulnerable;
  m_ID = ID;
}

void
Worm::StartApplication (void)
{
  srand (time (NULL));
  RngSeedManager::SetSeed (rand () % 1000 + 1);
  m_running = true;
  m_packetsSent = 0;
  m_packetsReceived = 0;
  for (uint32_t i = 0; i < m_connections; i++)
  {
    m_txSocket[i]->Bind (m_sourceAddress[i]);
    if (m_typeId == TcpSocketFactory::GetTypeId ())
    {
      m_txSocket[i]->SetConnectCallback (MakeCallback (&Worm::ConnectionSucceeded, this), MakeCallback (&Worm::ConnectionFailed, this));
      m_txSocket[i]->SetDataSentCallback (MakeCallback (&Worm::Sending, this));
    }
  }
  
  m_rxSocket->Bind (m_sinkAddress);
  m_rxSocket->Listen ();
  m_rxSocket->SetRecvCallback (MakeCallback (&Worm::HandleRead, this));
  m_rxSocket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (), MakeCallback (&Worm::HandleAccept, this));
     
  if(m_infected)
  {
    m_listInfected.push_back (m_ID);
    m_time.push_back (Simulator::Now().GetSeconds());
    CreateListIP ();
    if (m_typeId == UdpSocketFactory::GetTypeId ())
      Simulator::ScheduleNow (&Worm::SendPacketUDP, this);
    else
      Simulator::ScheduleNow (&Worm::SearchTCP, this);
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

  for (uint32_t i = 0; i < m_connections; i++)
  {
    if (m_txSocket[i])
    {
      m_txSocket[i]->Close ();
    }
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
Worm::SendPacketUDP (void)
{

  m_destinationAddress = InetSocketAddress (GetDestinationIP (), m_port);
  m_txSocket[0]->Connect (m_destinationAddress);
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  m_txSocket[0]->Send (packet);
  m_packetsSent++;
  Simulator::ScheduleNow (&Worm::ScheduleTxUDP, this);
}

void 
Worm::ScheduleTxUDP (void)
{
  if (m_running)
  {
    Time tNext (Seconds (m_packetSize * 8 / static_cast<double> (m_dataRate.GetBitRate ())));
    m_sendEvent = Simulator::Schedule (tNext, &Worm::SendPacketUDP, this);
  }
}

void 
Worm::SearchTCP (void)
{
  //cout<<m_ID<<" searching "<<Simulator::Now().GetSeconds()<<endl;
  //getchar();
  for (uint32_t i = 0; i < m_connections; i++)
  {
    m_destinationAddress = InetSocketAddress (GetDestinationIP (), m_port);
    m_txSocket[i]->Connect (m_destinationAddress);
  }
  //Simulator::ScheduleNow (&Worm::SearchTCP, this);
  if(succeeded == false)
{
 // cout<<m_ID<<" Worm ConnectionSucceed "<<Simulator::Now ().GetSeconds ()<<endl;
  Simulator::Schedule (Time (Seconds (0.05)), &Worm::SearchTCP, this);
   
}
 else if(succeeded == true)
  {
  // cout<<m_ID<<" Worm ConnectionFailed "<<Simulator::Now ().GetSeconds ()<<endl;

   Simulator::Schedule (Time (Seconds (0.00)), &Worm::SearchTCP, this);
   

  }
succeeded = false;
}

void
Worm::SendPacketTCP (Ptr<Socket> socket)
{
  Ptr<Packet> packet = Create<Packet> (m_packetSize);
  //cout<<m_ID<<" transmitting "<<Simulator::Now().GetSeconds()<<endl;
  socket->Send (packet);
  m_packetsSent++;
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
      if (m_typeId == UdpSocketFactory::GetTypeId ())
        SendPacketUDP();
      else
      {
        //cout<<m_ID<<" infected "<<Simulator::Now ().GetSeconds ()<<endl;
        Simulator::ScheduleNow (&Worm::SearchTCP, this);
      }
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
Worm::SetType (TypeId typeId)
{
  m_typeId = typeId;
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
  //cout<<m_ID<<" IP = "<<temp<<endl;
  m_listIP.erase (m_listIP.begin () + i);
  return Ipv4Address (temp);
}

void
Worm::SetConnections (uint32_t connections)
{
  m_connections = connections;
}

void
Worm::SetPacketSize (uint32_t packetSize)
{
  m_packetSize = packetSize;
}

void
Worm::SetDataRate (DataRate dataRate)
{
  m_dataRate = dataRate;
}

void
Worm::ConnectionSucceeded (Ptr<Socket> socket)
{
  succeeded = true;
  cout<<m_ID<<" Worm ConnectionSucceed "<<Simulator::Now ().GetSeconds ()<<endl;
  m_sendEvent = Simulator::ScheduleNow (&Worm::SendPacketTCP, this, socket);
}

void
Worm::ConnectionFailed (Ptr<Socket> socket)
{
  cout<<m_ID<<" Worm ConnectionFailed "<<Simulator::Now ().GetSeconds ()<<endl;
  //socket->Close ();
}

void
Worm::Sending (Ptr<Socket> socket, uint32_t txSpace)
{
  //cout<<m_ID<<" Worm Data Sent "<<txSpace<<" "<<Simulator::Now ().GetSeconds ()<<endl;
}
