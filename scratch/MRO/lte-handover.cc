/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Adapted from lena-x2-handover-measures.cc example from Manuel Requena
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream> // std::stringstream
#include <utility> // std::pair
#include <vector>
#include <map>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-module.h"
#include "ns3/spectrum-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/config-store-module.h"
#include <../json.hpp>
#include <filesystem>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <math.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LteHandover");

// These variables are declared outside of main() so that they may be
// referenced in the callbacks below.  They are prefixed with "g_" to
// indicate global variable status.

std::ofstream g_ueMeasurements;
std::ofstream g_packetSinkRx;
std::ofstream g_positionTrace;

// Report on execution progress
void
ReportProgress (Time reportingInterval)
{
  std::cout << "*** Simulation time: " << std::fixed << std::setprecision (1) << Simulator::Now ().GetSeconds () << "s" << std::endl;
  Simulator::Schedule (reportingInterval, &ReportProgress, reportingInterval);
}

// Parse context strings of the form "/NodeList/3/DeviceList/1/Mac/Assoc"
// to extract the NodeId
uint32_t
ContextToNodeId (std::string context)
{
  std::string sub = context.substr (10);  // skip "/NodeList/"
  uint32_t pos = sub.find ("/Device");
  return atoi (sub.substr (0,pos).c_str ());
}

void
NotifyUdpReceived (std::string context, Ptr<const Packet> p, const Address &addr)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () << " node "
            << ContextToNodeId (context)
            << " UDP received" << std::endl;
}

void
NotifyConnectionEstablishedUe (std::string context,
                               uint64_t imsi,
                               uint16_t cellid,
                               uint16_t rnti)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () << " node "
            << ContextToNodeId (context)
            << " UE IMSI " << imsi
            << ": connected to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti,
                       uint16_t targetCellId)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () << " node "
            << ContextToNodeId (context)
            << " UE IMSI " << imsi
            << ": previously connected to CellId " << cellid
            << " with RNTI " << rnti
            << ", doing handover to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkUe (std::string context,
                       uint64_t imsi,
                       uint16_t cellid,
                       uint16_t rnti)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () << " node "
            << ContextToNodeId (context)
            << " UE IMSI " << imsi
            << ": successful handover to CellId " << cellid
            << " with RNTI " << rnti
            << std::endl;
}

void
NotifyConnectionEstablishedEnb (std::string context,
                                uint64_t imsi,
                                uint16_t cellid,
                                uint16_t rnti)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () << " node "
            << ContextToNodeId (context)
            << " eNB CellId " << cellid
            << ": successful connection of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyHandoverStartEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti,
                        uint16_t targetCellId)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () << " node "
            << ContextToNodeId (context)
            << " eNB CellId " << cellid
            << ": start handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << " to CellId " << targetCellId
            << std::endl;
}

void
NotifyHandoverEndOkEnb (std::string context,
                        uint64_t imsi,
                        uint16_t cellid,
                        uint16_t rnti)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () << " node "
            << ContextToNodeId (context)
            << " eNB CellId " << cellid
            << ": completed handover of UE with IMSI " << imsi
            << " RNTI " << rnti
            << std::endl;
}

void
NotifyRecvMeasurementReport (std::string context,
                             uint64_t imsi,
                             uint16_t cellId,
                             uint16_t rnti,
                             LteRrcSap::MeasurementReport report)
{
  std::cout << std::setprecision (3) << Simulator::Now ().GetSeconds () 
            << " UE " << imsi
            << ": sent measReport to CellId " << cellId
            << std::endl;
}


void
TracePosition (NodeContainer ueNodes, Time interval)
{
  for (uint32_t i = 0; i < ueNodes.GetN (); ++i) {
 
    // Get node and then position for each UE.
    Vector v = ueNodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ();
    g_positionTrace  << std::setw (1) << std::setprecision (3) << std::fixed << Simulator::Now ().GetSeconds () << "," << (i+1) << "," << v.x << "," << v.y << std::endl;
  }

  Simulator::Schedule (interval, &TracePosition, ueNodes, interval);
}

void
NotifyUeMeasurements (std::string context, uint16_t imsi, uint16_t cellId, double rsrpDbm, double rsrqDbm, bool servingCell, uint8_t ccId)
{
  g_ueMeasurements << std::setw (1) << std::setprecision (3) << std::fixed << Simulator::Now ().GetSeconds () << "," 
    << std::setw (1) << imsi << ","
    << std::setw (1) << cellId << "," 
    << std::setw (1) << (servingCell ? "1" : "0") << "," 
    << std::setw (1) << rsrpDbm << "," 
    << std::setw (1) << rsrqDbm << std::endl;
}

void
NotifyPacketSinkRx (std::string context, Ptr<const Packet> packet, const Address &address, const Address &reciever)
{
  g_packetSinkRx << std::setw (1) << std::setprecision (3) << std::fixed << Simulator::Now ().GetSeconds () 
    << "," << std::setw (1) << packet->GetSize () << std::setw (1) << "," << reciever << std::endl;
}


/**
 * Program for an automatic X2-based handover based on the RSRQ measures.
 * It instantiates two eNodeB, attaches one UE to the 'source' eNB.
 * The UE moves between both eNBs, it reports measures to the serving eNB and
 * the 'source' (serving) eNB triggers the handover of the UE towards
 * the 'target' eNB when it considers it is a better eNB.
 */
int
main (int argc, char *argv[])
{
  
  // Constants that can be changed by command-line arguments
  //handover values
  std::string handoverType = "A3Rsrp"; //valid values are: "A3Rsrp", "A2A4RsrqHandoverAlgorithm"
  double hystVal = 3; // standards limited to: 0-15 dB (rounded to nearest .5 dB), enforced in code (values outside of range will error, values will be rounded)
  double timeToTrigger = 256; // standards limited to: 0, 40, 64, 80, 100, 128, 160, 256, 320, 480, 512, 640, 1024, 1280 ms
  double a3Offset = 0; // standards limited to: -15 dB to 15 dB
  //scenario description files
  std::string scenarioName = "0.8";
  int trialNum = 0;
  double numSectors = 3;
  std::string resultDir = "/home/collin/workspace/ns-3-dev-git/results/Scenario " + scenarioName + "/trial " + std::to_string(trialNum) + "/";
  std::string rfConfigFileName = "/home/collin/Dropbox/FBC_Maveric Academic Collaboration/NS-3_related_files/Simulation_Scenarios/Scenario " + scenarioName + "/trial " + std::to_string(trialNum) + "/rf_config.json";
  std::string protocolConfigFileName = "/home/collin/Dropbox/FBC_Maveric Academic Collaboration/NS-3_related_files/Simulation_Scenarios/Scenario " + scenarioName + "/trial " + std::to_string(trialNum) + "/protocol_config.json";
  std::string traceDir = "/home/collin/Dropbox/FBC_Maveric Academic Collaboration/NS-3_related_files/Simulation_Scenarios/Scenario 0.8/trial 0/";
  //Other Values
  double enbTxPowerDbm = 46.0;
  bool mroExp = true;
  int rngSeedNum = 1;

  
  
  // Command line arguments
  CommandLine cmd;
  cmd.AddValue ("resultDir","Local filepath to the top level results",resultDir);
  cmd.AddValue ("rfConfigFileName","Local filepath to the rf config file",rfConfigFileName);
  cmd.AddValue ("protocolConfigFileName","Local filepath to the protocol config file",protocolConfigFileName);
  cmd.AddValue ("traceDir","Local filepath to the Quadriga PHY traces",traceDir);
  cmd.AddValue ("mroExp","flag for MRO experiments, false turns them off. Will be removed in later versions",mroExp);
  cmd.AddValue ("rngSeedNum", "Number used for seeding the RNG seed.", rngSeedNum);
  cmd.Parse (argc, argv);
  



  std::ifstream  rf_config_file(rfConfigFileName);
  nlohmann::json rfSimParameters;// = nlohmann::json::parse(rf_config_file);
  rf_config_file >> rfSimParameters;
  std::cout << "rf config loaded" << std::endl;
  std::ifstream  protocol_config_file(protocolConfigFileName);
  nlohmann::json protocolSimParameters;// = nlohmann::json::parse(protocol_config_file);
  protocol_config_file >> protocolSimParameters;
  std::cout << "protocol config loaded" << std::endl;
  // Constants for this simulation
  uint16_t numberOfUes = uint16_t(rfSimParameters["UE"].size());
  uint16_t numberOfEnbs = uint16_t(numSectors*rfSimParameters["BS"].size());//Each eNb has three sectors which are treated as separate eNb by NS-3
  // eNb/UE have to be made first to ensure that eNbID = (0,...,numeNb-1) and UEID = (numeNb,...,numeNb+numUe-1)
  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create (numberOfEnbs);
  ueNodes.Create (numberOfUes);
  RngSeedManager::SetRun (int(rngSeedNum));

  
  // Additional constants (not changeable at command line) + "/Scenario" + scenarioName + "/Scenario" + scenarioName + "-" + std::to_string(trialNum)
  Time positionTracingInterval = MilliSeconds (1);
  Time reportingInterval = Seconds (10);
  uint64_t ftpSize = 8*pow(10,12); // 2 TB
  uint16_t port = 4000;  // port number
  double simTime = rfSimParameters["simulation"]["simulation_duration_s"]; // seconds
  
  // change some default attributes based upon command line settings
  Config::SetDefault ("ns3::LteHelper::UseIdealRrc", BooleanValue (true));
  Config::SetDefault ("ns3::LteHelper::UsePdschForCqiGeneration", BooleanValue (false));
  Config::SetDefault ("ns3::LteUePhy::Qout", DoubleValue(protocolSimParameters["NS3"]["qout_db"]));
  Config::SetDefault ("ns3::LteUePhy::Qin", DoubleValue(protocolSimParameters["NS3"]["qin_db"]));
  Config::SetDefault ("ns3::LteUePhy::NumQoutEvalSf", UintegerValue(protocolSimParameters["NS3"]["number_qout_eval_sf"]));
  Config::SetDefault ("ns3::LteUePhy::NumQinEvalSf", UintegerValue(protocolSimParameters["NS3"]["number_qin_eval_sf"]));
  Config::SetDefault ("ns3::LteUeRrc::T310", TimeValue( MilliSeconds(protocolSimParameters["NS3"]["t310"])));
  Config::SetDefault ("ns3::LteUeRrc::N310", UintegerValue(protocolSimParameters["NS3"]["n310"]));
  Config::SetDefault ("ns3::LteUeRrc::N311", UintegerValue(protocolSimParameters["NS3"]["n311"]));
  Config::SetDefault ("ns3::LteUeRrc::mroExp", BooleanValue(mroExp));
  Config::SetDefault ("ns3::UdpClient::Interval",TimeValue (Seconds (0.001)));
  Config::SetDefault ("ns3::UdpClient::MaxPackets",UintegerValue (simTime*1000+100));
    
  if (protocolSimParameters["NS3"]["use_rlc_um"] == 0)
    {
      Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue (LteEnbRrc::RLC_AM_ALWAYS));
    }
  
  if (!(std::filesystem::exists(resultDir)))
  {
    std::filesystem::create_directories(resultDir);
  }

  std::filesystem::copy_file(rfConfigFileName,(resultDir + "rf_config.json"),std::filesystem::copy_options::overwrite_existing);
  std::filesystem::copy_file(protocolConfigFileName,(resultDir + "protocol_config.json"),std::filesystem::copy_options::overwrite_existing);
  
  g_ueMeasurements.open ((resultDir + "ue-measurements.csv").c_str(), std::ofstream::out);
  g_ueMeasurements << "time,imsi,cellId,isServingCell?,RSRP(dBm),RSRQ(dB)" << std::endl;
  g_packetSinkRx.open ((resultDir + "packet-receive.csv").c_str(), std::ofstream::out);
  g_packetSinkRx << "time,bytesRx,mac_address" << std::endl;
  g_positionTrace.open ((resultDir + "position.csv").c_str(), std::ofstream::out);
  g_positionTrace << "time,imsi,x,y" << std::endl;
  
  
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  epcHelper->SetAttribute ("X2LinkDelay", TimeValue(MilliSeconds(protocolSimParameters["NS3"]["x2_delay_ms"])));
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");
  
  std::vector<double> hystValue (numberOfEnbs);
  for (unsigned i=0; i<hystValue.size(); ++i) hystValue[i] = hystVal;
   
  std::vector<double> TTT (numberOfEnbs);
  for (unsigned i=0; i<TTT.size(); ++i) TTT[i] = timeToTrigger;
   
  std::vector<double> A3 (numberOfEnbs);
  for (unsigned i=0; i<A3.size(); ++i) A3[i] = a3Offset;

  
  lteHelper->SetHandoverAlgorithmType ("ns3::A3RsrpHandoverAlgorithm");
  lteHelper->SetHandoverAlgorithmAttribute ("perCellParameterPath",StringValue (protocolConfigFileName));//path to json containing per-cell parameters
  lteHelper->SetHandoverAlgorithmAttribute ("Hysteresis",DoubleValue (hystVal));//default value to use if there are no per-cell values
  lteHelper->SetHandoverAlgorithmAttribute ("TimeToTrigger",TimeValue (MilliSeconds (timeToTrigger)));//default value to use if there are no per-cell values
  lteHelper->SetHandoverAlgorithmAttribute ("a3Offset",DoubleValue (a3Offset));//default value to use if there are no per-cell values

  
  // Install Mobility Model in eNB
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfEnbs/numSectors; i++)
  {
    for (int j = 0; j < numSectors; ++j) //each of the three sectors shares a location
    {
      Vector enbPosition (rfSimParameters["BS"][i]["location"][0], rfSimParameters["BS"][i]["location"][1], rfSimParameters["BS"][i]["location"][2]);
      enbPositionAlloc->Add (enbPosition);
    }
  }
    
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbPositionAlloc);
  enbMobility.Install (enbNodes);
  
  // Install Mobility Model in UE
  MobilityHelper ueMobility;
  if (rfSimParameters["simulation"]["mobility_type"] == "constant_velocity")
  {
    ueMobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install (ueNodes);
    
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
      Vector uePosition (rfSimParameters["UE"][i]["initial_position"][0], rfSimParameters["UE"][i]["initial_position"][1], rfSimParameters["UE"][i]["initial_position"][2]);
      ueNodes.Get (i)->GetObject<MobilityModel> ()->SetPosition (uePosition);
      Vector ueVelocity (rfSimParameters["UE"][i]["velocity"][0], rfSimParameters["UE"][i]["velocity"][1], rfSimParameters["UE"][i]["velocity"][2]);
      ueNodes.Get (i)->GetObject<ConstantVelocityMobilityModel> ()->SetVelocity (ueVelocity);
    }
  } else if (rfSimParameters["simulation"]["mobility_type"] == "random_turns") 
  {
    ueMobility.SetMobilityModel ("ns3::WaypointMobilityModel");
    ueMobility.Install (ueNodes);

    for (uint16_t i = 0; i < numberOfUes; i++) 
    {
      for (uint16_t j = 0; j < rfSimParameters["UE"][i]["turn_times"].size(); j++)
      {
        std::string turn = "t" + std::to_string(j);
        Waypoint ueWaypoint (Seconds(rfSimParameters["UE"][i]["turn_times"][j]), Vector (rfSimParameters["UE"][i]["turn_positions"][turn][0],rfSimParameters["UE"][i]["turn_positions"][turn][1],rfSimParameters["UE"][i]["turn_positions"][turn][2]));
        ueNodes.Get (i)->GetObject<WaypointMobilityModel> ()-> AddWaypoint (ueWaypoint);
      }
      
    }
  }

  // Install LTE Devices in eNB and UEs
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (enbTxPowerDbm));
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  
  // LTE Helper will, by default, install Friis loss model on UL and DL
  // Set table-based pathloss model on the downlink only
  // These steps must be done after InstallEnbDevice or InstallUeDevice above
  Ptr<TableLossModel> tableLossModel = CreateObject<TableLossModel> ();
  Ptr<SpectrumChannel> dlChannel = lteHelper->GetDownlinkSpectrumChannel ();
  Ptr<SpectrumChannel> ulChannel = lteHelper->GetUplinkSpectrumChannel ();
  // Configure tableLossModel here, by e.g. pointing it to a trace file
  tableLossModel->initializeTraceVals(numberOfEnbs, numberOfUes, rfSimParameters["simulation"]["resource_blocks"], simTime*1000);
  
  for (int i = 0; i < numberOfUes; ++i)
  {
  	for (int j = 0; j < numberOfEnbs/numSectors; ++j)
  	{
	   for (int k = 0; k < numSectors; ++k)
  	  {
  		  tableLossModel->LoadTrace (traceDir,"ULDL_TX_" + std::to_string(j+1) + "_Sector_" + std::to_string(k+1) + "_UE_" + std::to_string(i+1) + "_Channel_Response.csv");// the filepath (first input), must be changed to your local filepath for these trace files
      }
   	}
  }
  
  dlChannel->AddSpectrumPropagationLossModel (tableLossModel);
  ulChannel->AddSpectrumPropagationLossModel (tableLossModel);// we want the UL/DL channels to be reciprocal
  

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get(0);
  InternetStackHelper internet;
  internet.Install (remoteHost);
  
  // Create the notional Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (epcHelper->GetPgwNode (), remoteHost);
  //PointToPointStarHelper star (numberOfUes, p2ph);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  
  // Routing of the Internet Host (towards the LTE network)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  // interface 0 is localhost, 1 is the p2p device
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIfaces;
  ueIpIfaces = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  
  // Update the static routing table with the addresses of the UE
  Ptr<Ipv4StaticRouting> ueStaticRouting;// = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (0)->GetObject<Ipv4> ());
  
  for (uint32_t i = 0; i < numberOfUes; ++i)
  {
    ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNodes.Get (i)->GetObject<Ipv4> ());
    ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  }
  // Attach all UEs to the first eNodeB
  for (uint16_t i = 0; i < numberOfUes; i++)
    {
      lteHelper->Attach (ueLteDevs.Get (i), enbLteDevs.Get (int(rfSimParameters["UE"][i]["initial_attachment"]) - 1));
    }

  // Create the sender applications, 1 per UE, all originating from the remote node
  std::string transport_protocol = protocolSimParameters["NS3"]["transport_protocol"];
  if (transport_protocol == "TCP")
  {
    BulkSendHelper ftpServer ("ns3::TcpSocketFactory", Address ());
    ftpServer.SetAttribute ("MaxBytes", UintegerValue (ftpSize));

    ApplicationContainer sourceApps;
    
    for (uint32_t i = 0; i < numberOfUes; ++i)
    {
      AddressValue remoteAddress (InetSocketAddress (ueIpIfaces.GetAddress (i), port));
      ftpServer.SetAttribute ("Remote", remoteAddress);
      sourceApps.Add (ftpServer.Install(remoteHost));
    }
    
    sourceApps.Start (Seconds (1));
    sourceApps.Stop (Seconds (simTime));
  } 
  else if (transport_protocol == "UDP") 
  {
    
    //ftpServer.SetAttribute ("MaxBytes", UintegerValue (ftpSize));

    ApplicationContainer sourceApps;
    
    for (uint32_t i = 0; i < numberOfUes; ++i)
    {
      AddressValue remoteAddress (InetSocketAddress (ueIpIfaces.GetAddress (i), port));
      UdpClientHelper ftpServer (ueIpIfaces.GetAddress (i),port);
      //ftpServer.SetAttribute ("Remote", remoteAddress);
      sourceApps.Add (ftpServer.Install(remoteHost));
    }
    
    sourceApps.Start (Seconds (1));
    sourceApps.Stop (Seconds (simTime));
  }
  
  // Create the packet sinks, 1 per UE, each at 1 UE
  if (transport_protocol == "TCP")
  {
    PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory", Address ());

    ApplicationContainer sinkApps;
    
    for (uint32_t i = 0; i < numberOfUes; ++i)
    {
      AddressValue sinkLocalAddress (InetSocketAddress (ueIpIfaces.GetAddress (i), port));
      sinkHelper.SetAttribute ("Local", sinkLocalAddress);
      sinkApps.Add (sinkHelper.Install(ueNodes.Get(i)));
    }
    
    sinkApps.Start (Seconds (1));
    sinkApps.Stop (Seconds (simTime));
  } 
  else if (transport_protocol == "UDP") 
  {
    PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", Address ());

    ApplicationContainer sinkApps;
    
    for (uint32_t i = 0; i < numberOfUes; ++i)
    {
      AddressValue sinkLocalAddress (InetSocketAddress (ueIpIfaces.GetAddress (i), port));
      sinkHelper.SetAttribute ("Local", sinkLocalAddress);
      sinkApps.Add (sinkHelper.Install(ueNodes.Get(i)));
    }
    
    sinkApps.Start (Seconds (1));
    sinkApps.Stop (Seconds (simTime));
  }
  
  
  Ptr<EpcTft> tft = Create<EpcTft> ();
  EpcTft::PacketFilter dlpf;
  dlpf.localPortStart = port;
  dlpf.localPortEnd = port;
  tft->Add (dlpf);
  EpsBearer bearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT);
  for (uint32_t i = 0; i < numberOfUes; ++i)
  {
    lteHelper->ActivateDedicatedEpsBearer (ueLteDevs.Get (i), bearer, tft);
  }
  
  // Add X2 interface
  lteHelper->AddX2Interface (enbNodes);
  
  
  lteHelper->EnableLogComponents();
  
  // connect custom trace sinks for RRC connection establishment and handover notification
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/ConnectionEstablished",
                   MakeCallback (&NotifyConnectionEstablishedUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverStart",
                   MakeCallback (&NotifyHandoverStartUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkEnb));
  Config::Connect ("/NodeList/*/DeviceList/*/LteUeRrc/HandoverEndOk",
                   MakeCallback (&NotifyHandoverEndOkUe));
  Config::Connect ("/NodeList/*/DeviceList/*/LteEnbRrc/RecvMeasurementReport",
                   MakeCallback (&NotifyRecvMeasurementReport));
  // connect additional traces for more experiment tracing
  Config::Connect ("/NodeList/*/DeviceList/*/ComponentCarrierMapUe/*/LteUePhy/ReportUeMeasurements",
                   MakeCallback (&NotifyUeMeasurements));
  Config::Connect ("/NodeList/*/ApplicationList/*/$ns3::PacketSink/RxWithAddresses",
                   MakeCallback (&NotifyPacketSinkRx));
  
  // Initiate position tracing
  Simulator::Schedule (Seconds (0), &TracePosition, ueNodes, positionTracingInterval);
  
  std::cout << "Simulation time: " << simTime << " sec" << std::endl;
  
  Simulator::Schedule (reportingInterval, &ReportProgress, reportingInterval);
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  
  // This method coordinates the deallocation of heap memory so that no
  // memory leaks are reported.
  Simulator::Destroy ();
  
  // Close any open file descriptors
  g_ueMeasurements.close ();
  g_packetSinkRx.close ();
  g_positionTrace.close ();
  
  return 0;
}
