/* -*- mode:c; indent-tabs-mode:nil; tab-width:4; c-basic-offset:4 -*- */
/*     vi: set ts=4 sw=4 expandtab:                                    */

/*@COPYRIGHT@*/

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <drivers/uart.h>

#define DUT_SERIAL_PORT_BASE 20000

struct dut_serial_cfg {
    char              *dev_name;
    struct uart_config cfg;
    int                tcp_port;
};

#define DUT_SERIAL_LAST {NULL}

int dut_serial_start (struct dut_serial_cfg *cfg);

#endif /* __SERIAL_H__ */
