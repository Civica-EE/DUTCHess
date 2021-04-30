#include <stdio.h>
#include <drivers/gpio.h>

#include "gpio.h"

static const struct device *dev = NULL;

void dutchess_gpio_init ()
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    dev = device_get_binding("GPIO_1");
#endif
}

void dutchess_gpio_configure (int pin)
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    gpio_pin_configure(dev, pin, GPIO_OUTPUT_ACTIVE);
#endif
}

void dutchess_gpio_set (int pin, int value)
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    gpio_pin_set(dev, pin, value);
#endif
}

int dutchess_gpio_get (int pin)
{
    int value = 0;
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    value = gpio_pin_get(dev, pin);
#endif
    return value;
}
