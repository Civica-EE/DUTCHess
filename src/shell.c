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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dutchess,
                               SHELL_CMD(relay, NULL, "Relay command.", dutchess_relay_cmd),
                               SHELL_SUBCMD_SET_END);
SHELL_CMD_REGISTER(dutchess, &sub_dutchess, "DUTCHess commands", NULL);
