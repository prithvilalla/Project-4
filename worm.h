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
  
  void Setup (Ptr<Node> node, Ipv4Address address, uint32_t port, uint32_t packetSize, uint32_t nPackets, DataRate dataRate, TypeId typeId, bool infected, bool vulnerable, string ID);
  uint32_t GetTotalTx (void);
  uint32_t GetTotalRx (void);
  string GetID (void);
  
  static void SetFirstIP (uint32_t first);
  static void SetLastIP (uint32_t last);
  static vector<string> GetListInfected (void);
  static vector<double> GetTime (void);
  
private:
  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void ScheduleTx (void);
  void SendPacket (void);
  void HandleRead (Ptr<Socket> socket);
  void HandleAccept (Ptr<Socket> s, const Address& from);
  void CreateListIP (void);
  Ipv4Address GetDestinationIP (void);

  Ptr<Node>                   m_node;
  Ipv4Address                 m_address;
  uint32_t                    m_port;
  Ptr<Socket>                 m_txSocket;
  Ptr<Socket>                 m_rxSocket;
  list<Ptr<Socket> >          m_rxSocketList;
  Address                     m_destinationAddress;
  Address                     m_sinkAddress;
  Address                     m_sourceAddress;
  uint32_t                    m_packetSize;
  uint32_t                    m_nPackets;
  DataRate                    m_dataRate;
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
};

#endif /* Worm_H */
