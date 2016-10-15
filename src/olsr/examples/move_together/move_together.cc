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
#include "ns3/pmip6-module.h"
#include "ns3/csma-module.h"

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

class MyNode :public Node
{

};

template<typename T=MyNode>
class MyNodeContainer : public NodeContainer
{
public:
	void Create (uint32_t n)
	{
	  for (uint32_t i = 0; i < n; i++)
		{
		    Add (CreateObject<T> ());
		}
	}
};

template<typename T=MyNodeContainer<> >
struct Nodes
{
	T all;
	T& CreateAll(uint32_t numNodes = defaultValues.numNodes)
	{
		all.Create(numNodes);
		return all;
	}
};

static Nodes<> nodeContainers;

void DoDefaultConfig()
{
	  // turn off RTS/CTS for frames below 2200 bytes
	  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue (defaultValues.rtslimit));
	  // Fix non-unicast data rate to be the same as that of unicast
	  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (defaultValues.phyMode));
}



Ipv6InterfaceContainer AssignIpv6Address(Ptr<NetDevice> device, Ipv6Address addr, Ipv6Prefix prefix)
{
  Ipv6InterfaceContainer retval;

  Ptr<Node> node = device->GetNode ();
  NS_ASSERT_MSG (node, "Ipv6AddressHelper::Allocate (): Bad node");

  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
  NS_ASSERT_MSG (ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");
  int32_t ifIndex = 0;

  ifIndex = ipv6->GetInterfaceForDevice (device);
  if (ifIndex == -1)
    {
      ifIndex = ipv6->AddInterface (device);
    }
  NS_ASSERT_MSG (ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                 "Interface index not found");

  Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (addr, prefix);
  ipv6->SetMetric (ifIndex, 1);
  ipv6->SetUp (ifIndex);
  ipv6->AddAddress (ifIndex, ipv6Addr);

  retval.Add (ipv6, ifIndex);

  return retval;
}

Ipv6InterfaceContainer AssignWithoutAddress(Ptr<NetDevice> device)
{
  Ipv6InterfaceContainer retval;

  Ptr<Node> node = device->GetNode ();
  NS_ASSERT_MSG (node, "Ipv6AddressHelper::Allocate (): Bad node");

  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
  NS_ASSERT_MSG (ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");
  int32_t ifIndex = 0;

  ifIndex = ipv6->GetInterfaceForDevice (device);
  if (ifIndex == -1)
    {
      ifIndex = ipv6->AddInterface (device);
    }
  NS_ASSERT_MSG (ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                 "Interface index not found");

  ipv6->SetMetric (ifIndex, 1);
  ipv6->SetUp (ifIndex);

  retval.Add (ipv6, ifIndex);

  return retval;
}

NetDeviceContainer SetCsma(NodeContainer &nodes = nodeContainers.all)
{
	CsmaHelper csma;
	//All Link is 50Mbps and 0.1ms delay
	csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
	csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
	csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
	return csma.Install(nodes);
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

Pmip6ProfileHelper *
CreateProfiler(
		const MyNodeContainer<> &nc,
		const Node &lma)
{
	Pmip6ProfileHelper *profile = new Pmip6ProfileHelper();
	for( MyNodeContainer<>::Iterator it = nc.Begin(); it != nc.End(); it++)
	{
		Ptr<Node> node = *it;
		Ptr<NetDevice> device = node->GetDevice(0);
		Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
		int32_t ifIndex = ipv6->GetInterfaceForDevice (device);
		char str[16];
		memset(str,0,16);
		snprintf(str, sizeof str, "node-%lu", (unsigned long)node->GetId());

		profile->AddProfile(
				str,
				Identifier(Mac48Address::ConvertFrom(device->GetAddress())),
				lma.GetDevice()->GetAddress(0, 1),
				std::list<Ipv6Address>());
	}

	return profile;
}

void SetLma(Node &lma, Pmip6ProfileHelper *profile)
{
	  Pmip6LmaHelper lmahelper;
	  lmahelper.SetPrefixPoolBase(Ipv6Address("3ffe:1:4::"), 48);
	  lmahelper.SetProfileHelper(profile);

	  lmahelper.Install(lma);
}

void SetMag(NodeContainer &mags,NodeContainer &aps, Pmip6ProfileHelper *profile)
{
	  Pmip6MagHelper maghelper;
	  maghelper.SetProfileHelper(profile);
	  maghelper.Install (mags.Get(0), mag1Ifs.GetAddress(0, 0), aps.Get(0));
	  maghelper.Install (mags.Get(1), mag2Ifs.GetAddress(0, 0), aps.Get(1));
}



int main (int argc, char *argv[])
{
	return 1;
}

