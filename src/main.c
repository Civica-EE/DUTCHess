#include "gpio.h"
#include "fs.h"
#include "led.h"
#include "relay.h"
#include "web.h"

void main (void)
{
    dutchess_gpio_init();
    dutchess_fs_init();
    dutchess_led_init();
    dutchess_relay_init();
    dutchess_web_server_init();

    while (1)
    {
        int32_t durations_ms[] = {500, 500, 500, 1000, 1000, 1000, 500, 500, 500};
        dutchess_led_blink(durations_ms, 9);
    }
}
