/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#ifndef WORM_H
#define WORM_H

#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

using namespace ns3;
using namespace std;

class Worm : public Application 
{
public:

  Worm ();
  virtual ~Worm ();
  
  void Setup (Ptr<Node> node, Ipv4Address address, uint32_t port, bool infected, bool vulnerable, string ID);
  
  uint32_t GetTotalTx (void);
  uint32_t GetTotalRx (void);
  string GetID (void);
  
  static vector<string> GetListInfected (void);
  static vector<double> GetTime (void);
  
  static void SetType (TypeId typeId);
  static void SetFirstIP (uint32_t first);
  static void SetLastIP (uint32_t last);
  static void SetConnections (uint32_t connections);
  static void SetPacketSize (uint32_t packetSize);
  static void SetDataRate (DataRate dataRate);
  
private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTxUDP (void);
  void SendPacketUDP (void);
  
  void ScheduleTxTCP (Ptr<Socket> socket, uint32_t txSpace);
  void SearchTCP (void);
  void SendPacketTCP (Ptr<Socket> socket);
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);
  void Sending (Ptr<Socket> socket, uint32_t txSpace); 
  
  void HandleRead (Ptr<Socket> socket);
  void HandleAccept (Ptr<Socket> s, const Address& from);
  
  void CreateListIP (void);
  Ipv4Address GetDestinationIP (void);

  Ptr<Node>                   m_node;
  Ipv4Address                 m_address;
  uint32_t                    m_port;
  vector<Ptr<Socket> >        m_txSocket;
  Ptr<Socket>                 m_rxSocket;
  list<Ptr<Socket> >          m_rxSocketList;
  Address                     m_destinationAddress;
  Address                     m_sinkAddress;
  vector<Address>             m_sourceAddress;
  EventId                     m_sendEvent;
  bool                        m_running;
  uint32_t                    m_packetsSent;
  uint32_t                    m_packetsReceived;
  bool                        m_infected;
  bool                        m_vulnerable;
  string                      m_ID;
  vector<uint32_t>            m_listIP;
  Ptr<UniformRandomVariable>  U;
  
  static TypeId               m_typeId;
  static uint32_t             firstIP;
  static uint32_t             lastIP;
  static vector<string>       m_listInfected;
  static vector<double>       m_time;
  static uint32_t             m_connections;
  static uint32_t             m_packetSize;
  static DataRate             m_dataRate;
};

#endif /* Worm_H */
