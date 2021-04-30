#include <stdio.h>
#include <drivers/gpio.h>

#include "gpio.h"
#include "relay.h"

#define PIN 25 // D0 on the Arduino header.

void dutchess_relay_init ()
{
    dutchess_gpio_configure(PIN);
}

void dutchess_relay_set (int value)
{
    // Relay is active low so invert the value here to abstract that.
    dutchess_gpio_set(PIN, !value);
}

int dutchess_relay_state ()
{
    // As with the function above, need to invert the value so the rest of the
    // application doesn't have to care that this device is active low.
    return !dutchess_gpio_get(PIN);
}
