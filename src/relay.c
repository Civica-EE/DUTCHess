#include <stdio.h>
#include <drivers/gpio.h>

#include "relay.h"

#define PIN 25 // D0 on the Arduino header.

static const struct device *dev = NULL;

void dutchess_relay_init ()
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    if ((dev = device_get_binding("GPIO_1")) == NULL)
    {
        return;
    }

    if (gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE) < 0)
    {
        return;
    }
#endif
}

void dutchess_relay_set (int value)
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    if (dev)
    {
        gpio_pin_set(dev, PIN, value);
    }
#endif
}
