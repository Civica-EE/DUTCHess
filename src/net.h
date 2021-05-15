#ifndef __DUTCHESS_NET_H__
#define __DUTCHESS_NET_H__

#include <net/net_ip.h>

void dutchess_net_init ();
void dutchess_net_address_set (struct in_addr address);
void dutchess_net_netmask_set (struct in_addr netmask);
void dutchess_net_gateway_set (struct in_addr gateway);

#endif // __DUTCHESS_NET_H__
