#include <logging/log.h>
#include <net/net_if.h>

#include "net.h"

LOG_MODULE_REGISTER(dutchess_net);

struct net_if *iface = NULL;

// Store the IP address at a global level; we need to remove this explicitly to
// add a new one.
static struct in_addr address;

void dutchess_net_init ()
{
    char address_str[] = "192.0.2.1";
    char netmask_str[] = "255.255.255.0";
    char gateway_str[] = "192.0.2.2";
    struct in_addr netmask;
    struct in_addr gateway;

    iface = net_if_get_default();

    if (net_addr_pton(AF_INET, address_str, &address))
    {
        LOG_ERR("Invalid address: %s\n", address_str);
        return;
    }

    if (net_addr_pton(AF_INET, netmask_str, &netmask))
    {
        LOG_ERR("Invalid netmask: %s\n", netmask_str);
        return;
    }

    if (net_addr_pton(AF_INET, gateway_str, &gateway))
    {
        LOG_ERR("Invalid address: %s\n", gateway_str);
        return;
    }

    // Set default values.
    dutchess_net_address_set(address);
    dutchess_net_netmask_set(netmask);
    dutchess_net_gateway_set(gateway);
}

void dutchess_net_address_set (struct in_addr new_address)
{
    if (iface)
    {
        net_if_ipv4_addr_rm(iface, &address);
        net_if_ipv4_addr_add(iface, &new_address, NET_ADDR_MANUAL, 0);
        memcpy(&address, &new_address, sizeof(address));
    }
}

void dutchess_net_netmask_set (struct in_addr netmask)
{
    if (iface)
    {
        net_if_ipv4_set_netmask(iface, &netmask);
    }
}

void dutchess_net_gateway_set (struct in_addr gateway)
{
    if (iface)
    {
        net_if_ipv4_set_gw(iface, &gateway);
    }
}

