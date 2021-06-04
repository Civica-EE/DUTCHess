#include "gpio.h"
#include "fs.h"
#include "led.h"
#include "net.h"
#include "relay.h"
#include "settings.h"
#include "temperature.h"
#include "terminal_server.h"
#include "tftp.h"
#include "web.h"

static struct dutchess_terminal_server_cfg dut_serial_cfg[] = {
    {
        .cfg = {
            .baudrate = 115200,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = 1,
            .data_bits = 8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
        },
       .tcp_port = 21500
    },
    {
        .cfg = {
            .baudrate = 57600,
            .parity = UART_CFG_PARITY_NONE,
            .stop_bits = 1,
            .data_bits = 8,
            .flow_ctrl = UART_CFG_FLOW_CTRL_NONE
        },
        .tcp_port = 25700
    },
};

void main (void)
{
    dutchess_gpio_init();
    dutchess_fs_init();
    dutchess_led_init();
    dutchess_net_init();
    dutchess_relay_init();
    dutchess_temperature_init();
    dutchess_terminal_server_init();
    dut_start_tftpServer();
    dutchess_web_server_init();

    // Initialise the settings last as this will read the settings and call the
    // relevant modules.
    dutchess_settings_init();

    for (int i = 0; i < 2; i++)
    {
        dutchess_terminal_server_add(&dut_serial_cfg[i]);
    }

    while (1)
    {
        int32_t durations_ms[] = {500, 500, 500, 1000, 1000, 1000, 500, 500, 500};
        dutchess_led_blink(durations_ms, 9);
    }
}
