/*
 * move_together.h
 *
 *  Created on: Oct 3, 2016
 *      Author: vahid
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/flow-monitor-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("move_together");
using namespace ns3;
static struct DefaultValues
{
	DefaultValues()
	{
		phyMode = "DsssRate1Mbps";
		distance = 500;  // m
		numNodes = 25;  // by default, 5x5
		//  double interval = 0.001; // seconds
		packetSize = 600; // bytes
		numPackets = 10000000;
		rtslimit = "1500";

	}
	  std::string phyMode;
	  double distance;  // m
	  uint32_t numNodes;  // by default, 5x5
	//  double interval = 0.001; // seconds
	  uint32_t packetSize; // bytes
	  uint32_t numPackets;
	  std::string rtslimit;
} defaultValues;

static struct Nodes
{
	NodeContainer all;
	NodeContainer& CreateAll(uint32_t numNodes = defaultValues.numNodes)
	{
		all.Create(numNodes);
		return all;
	}
} nodeContainers;

void DoDefaultConfig()
{
	  // turn off RTS/CTS for frames below 2200 bytes
	  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue (defaultValues.rtslimit));
	  // Fix non-unicast data rate to be the same as that of unicast
	  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (defaultValues.phyMode));
}

NetDeviceContainer SetWifi(NodeContainer &nodes = nodeContainers.all)
{
	WifiHelper wifi;
	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
	wifiPhy.Set ("RxGain" , DoubleValue(-10));
	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
	wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
	wifiPhy.SetChannel (wifiChannel.Create ());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
	wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
								"DataMode",StringValue (defaultValues.phyMode),
								"ControlMode",StringValue (defaultValues.phyMode));
	// Set it to adhoc mode
	wifiMac.SetType ("ns3::AdhocWifiMac");
	NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);
	return devices;
}

void SetMobility(NodeContainer &nodes = nodeContainers.all)
{
	MobilityHelper mobility;

	mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
		  "X", StringValue ("0.0"),
		  "Y", StringValue ("200.0"),
		  "Rho", StringValue ("Uniform:100:150"));

	mobility.SetMobilityModel ("ns3::RandomDirection2dMobilityModel",
	                             "Bounds", RectangleValue (Rectangle (0, 200, 30, 60)),
	                             "Speed", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"),
	                             "Pause", StringValue ("ns3::ConstantRandomVariable[Constant=0.2]"));
	  //  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	  mobility.Install (nodes.Get(0));


	  mobility.PushReferenceMobilityModel (nodes.Get (0));
	  Ptr<MobilityModel> parentMobility = nodes.Get (0)->GetObject<MobilityModel> ();
	  Vector pos =  parentMobility->GetPosition ();
	  Ptr<ListPositionAllocator> positionAllocMnn =
	    CreateObject<ListPositionAllocator> ();
	  for (uint32_t i = 1; i < nodes.GetN(); i++)
	  {
		  pos.x = 5 + i*2;
		  pos.y = 20 + i*2;
		  positionAllocMnn->Add (pos);
	  }
	  mobility.SetPositionAllocator (positionAllocMnn);
	  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	  for (uint32_t i = 1; i < nodes.GetN(); i++)
	  {
		  mobility.Install (nodes.Get(i));
	  }
}

int main (int argc, char *argv[])
{
	return 1;
}

