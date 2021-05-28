#include <fcntl.h>
#include <logging/log.h>
#include <poll.h>
#include <posix/pthread.h>
#include <settings/settings.h>

#include "terminal_server.h"

#ifndef CONFIG_UART_INTERRUPT_DRIVEN
#error "We must has interrupts"
#endif

#define DUTCHESS_TERMINAL_SERVER_MAX_PORTS 10
// Use D0 and D1 (UART 2 RX/TX)
#define DUTCHESS_TERMINAL_SERVER_UART_NAME "UART_2"
#define TELNET_IAC   255

LOG_MODULE_REGISTER(dutchess_terminal_server);

K_THREAD_STACK_DEFINE(dutchess_terminal_server_stack, CONFIG_MAIN_STACK_SIZE);

static struct dutchess_terminal_server_cfg servers[DUTCHESS_TERMINAL_SERVER_MAX_PORTS];

// Sockets to poll; one more than the number of servers to handle the client
// socket.
static struct pollfd fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS + 1];
static const struct device *dev;
static pthread_mutex_t lock;

static int dutchess_terminal_server_blocking_set (int fd,
                                                  bool val)
{
    int fl;
    int ret;

    if ((fl = zsock_fcntl(fd, F_GETFL, 0)) < 0)
    {
        return fl;
    }

    if (val)
    {
        fl &= ~O_NONBLOCK;
    }
    else
    {
        fl |= O_NONBLOCK;
    }

    if ((ret = zsock_fcntl(fd, F_SETFL, fl)) < 0)
    {
        return ret;
    }

    return 0;
}

int dutchess_terminal_server_add (struct dutchess_terminal_server_cfg *cfg)
{
    int ret = 0;
    struct dutchess_terminal_server_cfg *server = NULL;
    int *fd = NULL;
    struct sockaddr_in baddr = {
        .sin_family = AF_INET,
        .sin_port = htons(cfg->tcp_port),
        .sin_addr = {
            .s_addr = htonl(INADDR_ANY),
        },
    };

    if ((ret = pthread_mutex_lock(&lock)) != 0)
    {
        LOG_ERR("Could not lock!");
        return ret;
    }

    // Find somewhere in the configuration to store this terminal server.
    for (int i = 0; i < DUTCHESS_TERMINAL_SERVER_MAX_PORTS; i++)
    {
        if (fds[i].fd == -1)
        {
            server = &servers[i];
            fd = &fds[i].fd;
            break;
        }
    }

    if (!server)
    {
        LOG_ERR("Terminal server port limit exceeded!");
        return -ENOMEM;
    }

    // Open the socket, bind it to the configured port, and make it non-blocking.
    if ((ret = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        return ret;
    }

    *fd = ret;

    if ((ret = zsock_bind(*fd, (const struct sockaddr*) &baddr, sizeof(baddr))) < 0)
    {
        zsock_close(*fd);
        *fd = -1;
        return ret;
    }

    if ((ret = dutchess_terminal_server_blocking_set(*fd, false)) < 0)
    {
        zsock_close(*fd);
        *fd = -1;
        return ret;
    }

    // Listen for connections on this socket; only one UART here so only accept
    // a single connection at a time.
    if ((ret = zsock_listen(*fd, 1)) < 0)
    {
        zsock_close(*fd);
        *fd = -1;
        return ret;
    }

    // Save the configuration.
    memcpy(server, cfg, sizeof(*cfg));
    settings_save_one("terminal_server/config", servers, sizeof(servers));

    pthread_mutex_unlock(&lock);

    return 0;
}

int dutchess_terminal_server_del (struct dutchess_terminal_server_cfg *cfg)
{
    struct dutchess_terminal_server_cfg *server = NULL;
    int *fd = NULL;
    int ret = 0;

    if ((ret = pthread_mutex_lock(&lock)) != 0)
    {
        LOG_ERR("Could not lock!");
        return ret;
    }

    // Find the matching element in the servers array.
    for (int i = 0; i < DUTCHESS_TERMINAL_SERVER_MAX_PORTS; i++)
    {
        if (fds[i].fd != -1 &&
            memcmp(cfg, &servers[i], sizeof(*cfg)) == 0)
        {
            // Found.
            server = &servers[i];
            fd = &fds[i].fd;
        }
    }

    if (!server)
    {
        LOG_ERR("Terminal server not found!");
        return -EINVAL;
    }

    // Close the socket
    zsock_close(*fd);

    *fd = -1;

    // Clear the configuration.
    memset(server, 0, sizeof(*server));
    settings_save_one("terminal_server/config", servers, sizeof(servers));

    pthread_mutex_unlock(&lock);

    return 0;
}

static void dutchess_terminal_server_uart_isr (const struct device *dev, void *user_data)
{
    while (uart_irq_update(dev) &&
           uart_irq_is_pending(dev))
    {
        char buf[128];
        int len;

        if (!uart_irq_rx_ready(dev))
            continue;

        len = uart_fifo_read(dev, buf, 128);

        if (len < 0)
        {
            LOG_ERR("Failed UART read!");
            break;
        }
        else if (len == 0)
        {
            break;
        }
        else if (fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS].fd != -1) // Note: this may have been closed in the mean time.
        {
            if (zsock_send(fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS].fd, buf, len, 0) < 0)
            {
                LOG_ERR("Failed to send data to the session!");
            }
        }
    }
}

static int dutchess_terminal_server_uart_init (struct uart_config *cfg)
{
    // Disable interrupts while we set things up.
    uart_irq_rx_disable(dev);
    uart_irq_tx_disable(dev);

    // Set the baudrate, etc.
    uart_configure(dev, cfg);

    // Set our ISR.
    uart_irq_callback_user_data_set(dev, dutchess_terminal_server_uart_isr, NULL);

    // Drain the FIFO.
    while (uart_irq_rx_ready(dev))
    {
        char c;
        uart_fifo_read(dev, &c, 1);
    }

    // Bring everything up.
    uart_irq_rx_enable(dev);

    return 0;
}

static int dutchess_terminal_server_uart_write (const uint8_t *buf, size_t len)
{
    for (int i = 0; i < len; ++i)
    {
        uart_poll_out(dev, buf[i]);
    }

    char ch;
    if (uart_poll_in(dev, &ch) == 0)
    {
        LOG_INF("Got a char '%c' (%02x)", ch, ch);
    }

    return 0;
}

void *server (void *arg)
{
    while (1)
    {
        int ret = 0;

        if ((ret = pthread_mutex_lock(&lock)) != 0)
        {
            LOG_ERR("Error during lock (%d)!", ret);
            continue;
        }

        if ((ret = zsock_poll(fds, DUTCHESS_TERMINAL_SERVER_MAX_PORTS + 1, -1)) < 0)
        {
            LOG_ERR("Error during poll (%d)!", ret);
            continue;
        }

        // Check for new connections.
        for (int i = 0; i < DUTCHESS_TERMINAL_SERVER_MAX_PORTS; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                struct sockaddr_storage client_addr;
                void *addr = &((struct sockaddr_in*) &client_addr)->sin_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                char addr_str[32];
                int client = -1;

                if ((client = zsock_accept(fds[i].fd,
                                           (struct sockaddr*) &client_addr,
                                           &client_addr_len)) < 0)
                {
                    LOG_ERR("Error accepting connection (%d)", client);
                    continue;
                }

                zsock_inet_ntop(client_addr.ss_family, addr, addr_str, sizeof(addr_str));

                LOG_INF("New connection from %s", addr_str);

                if (fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS].fd != -1)
                {
                    char *msg = "Connection in use.\r\n";
                    zsock_send(client, msg, sizeof(msg), 0);
                    zsock_close(client);
                }
                else
                {
                    unsigned char charmode[]={255, 251, 3, 255, 251, 1, 0, 0, 0};

                    dutchess_terminal_server_blocking_set(client, false);

                    fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS].fd = client;

                    zsock_send(client, charmode, sizeof(charmode), 0);
                }

                dutchess_terminal_server_uart_init(&servers[i].cfg);
            }
        }

        // Check for data on the client socket.
        if (fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS].fd != -1 &&
            fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS].revents & POLLIN)
        {
            int *fd = &fds[DUTCHESS_TERMINAL_SERVER_MAX_PORTS].fd;
            char buf[1];

            int len = zsock_recv(*fd, buf, sizeof(buf), 0);

            if (len < 0)
            {
                LOG_ERR("Error during receive (%d)", len);
            }
            else if (len == 0)
            {
                struct sockaddr_storage client_addr;
                void *addr = &((struct sockaddr_in *)&client_addr)->sin_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                char addr_str[32];

                zsock_getsockname(*fd,
                        (struct sockaddr*) &client_addr,
                        &client_addr_len);

                zsock_inet_ntop(client_addr.ss_family, addr,
                        addr_str, sizeof(addr_str));

                LOG_INF("Closing connection from %s", addr_str);

                *fd = -1;
            }
            else
            {
                if (buf[0] == TELNET_IAC)
                {
                    zsock_recv(*fd, buf, sizeof(buf), 0);
                }
                else
                {
                    dutchess_terminal_server_uart_write(buf, len);
                }
            }
        }

        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

void dutchess_terminal_server_init ()
{
    pthread_attr_t attr;
    pthread_t thread;

    dev = device_get_binding(DUTCHESS_TERMINAL_SERVER_UART_NAME);

    for (int i = 0; i < DUTCHESS_TERMINAL_SERVER_MAX_PORTS + 1; i++)
    {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        LOG_ERR("Could not create lock!");
        return;
    }

    pthread_attr_init(&attr);
    pthread_attr_setstack(&attr,
                          &dutchess_terminal_server_stack,
                          CONFIG_MAIN_STACK_SIZE);

    pthread_create(&thread, &attr, server, 0);
}
