#include <net/net_if.h>
#include <settings/settings.h>

#include "net.h"

struct net_if *iface = NULL;

static struct in_addr address;

int network_settings_set (const char *name,
                          size_t len,
                          settings_read_cb read_cb,
                          void *cb_arg)
{
    const char *next;
    size_t name_len;
    struct in_addr addr;

    name_len = settings_name_next(name, &next);

    if (!next)
    {
        if (!strncmp(name, "address", name_len))
        {
            read_cb(cb_arg, &addr, sizeof(addr));
            dutchess_net_address_set(addr);
            return 0;
        }
        else if (!strncmp(name, "netmask", name_len))
        {
            read_cb(cb_arg, &addr, sizeof(addr));
            dutchess_net_netmask_set(addr);
            return 0;
        }
        else if (!strncmp(name, "gateway", name_len))
        {
            read_cb(cb_arg, &addr, sizeof(addr));
            dutchess_net_gateway_set(addr);
            return 0;
        }
    }

    return -ENOENT;
}

SETTINGS_STATIC_HANDLER_DEFINE(network,
                               "network",
                               NULL,
                               network_settings_set,
                               NULL,
                               NULL);

void dutchess_net_init ()
{
    iface = net_if_get_default();
}

void dutchess_net_address_set (struct in_addr new_address)
{
    if (iface)
    {
        if (address.s_addr != 0)
        {
            net_if_ipv4_addr_rm(iface, &address);
        }

        net_if_ipv4_addr_add(iface, &new_address, NET_ADDR_MANUAL, 0);
        memcpy(&address, &new_address, sizeof(address));
        settings_save_one("network/address", &address, sizeof(address));
    }
}

void dutchess_net_netmask_set (struct in_addr netmask)
{
    if (iface)
    {
        net_if_ipv4_set_netmask(iface, &netmask);
        settings_save_one("network/netmask", &netmask, sizeof(netmask));
    }
}

void dutchess_net_gateway_set (struct in_addr gateway)
{
    if (iface)
    {
        net_if_ipv4_set_gw(iface, &gateway);
        settings_save_one("network/gateway", &gateway, sizeof(gateway));
    }
}

