#include <net/net_ip.h>
#include <net/net_if.h>
#include <shell/shell.h>

#include "relay.h"

static int dutchess_relay_cmd (const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2)
    {
        shell_print(shell, "Usage: Please supply on/off as a parameter\n");
    }
    else
    {
        if (!strcmp(argv[1], "on"))
        {
            dutchess_relay_set(1);
            shell_print(shell, "Power ON\n");
        }
        else if (!strcmp(argv[1], "off"))
        {
            dutchess_relay_set(0);
            shell_print(shell, "Power OFF\n");
        }
        else
        {
            shell_print(shell, "Please supply on/off\n");
        }
    }

    return 0;
}

static int dutchess_ip_address_cmd (const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2)
    {
        shell_print(shell, "Usage: Please supply an IP address as a parameter\n");
    }
    else
    {
        struct net_if *iface = NULL;
        struct in_addr addr;

        if (net_addr_pton(AF_INET, argv[1], &addr))
        {
            shell_print(shell, "Invalid address: %s\n", argv[1]);
            return -1;
        }

        iface = net_if_get_default();

        net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
    }

    return 0;
}

static int dutchess_ip_netmask_cmd (const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2)
    {
        shell_print(shell, "Usage: Please supply a dotted decimal netmask as a parameter\n");
    }
    else
    {
        struct net_if *iface = NULL;
        struct in_addr addr;

        if (net_addr_pton(AF_INET, argv[1], &addr))
        {
            shell_print(shell, "Invalid address: %s\n", argv[1]);
            return -1;
        }

        iface = net_if_get_default();

        net_if_ipv4_set_netmask(iface, &addr);
    }

    return 0;
}

static int dutchess_ip_gateway_cmd (const struct shell *shell, size_t argc, char **argv)
{
    if (argc != 2)
    {
        shell_print(shell, "Usage: Please supply an IP address as a parameter\n");
    }
    else
    {
        struct net_if *iface = NULL;
        struct in_addr addr;

        if (net_addr_pton(AF_INET, argv[1], &addr))
        {
            shell_print(shell, "Invalid address: %s\n", argv[1]);
            return -1;
        }

        iface = net_if_get_default();

        net_if_ipv4_set_gw(iface, &addr);
    }

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dutchess_ip,
                               SHELL_CMD(address,   NULL, "IP address command.", dutchess_ip_address_cmd),
                               SHELL_CMD(netmask,   NULL, "IP netmask command.", dutchess_ip_netmask_cmd),
                               SHELL_CMD(gateway,   NULL, "IP gateway command.", dutchess_ip_gateway_cmd),
                               SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(sub_dutchess,
                               SHELL_CMD(relay, NULL, "Relay command.", dutchess_relay_cmd),
                               SHELL_CMD(ip, &sub_dutchess_ip, "IP command.", NULL),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(dutchess, &sub_dutchess, "DUTCHess commands", NULL);
