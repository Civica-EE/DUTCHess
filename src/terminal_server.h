#ifndef __DUTCHESS_TERMINAL_SERVER_H__
#define __DUTCHESS_TERMINAL_SERVER_H__

#include <drivers/uart.h>

struct dutchess_terminal_server_cfg {
    struct uart_config cfg;
    int                tcp_port;
};

void dutchess_terminal_server_init ();
int dutchess_terminal_server_add (struct dutchess_terminal_server_cfg *sap);
int dutchess_terminal_server_del (struct dutchess_terminal_server_cfg *sap);

#endif /* __DUTCHESS_TERMINAL_SERVER_H__ */
