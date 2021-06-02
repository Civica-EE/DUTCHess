/* -*- mode:c; indent-tabs-mode:nil; tab-width:4; c-basic-offset:4 -*- */
/*     vi: set ts=4 sw=4 expandtab:                                    */

/*@COPYRIGHT@*/

#include "DUTCHess.h"
#include "dut_serial.h"
#include <device.h>
#include <init.h>
#include <fcntl.h>
#include <net/socket.h>
#include <poll.h>

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
#error "We must has interrupts"
#endif

#define TELNET_IAC   255
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254

#define DUT_CK(rt,...) do {                                     \
        int _err;                                               \
        if ((_err = dut_log_error(__FILE__, __LINE__, (rt)))) { \
            __VA_ARGS__;                                        \
        }                                                       \
    }                                                           \
    while (0)

#define DUT_NOK(er) dut_log_error(__FILE__, __LINE__, (er))

struct dut_uart;

struct dut_uart_port {
    struct dut_uart_port *next;
    struct dut_uart *uart;

    struct dut_serial_cfg cfg;
    int socket;
    int id; // Linuar unique id across all ports
};

struct dut_uart {
    struct dut_uart *next;
    
    const struct device *dev;

    struct dut_uart_port *ports; // All ports sharing this device

    int csocket; // Client socket (non-0 means uart in use)
    int id; // Linuar unique id across all uarts
};

static struct dut_uart *dut_uarts = NULL;
static int dut_ports_count = 0;
static int dut_uarts_count = 0;

static int dut_log_error (char *file_name, int line, int ret_val)
{
    if (ret_val < 0)
    {
        printk("%s:%d: Error: %d (errno %d)\n", file_name, line, ret_val, errno);
    }

    return ret_val;
}

static int dut_setblocking (int fd, bool val)
{
	int fl, res;

	fl = zsock_fcntl(fd, F_GETFL, 0);
	if (fl < 0) {
		return DUT_NOK(fl);
	}

	if (val) {
		fl &= ~O_NONBLOCK;
	} else {
		fl |= O_NONBLOCK;
	}

	res = zsock_fcntl(fd, F_SETFL, fl);
	if (res < 0) {
        return DUT_NOK(res);
    }

    return 0;
}

static void dut_uart_isr (const struct device *dev, void *user_data)
{
    (void) dev;
    struct dut_uart *uart = (struct dut_uart*) user_data;

    SAY("ISR trigger %s", uart->ports->cfg.dev_name);
    
    while (uart_irq_update(uart->dev) &&
                  uart_irq_is_pending(uart->dev))
    {
        char buf[128];
        int len;

        if (!uart_irq_rx_ready(uart->dev))
            continue;
        
		len = uart_fifo_read(uart->dev, buf, 128);

        if (len < 0)
        {
            DUT_NOK(len);
            break;
        }
        else if (len == 0)
        {
            break;
        }
        else if (uart->csocket != -1) // note this may have been closed in the mean time
        {
            
            DUT_CK(zsock_send(uart->csocket, buf, len, 0));
        }
	}
}

static int dut_serial_init (struct dut_uart_port *port)
{
    struct dut_uart *uart = port->uart;
	uart_irq_rx_disable(uart->dev);
	uart_irq_tx_disable(uart->dev);

    /* set speed etc */
    uart_configure(uart->dev, &port->cfg.cfg);
    
	uart_irq_callback_user_data_set(uart->dev, dut_uart_isr, uart);

	/* Drain the fifo */
	while (uart_irq_rx_ready(uart->dev)) {
        char c;
		uart_fifo_read(uart->dev, &c, 1);
	}

	uart_irq_rx_enable(uart->dev);

    return 0;
}

static int dut_serial_write (struct dut_uart *uart, const uint8_t *buf, size_t len)
{
    /*
    DUT_CK(uart_tx(uart->dev, buf, len, SYS_FOREVER_MS),
           return _err);
    */

    for (int i = 0; i < len; ++i)
        uart_poll_out(uart->dev, buf[i]);

    char ch;
    if (uart_poll_in(uart->dev, &ch) == 0)
        SAY("Got a char '%c' (%02x)", ch, ch);
    
    return 0;
}


// Adds the port with access point sap into the uart's list of ports (cerating
// the former if needed).
static int dut_add_port (struct dut_serial_cfg *sap)
{
    struct dut_uart *uart = NULL;
    struct dut_uart_port *port = NULL;
	struct sockaddr_in baddr = {
		.sin_family = AF_INET,
		.sin_port = htons(sap->tcp_port),
		.sin_addr = {
			.s_addr = htonl(INADDR_ANY),
		},
	};

    SAY("Serial %s speed %d port %d", sap->dev_name, sap->cfg.baudrate, sap->tcp_port);

    for (uart = dut_uarts; uart; uart = uart->next)
        if (!strcmp(uart->ports->cfg.dev_name, sap->dev_name))
        {
            break;
        }

    if (!uart)
    {
        uart = k_malloc(sizeof(struct dut_uart));

        if (!uart)
            return DUT_NOK(-ENOMEM);

        if ((uart->dev = device_get_binding(sap->dev_name)) == NULL)
        {
            k_free(uart);
            return DUT_NOK(-ENODEV);
        }

        uart->ports = NULL;
        uart->csocket = -1;

        uart->id = dut_uarts_count++;
        uart->next = dut_uarts;
        dut_uarts = uart;
    }

    port = k_malloc(sizeof(struct dut_uart_port));
    if (!port)
    {
        dut_uarts = uart->next;
        k_free(uart);
        return DUT_NOK(-ENOMEM);
    }

    port->cfg = *sap;

    port->socket = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (port->socket < 0)
    {
        k_free(port);
        return DUT_NOK(port->socket);
    }
    
    DUT_CK(zsock_bind(port->socket, (const struct sockaddr*) &baddr, sizeof(baddr)),
           {
               zsock_close(port->socket);
               k_free(port);
               return _err;
           });
    
    DUT_CK(dut_setblocking(port->socket, false),
           {
               zsock_close(port->socket);
               k_free(port);
               return _err;
           });

    port->id = dut_ports_count++;
    port->uart = uart;
    port->next = uart->ports;
    uart->ports = port;
    
    return 0;

}

int dut_serial_thread_fun (int dummy1, int dummy2, int dummy3)
{
    struct pollfd fds[dut_ports_count + dut_uarts_count], *cfds = &fds[dut_ports_count];
    struct pollfd *fp;

    (void) dummy1, (void) dummy2, (void) dummy3;

    SAY("Serial server running, %d serials x %d ports...", dut_uarts_count, dut_ports_count);

    fp = fds;
    for (struct dut_uart *uart = dut_uarts; uart; uart = uart->next)
        for (struct dut_uart_port *port = uart->ports; port; port = port->next)
        {
            DUT_CK(zsock_listen(port->socket, 5));
            fp->fd = port->socket;
            fp->events = POLLIN;
            ++fp;
        }

    SAY("Fp now %p start is %p end is %p elem is %d", fp, fds, fds + 1, sizeof(fds));
    for (; fp < &fds[dut_ports_count + dut_uarts_count]; ++fp)
    {
        SAY("Set cfd %d", fp - fds);
        fp->fd = -1;
        fp->events = POLLIN;
    }


    for (;;)
    {
        SAY("poll %d fds..", sizeof(fds) / sizeof(fds[0]));
        DUT_CK(zsock_poll(fds, dut_ports_count + dut_uarts_count, -1),
               if (_err <= 0) continue);
        SAY("end..");

        fp = fds;
        for (struct dut_uart *uart = dut_uarts; uart; uart = uart->next)
            for (struct dut_uart_port *port = uart->ports; port; port = port->next)
            {
                if (fp->revents & POLLIN)
                {
                    struct sockaddr_storage client_addr;
                    void *addr = &((struct sockaddr_in*) &client_addr)->sin_addr;
                    socklen_t client_addr_len = sizeof(client_addr);
                    char addr_str[32];

                    int client = zsock_accept(fp->fd,
                                              (struct sockaddr*) &client_addr,
                                              &client_addr_len);
                    SAY("Accept client fd %d", client);
                    if (client < 0) {
                        DUT_NOK(client);
                        continue;
                    }

                    zsock_inet_ntop(client_addr.ss_family, addr,
                                    addr_str, sizeof(addr_str));

                    SAY("New connection from %s serial %s@%d\n",
                        addr_str,
                        port->cfg.dev_name,
                        port->cfg.cfg.baudrate);

                    if (uart->csocket != -1)
                    {
                        char *msg = "Connection in use.\r\n";
                        zsock_send(client, msg, sizeof(msg), 0);
                        zsock_close(client);
                    }
                    else
                    {
                        dut_setblocking(client, false);
                        cfds[uart->id].fd = uart->csocket = client;

                        unsigned char charmode[]={255, 251, 3, 255, 251, 1, 0, 0, 0};
                        zsock_send(client, charmode, sizeof(charmode), 0);
                    }

                    DUT_CK(dut_serial_init(port));
                }

                fp++;
            }

        
        for (struct dut_uart *uart = dut_uarts; uart; uart = uart->next)
        {
            if (fp->revents & POLLIN)
            {
                char buf[1];

                SAY("Activity on fd %d", fp->fd);
                int len = zsock_recv(fp->fd, buf, sizeof(buf), 0);
                
                if (len < 0)
                {
                    DUT_NOK(len);
                }
                else if (len == 0)
                {
                    struct sockaddr_storage client_addr;
                    void *addr = &((struct sockaddr_in *)&client_addr)->sin_addr;
                    socklen_t client_addr_len = sizeof(client_addr);
                    char addr_str[32];
                    
                    zsock_getsockname(fp->fd,
                                      (struct sockaddr*) &client_addr,
                                      &client_addr_len);
                    
                    zsock_inet_ntop(client_addr.ss_family, addr,
                                    addr_str, sizeof(addr_str));
                    
                    SAY("Closing connection from %s serial %s\n",
                        addr_str,
                        uart->ports->cfg.dev_name);
                    
                    fp->fd = -1;
                    cfds[uart->id].fd = uart->csocket = -1;
                }
                else
                {
                    if (buf[0] == TELNET_IAC)
                    {
                        zsock_recv(fp->fd, buf, sizeof(buf), 0);
                    }
                    else
                    {
                        SAY("Sending %d bytes [%02x] to serial %s",
                            len,
                            buf[0], 
                            uart->ports->cfg.dev_name);
                        DUT_CK(dut_serial_write(uart, buf, len));
                    }
                }
            }

            fp++;
        }
    }
    
    return 0;
}

K_THREAD_DEFINE(dut_serial_thread, 4096, dut_serial_thread_fun,
                NULL, NULL, NULL,
                0, 0, 0);

int dut_serial_start (struct dut_serial_cfg *cfg)
{
    SAY("Starting serial server...");

    for (struct dut_serial_cfg *port = cfg; port->dev_name; ++port)
    {
        DUT_CK(dut_add_port(port));
    }

    return 0;
}
