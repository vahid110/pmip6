#ifndef OLSR_PATH_FINDER_H
#define OLSR_PATH_FINDER_H

#include "ns3/ipv4-address.h"
#include "ns3/olsr-routing-protocol.h"

#include <list>
using namespace ns3;

template<typename T>
struct MyUniqueList : public std::list<T>
{
	bool is_available(const T& val)
	{
		typename std::list<T>::iterator it = this->begin();
		for(; it != this->end(); it++)
		{
			if (val == *it)
				return true;
		}
		return false;
	}
};

class OlsrPathFinder
{

	typedef MyUniqueList<Ipv4Address> result_t;
public:
	static void GetPath(
			Ptr<Node> src_node,
			const Ipv4Address &dest_address,
			MyUniqueList<Ipv4Address> &res)
	{
		Ptr<olsr::RoutingProtocol> src_proto = GetOLSR(src_node);
		if (!src_proto)
		{
			NS_LOG_UNCOND("GetOLSR Failed");
			return;
		}

		OlsrState& src_proto_state = src_proto->GetState();
		const olsr::TopologySet &src_topo_set = src_proto_state.GetTopologySet();

		olsr::RoutingTableEntry outEntry;

		if (src_proto->Lookup(dest_address, outEntry) == true)
		{
			GetPath(src_topo_set, outEntry.nextAddr, dest_address, res);
			res.push_front(GetIpv4Address(src_node));
		}
		else
		{
			//error message
		}
	}

	static int protect;
	static void GetPath(
			const olsr::TopologySet &src_topo_set,
			const Ipv4Address &src_next_hop,
			const Ipv4Address &dest_address,
			MyUniqueList<Ipv4Address> &res)
	{
		if(++protect == 100)
		{
			NS_LOG_UNCOND("Force stopping recursion");
			res.clear();
			protect = 0;
			return;
		}

//		NS_LOG_UNCOND(Simulator::Now().GetSeconds() <<" GetPath[" <<
//				src_next_hop << " --> " <<
//				dest_address << "] "  << protect);

		res.push_front(dest_address);

		if (dest_address == src_next_hop)
			return;

		uint16_t sequenceNumber = 0;
		Ipv4Address lastAddr;
		bool found = false;

		for ( olsr::TopologySet::const_iterator it = src_topo_set.begin();
			  it != src_topo_set.end();
			  ++it )
		{
			if (it->destAddr == dest_address)
			{
//				NS_LOG_UNCOND("" << it->destAddr << " " << it->lastAddr << " "
//				<< it->sequenceNumber);
				if (it->sequenceNumber >= sequenceNumber)//find the latest
				{
					if (! res.is_available(it->lastAddr))
					{
						found = true;
						sequenceNumber = it->sequenceNumber;
						lastAddr = it->lastAddr;
					}
				}
			}
		}

		if (found)
			GetPath(src_topo_set, src_next_hop, lastAddr, res);
		else
		{
			protect = 0;
		}
	}

	static void PrintPath(
			Ptr<Node> src_node,
			const Ipv4Address &dest_address)
	{
		MyUniqueList<Ipv4Address> res;
		GetPath(src_node, dest_address, res);

		NS_LOG_UNCOND("PATH[" << GetIpv4Address(src_node) << " --> " <<
				dest_address << "] =>");
		for (MyUniqueList<Ipv4Address>::iterator it = res.begin();
				it != res.end(); it++)
		{
			NS_LOG_UNCOND(*it);
		}
	}

//private:
	static Ptr<olsr::RoutingProtocol> GetOLSR(Ptr<Node> node)
	{
		Ptr<olsr::RoutingProtocol> res;

		Ptr<Ipv4> stack = node->GetObject<Ipv4> ();
		if (!stack)
			return res;
		Ptr<Ipv4RoutingProtocol> rp = (stack->GetRoutingProtocol ());

	  Ptr<Ipv4ListRouting> lrp = DynamicCast<Ipv4ListRouting> (rp);

	  for (uint32_t i = 0; i < lrp->GetNRoutingProtocols ();  i++)
		{
		  int16_t priority;
		  Ptr<Ipv4RoutingProtocol> temp = lrp->GetRoutingProtocol (i, priority);
		  if (DynamicCast<olsr::RoutingProtocol> (temp))
			{
			  res = DynamicCast<olsr::RoutingProtocol> (temp);
			}
		}

		return res;
	}

	static Ipv4Address GetIpv4Address(Ptr<Node> node, uint32_t interface = 1)
	{
		Ipv4Address ipAddr;
		Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();

		Ipv4InterfaceAddress iaddr = ipv4->GetAddress (interface, 0);
		ipAddr = iaddr.GetLocal ();
		return ipAddr;
	}
};
int OlsrPathFinder::protect = 0;

#endif //OLSR_PATH_FINDER_H
