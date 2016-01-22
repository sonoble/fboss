#include "NetlinkListener.h"

NetlinkListener::NetlinkListener(): sock(0), link_cache(0), route_cache(0), manager(0), netlink_listener_thread(NULL)
{
	printf("Constructor of NetlinkListener\r\n");
	init();
	init_ifaces("wedgetap", 3);
}

void NetlinkListener::init_dump_params()
{
	memset(&dump_params, 0, sizeof(dump_params)); /* globals should be 0ed out, but just in case we want to call this w/different params in future */
	dump_params.dp_type = NL_DUMP_STATS;
	dump_params.dp_fd = stdout;
}

struct nl_dump_params * NetlinkListener::get_dump_params()
{
	return &dump_params;
}

void NetlinkListener::log_and_die_rc(const char * msg, int rc)
{
	printf("%s. RC=%d\r\n", msg, rc);
 	exit(1);
}

void NetlinkListener::log_and_die(const char * msg)
{
	printf("%s\r\n", msg);
	exit(1);
}

void NetlinkListener::netlink_link_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data)
{
	struct rtnl_link * link = (struct rtnl_link *) obj;
	NetlinkListener * nll = (NetlinkListener *) data;
	rtnl_link_get_name(link);
 	printf("Link cache callback was triggered for link: %s\r\n", rtnl_link_get_name(link));

        nl_cache_dump(cache, nll->get_dump_params());
}

void NetlinkListener::netlink_route_updated(struct nl_cache * cache, struct nl_object * obj, int idk, void * data)
{
	struct rtnl_route * route = (struct rtnl_route *) obj;
	NetlinkListener * nll = (NetlinkListener *) data;
	printf("Route cache callback was triggered for route: %s\r\n", "");

	nl_cache_dump(cache, nll->get_dump_params());
}

void NetlinkListener::init()
{
	int rc = 0; /* track errors; defined in libnl3/netlinks/errno.h */

	init_dump_params();

	if ((sock = nl_socket_alloc()) == NULL)
	{
		log_and_die("Opening netlink socket failed");
  	}
  	else
  	{
    	printf("Opened netlink socket\r\n");
  	}
    
  	if ((rc = nl_connect(sock, NETLINK_ROUTE)) < 0)
  	{
   		nl_socket_free(sock);
    		log_and_die_rc("Connecting to netlink socket failed", rc);
  	}
  	else
  	{
  	  	printf("Connected to netlink socket\r\n");
  	}

  	if ((rc = rtnl_link_alloc_cache(sock, AF_UNSPEC, &link_cache)) < 0)
  	{
    		nl_socket_free(sock);
    		log_and_die_rc("Allocating link cache failed", rc);
  	}
  	else
  	{
    		printf("Allocated link cache\r\n");
  	}

  	if ((rc = rtnl_route_alloc_cache(sock, AF_UNSPEC, 0, &route_cache)) < 0)
  	{
    		nl_cache_free(link_cache);
    		nl_socket_free(sock);
    		log_and_die_rc("Allocating route cache failed", rc);
  	}
  	else
  	{
    		printf("Allocated route cache\r\n");
  	}

  	if ((rc = nl_cache_mngr_alloc(NULL, AF_UNSPEC, 0, &manager)) < 0)
  	{
    		nl_cache_free(link_cache);
    		nl_cache_free(route_cache);
    		nl_socket_free(sock);
    		log_and_die_rc("Failed to allocate cache manager", rc);
  	}
  	else
  	{
    		printf("Allocated cache manager\r\n");
  	}

  	nl_cache_mngt_provide(link_cache);
	nl_cache_mngt_provide(route_cache);
	
	printf("Initial Cache Manager:\r\n");
  	nl_cache_mngr_info(manager, get_dump_params());
  	printf("\r\nInitial Link Cache:\r\n");
  	nl_cache_dump(link_cache, get_dump_params());
  	printf("\r\nInitial Route Cache:\r\n");
  	nl_cache_dump(route_cache, get_dump_params());

  	if ((rc = nl_cache_mngr_add_cache(manager, route_cache, netlink_route_updated, this)) < 0)
  	{
    		nl_cache_mngr_free(manager);
    		nl_cache_free(link_cache);
    		nl_cache_free(route_cache);
    		nl_socket_free(sock);
    		log_and_die_rc("Failed to add route cache to cache manager", rc);
  	}
  	else
  	{
    		printf("Added route cache to cache manager\r\n");
  	}

  	if ((rc = nl_cache_mngr_add_cache(manager, link_cache, netlink_link_updated, this)) < 0)
  	{
    	nl_cache_mngr_free(manager);
    	nl_cache_free(link_cache);
    	nl_cache_free(route_cache);
    	nl_socket_free(sock);
    	log_and_die_rc("Failed to add link cache to cache manager", rc);
  	}
  	else
  	{	
    		printf("Added link cache to cache manager\r\n");
  	}
}

void NetlinkListener::init_ifaces(const char * prefix, int qty)
{
	if (sock == 0)
	{
		printf("Netlink listener socket not initialized. Initializing...\r\n");
		init();
	}

	/* create links/ifaces */
	for (int i = 0; i < qty; i++)
	{
		struct rtnl_link * new_link = NULL;
		int rc;

		if ((new_link = rtnl_link_alloc()) == NULL)
		{
			log_and_die("Could not allocate link");
		}

		printf("Adding link %s%d\r\n", prefix, i);
		//rtnl_link_set_name(new_link, prefix);
		//rtnl_link_set_ifindex(new_link, i);
		//rtnl_link_set_family(new_link, AF_UNSPEC);
		rtnl_link_set_flags(new_link, IFF_TAP | IFF_NO_PI);	
		//rtnl_link_set_group(new_link, 18); /* random for now */
		//rtnl_link_set_mtu(new_link, 1500);
		//rtnl_link_set_txqlen(new_link, 1000);
		rtnl_link_set_type(new_link, "tap");	
		if ((rc = rtnl_link_add(sock, new_link, NLM_F_CREATE)) < 0)
		{
			log_and_die_rc("Unable to create interface", rc);
		}

		/* clean up */
		rtnl_link_put(new_link);
	}
}

void NetlinkListener::startListening(int pollIntervalMillis)
{
	if (netlink_listener_thread == NULL)
	{
		netlink_listener_thread = new boost::thread(netlink_listener, pollIntervalMillis /* args to function ptr */, this);
		if (netlink_listener_thread == NULL)
		{
			log_and_die("Netlink listener thread creation failed");
		}
		printf("Started netlink listener thread\r\n");
	}
	else
	{
		printf("Tried to start netlink listener thread, but thread was already started\r\n");
	}
}

void NetlinkListener::stopListening()
{
	netlink_listener_thread->interrupt();
	delete netlink_listener_thread;
	netlink_listener_thread = NULL;
	printf("Stopped netlink listener thread\r\n");
}

void NetlinkListener::netlink_listener(int pollIntervalMillis, NetlinkListener * nll)
{
	int rc;
  	while(1)
  	{
		boost::this_thread::interruption_point();
    		if ((rc = nl_cache_mngr_poll(nll->manager, pollIntervalMillis)) < 0)
    		{
      			nl_cache_mngr_free(nll->manager);
      			nl_cache_free(nll->link_cache);
      			nl_cache_free(nll->route_cache);
      			nl_socket_free(nll->sock);
      			nll->log_and_die_rc("Failed to set poll for cache manager", rc);
    		}
    		else
    		{
      			if (rc > 0)
      			{
				printf("Processed %d updates from netlink\r\n", rc);	
      			}
			else
			{
      				printf("No news from netlink (%d updates to process). Polling...\r\n", rc);
			}
    		}
  	}
}
