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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/random-variable-stream.h"
#include "ns3/csma-module.h"
#include "ns3/worm-module.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>
#include <fstream>

using namespace std;
using namespace ns3;

uint32_t i = 0;
uint32_t j = 0;
uint32_t k = 0;
uint32_t seed = 123;
uint32_t fanout = 10;
uint32_t trees = 10;
double utilization = 0.75;
uint32_t created = 0;
uint32_t missed = 0;
uint32_t total = 0;
double stopTime = 20.0;
string wormTypeCheck = "tcp";
uint32_t packetSize = 128;
double vulnerability = 0.75;
string dataRate = "0.1Mbps";
uint32_t connections = 1;

int main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("seed", "Random seed value", seed);
  cmd.AddValue ("fanout", "Fanout value", fanout);
  cmd.AddValue ("utilization", "Utilization value", utilization);
  cmd.AddValue ("trees", "Number of trees", trees);
  cmd.AddValue ("stopTime", "Stop Time", stopTime);
  cmd.AddValue ("wormTypeCheck", "Type of worm", wormTypeCheck);
  cmd.AddValue ("packetSize", "Size of packet in bytes", packetSize);
  cmd.AddValue ("vulnerability", "Vulnerability probability", vulnerability);
  cmd.AddValue ("dataRate", "Data rate in Mbps", dataRate);
  cmd.AddValue ("connections", "Number of parallel connections", connections);
  cmd.Parse (argc,argv);
  
  TypeId wormType;
  if (wormTypeCheck == "udp")
  {
    wormType = UdpSocketFactory::GetTypeId ();
    connections = 1;
    Worm::SetConnections (connections);
    Worm::SetType (wormType);
  }
  else
  {
    wormType = TcpSocketFactory::GetTypeId ();
    Worm::SetConnections (connections);
    Worm::SetType (wormType);
  }
  
  Worm::SetPacketSize (packetSize);
  Worm::SetNPackets (0);
  Worm::SetDataRate (DataRate (dataRate));
  
  double probability = exp (log (utilization)/2);
  
  RngSeedManager::SetSeed (seed);
  Ptr<UniformRandomVariable> U = CreateObject<UniformRandomVariable> ();
  
  Config::SetDefault ("ns3::OnOffApplication::DataRate", DataRateValue (DataRate ("0.5Mbps")));
  Config::SetDefault ("ns3::OnOffApplication::OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"));
  Config::SetDefault ("ns3::OnOffApplication::OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.5]"));
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (packetSize));
  Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpTahoe::GetTypeId ()));
  Config::SetDefault ("ns3::TcpSocketBase::MaxWindowSize", UintegerValue (64000));
  Config::SetDefault ("ns3::TcpSocketBase::WindowScaling", BooleanValue (false));
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (packetSize));
  
  NodeContainer center;
  center.Create (1);
  
  NodeContainer *tree = new NodeContainer[trees];
  NodeContainer **child1 = new NodeContainer*[trees];
  NodeContainer ***child2 = new NodeContainer**[trees];
  
  for (i = 0; i < trees; i++)
  {
    tree[i].Create (1);
    //cout<<"Creating tree "<<i<<endl;
    child1[i] = new NodeContainer[fanout];
    child2[i] = new NodeContainer*[fanout];
    for (j = 0; j < fanout; j++)
    {
      if (U->GetValue (0.0, 1.0) <= probability)
      {
        created++;
        //cout<<"Node "<<i<<j<<endl;
        child1[i][j].Create (1);
        child2[i][j] = new NodeContainer[fanout];
        for (k = 0; k < fanout; k++)
        {
          if (U->GetValue (0.0,1.0) <= probability)
          {
            created++;
            //cout<<"Node "<<i<<j<<k<<endl;
            child2[i][j][k].Create (1);
          }
          else
          {
            missed++;
            //cout<<"Node "<<i<<j<<k<<" not created\n";
          }
        }
      }
      else
      {
        missed += 1 + fanout;
        //cout<<"Node "<<i<<j<<" not created\n";
        child2[i][j] = new NodeContainer[fanout];
      }
    }
  }
  
  total = created + missed + trees + 1;
  cout<<"Created = "<<created<<endl;
  cout<<"Missed = "<<missed<<endl;
  cout<<"Final ratio = "<<double (created)/ (created+missed)<<endl;
  cout<<"Child probability factor = "<<probability<<endl;
  
  InternetStackHelper stack;
  
  stack.Install (center);
  
  for (i = 0; i < trees; i++)
  {
    stack.Install (tree[i]);
    for (j = 0; j < fanout; j++)
    {
      if (child1[i][j].GetN ())
      {
        stack.Install (child1[i][j]);
        for (k = 0; k < fanout; k++)
        {
          if (child2[i][j][k].GetN ())
            stack.Install (child2[i][j][k]);
        }
      }
    }
  }
  
  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("5Mbps")));
  
  if (wormTypeCheck == "udp")
    link.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  else
    link.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (1)));
  
  NetDeviceContainer *device = new NetDeviceContainer[trees];
  NetDeviceContainer **device1 = new NetDeviceContainer*[trees];
  NetDeviceContainer ***device2 = new NetDeviceContainer**[trees];
  
  for (i = 0; i < trees; i++)
  {
    device[i].Add (link.Install (center.Get (0), tree[i].Get (0)));
    device1[i] = new NetDeviceContainer[fanout];
    device2[i] = new NetDeviceContainer*[fanout];
    for (j = 0; j < fanout; j++)
    {
      if (child1[i][j].GetN ())
      {
        device2[i][j] = new NetDeviceContainer[fanout];
        device1[i][j].Add (link.Install (tree[i].Get (0), child1[i][j].Get (0)));
        for (k = 0; k < fanout; k++)
        {
          if (child2[i][j][k].GetN ())
          {
            device2[i][j][k].Add (link.Install (child1[i][j].Get (0), child2[i][j][k].Get (0)));
          }
        }
      }
      else
        device2[i][j] = new NetDeviceContainer[fanout];
    }
  }
  
  Ipv4InterfaceContainer *interface = new Ipv4InterfaceContainer[trees];
  Ipv4InterfaceContainer **interface1 = new Ipv4InterfaceContainer*[trees];
  Ipv4InterfaceContainer ***interface2 = new Ipv4InterfaceContainer**[trees];
  
  uint32_t ip = 167772160;
  
  Worm::SetFirstIP (ip);
  
  Ipv4AddressHelper address;
  
  for (i = 0; i < trees; i++)
  {
    address.SetBase (Ipv4Address (ip), "255.255.255.252");
    interface[i].Add (address.Assign (device[i]));
    ip += 4;
    interface1[i] = new Ipv4InterfaceContainer[fanout];
    interface2[i] = new Ipv4InterfaceContainer*[fanout];
    for (j = 0; j < fanout; j++)
    {
      if (child1[i][j].GetN ())
      {
        address.SetBase (Ipv4Address (ip), "255.255.255.252");
        interface1[i][j].Add (address.Assign (device1[i][j]));
        ip += 4;
        interface2[i][j] = new Ipv4InterfaceContainer[fanout];
        for (k = 0; k < fanout; k++)
        {
          if (child2[i][j][k].GetN ())
          {
            address.SetBase (Ipv4Address (ip), "255.255.255.252");
            interface2[i][j][k].Add (address.Assign (device2[i][j][k]));
            ip += 4;
          }
          else
            ip += 4;
        }
      }
      else
      {
        for (k = 0; k < fanout; k++)
          ip += 4;
        interface2[i][j] = new Ipv4InterfaceContainer[fanout];
      }
    }
  }
  
  Worm::SetLastIP (ip-1);
  
  vector<string> sender;
  
  stringstream ss;
  
  for (i = 0; i < trees; i++)
  {
    for (j = 0; j < fanout; j++)
    {
      for (k = 0; k < fanout; k++)
      {
        if (child2[i][j][k].GetN ())
        {
          ss<<i<<j<<k;
          sender.push_back (ss.str ());
          ss.str ("");
        }
      }
    }
  }
  
  vector<string> receiver = sender;
  
  uint32_t normalPort = 2000;
  uint32_t wormPort = 200;
  
  ApplicationContainer sourceApps;
  ApplicationContainer sinkApps;
  ApplicationContainer wormApps;
  
  bool vulnerable;
  
  Ptr<Worm> app = CreateObject<Worm> ();
  app->Setup (center.Get (0), interface[0].GetAddress (0), wormPort, true, true, "-1");
  center.Get (0)->AddApplication (app);
  wormApps.Add (center.Get (0)->GetApplication (0));
  
  for (i = 0; i < trees; i++)
  {
    ss<<i;
    Ptr<Worm> app1 = CreateObject<Worm> ();
    vulnerable = U->GetValue (0, 1.0) <= vulnerability;
    app1->Setup (tree[i].Get (0), interface[i].GetAddress (1), wormPort, false, vulnerable, ss.str());
    ss.str("");
    tree[i].Get (0)->AddApplication (app1);
    wormApps.Add (tree[i].Get (0)->GetApplication (0));
    for (j = 0; j < fanout; j++)
    {
      if (child1[i][j].GetN ())
      {
        ss<<i<<j;
        Ptr<Worm> app2 = CreateObject<Worm> ();
        vulnerable = U->GetValue (0, 1.0) <= vulnerability;
        app2->Setup (child1[i][j].Get (0), interface1[i][j].GetAddress (1), wormPort, false, vulnerable, ss.str());
        ss.str("");
        child1[i][j].Get (0)->AddApplication (app2);
        wormApps.Add (child1[i][j].Get (0)->GetApplication (0));
        for (k = 0; k < fanout; k++)
        {
          if (child2[i][j][k].GetN ())
          {
            ss<<i<<j<<k;
            Ptr<Worm> app3 = CreateObject<Worm> ();
            vulnerable = U->GetValue (0, 1.0) <= vulnerability;
            app3->Setup (child2[i][j][k].Get (0), interface2[i][j][k].GetAddress (1), wormPort, false, vulnerable, ss.str());
            ss.str("");
            child2[i][j][k].Get (0)->AddApplication (app3);
            wormApps.Add (child2[i][j][k].Get (0)->GetApplication (0));
          }
        }
      }
    }
  }
  
  while (sender.size ())
  {
    uint32_t tx = U->GetInteger (0, sender.size ()-1);
    uint32_t rx = U->GetInteger (0, receiver.size ()-1);
    if (sender[tx] == receiver[rx])
      continue;
      
    uint32_t tx0 = sender[tx][0] - 48;
    uint32_t tx1 = sender[tx][1] - 48;
    uint32_t tx2 = sender[tx][2] - 48;
    
    uint32_t rx0 = receiver[rx][0] - 48;
    uint32_t rx1 = receiver[rx][1] - 48;
    uint32_t rx2 = receiver[rx][2] - 48;
    
    OnOffHelper source ("ns3::UdpSocketFactory", InetSocketAddress (interface2[rx0][rx1][rx2].GetAddress (1), normalPort));
    ApplicationContainer temp1 = source.Install (NodeContainer (child2[tx0][tx1][tx2].Get (0)));
    temp1.Start (Seconds (U->GetValue (0, 0.1)));
    sourceApps.Add (temp1);
    
    PacketSinkHelper sink ("ns3::UdpSocketFactory", InetSocketAddress (interface2[rx0][rx1][rx2].GetAddress (1), normalPort));
    sinkApps.Add (sink.Install (child2[rx0][rx1][rx2].Get (0)));
        
    sender.erase (sender.begin () + tx);
    receiver.erase (receiver.begin () + rx);
  }
  
  sourceApps.Stop (Seconds (stopTime));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (stopTime));
  wormApps.Start (Seconds (0.0));
  wormApps.Stop (Seconds (stopTime));
  
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  
  Simulator::Stop (Seconds (stopTime));
  Simulator::Run ();
  Simulator::Destroy ();
  
  ofstream output ("p4-output.txt", ios::app);
  output<<" = [";
  
  double rcvd = 0;
  cout<<"---------------------------------------------------"<<endl;
  for (ApplicationContainer::Iterator it = sinkApps.Begin ();it != sinkApps.End ();it++)
  {
    Ptr<PacketSink> temp = DynamicCast<PacketSink> (*it);
    rcvd += temp->GetTotalRx ();
  }
  rcvd /= 1024;
  cout<<"Total Received Data = "<<rcvd<<" kB"<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"seed = "<<seed<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"trees = "<<trees<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"fanout = "<<fanout<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"utilization = "<<utilization<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"dataRate = "<<dataRate<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"wormTypeCheck = "<<wormTypeCheck<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"vulnerability = "<<vulnerability<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"packetSize = "<<packetSize<<endl;
  cout<<"---------------------------------------------------"<<endl;
  cout<<"connections = "<<connections<<endl;
  cout<<"---------------------------------------------------"<<endl;
  
  vector<string> listInfected = Worm::GetListInfected();
  vector<double> time = Worm::GetTime();
  
  for (i = 0; i < listInfected.size (); i++)
  {
    cout<<setw(10)<<listInfected[i]<<"\t"<<setw(10)<<time[i]<<"\t"<<setw(10)<<i+1<<endl;
    output<<time[i]<<" ";
  }
  cout<<"---------------------------------------------------"<<endl;
  
  output<<"];\n";
  output.close ();
  
  return 0;
}
