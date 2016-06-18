#ifndef IPV6_STATIC_ROUTER
#define IPV6_STATIC_ROUTER

#include "olsr_path_finder.h"

using namespace ns3;

class Ipv6StaticRouter
{
	typedef std::map<Ptr<Node> , uint32_t> container_t;

	struct NodeIndex
	{
		NodeIndex(NodeContainer c)
		{
			NodeContainer::Iterator it;
			for (it = c.Begin(); it != c.End(); it++)
			{
				m_container[*it] = (*it)->GetId();
			}
		}

		uint32_t GetIndex(Ptr<Node> node)
		{
			container_t::iterator it = m_container.find(node);
			assert(it != m_container.end());
			return it->second;
		}
	private:
		container_t m_container;
	};

public:
	Ipv6StaticRouter(
			NodeContainer &ipv6NodeContainer,
			Ipv6InterfaceContainer &ifv6cont,
			NodeContainer &ipv4NodeContainer,
			Ipv4InterfaceContainer &ifv4cont)
	: m_ipv4Nodes(&ipv4NodeContainer)
	, m_ipv6Nodes(&ipv6NodeContainer)
	, m_ifv6cont(&ifv6cont)
	, m_ifv4cont(&ifv4cont)
	, m_ipv6_index(ipv6NodeContainer)
	{}


	Ipv6StaticRouter(const Ipv6StaticRouter &o)
	: m_ipv4Nodes(o.ipv4NodeContainer)
	, m_ipv6Nodes(o.ipv6NodeContainer)
	, m_ifv6cont(o.ifv6cont)
	, m_ifv4cont(o.ifv4cont)
	, m_ipv6_index(*o.ipv6NodeContainer)
	{}
public:
	void SetRoutes(Ptr<Socket> src_socket_ipv6, Address dest_ipv6)
	{
		Ptr<Node> src_node_ipv6 = src_socket_ipv6->GetNode();

		const Ipv4Address dest_address_ipv6 = InetSocketAddress::ConvertFrom(m_peer).GetIpv4();

		NS_LOG_DEBUG("SendPacket[" << OlsrPathFinder::GetIpv4Address(src_node) <<
				" --> " << dest_address << "]");

		OlsrPathFinder::PrintPath(src_node, dest_address)	;

	}
private:
	NodeContainer *m_ipv4Nodes, *m_ipv6Nodes;
	Ipv4InterfaceContainer *m_ifv4cont;
	Ipv6InterfaceContainer *m_ifv6cont;
	NodeIndex m_ipv6_index;
};
#endif // IPV6_STATIC_ROUTER
