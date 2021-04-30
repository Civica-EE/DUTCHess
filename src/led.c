#include <drivers/gpio.h>

#include "gpio.h"
#include "led.h"

#define BLINK_INTERVAL_MS 500

#define PIN 5

void dutchess_led_init ()
{
    dutchess_gpio_configure(PIN);
}

void dutchess_led_blink (int32_t *durations_ms, int32_t blinks)
{
    // Blink the LED for the specified durations and number of blinks.
    for (int i = 0; i < blinks; i++)
    {
        // LED is active low, so pull the GPIO low for the duration.
        dutchess_gpio_set(PIN, 0);
        k_msleep(durations_ms[i]);

        // Turn the LED off for the interval.
        dutchess_gpio_set(PIN, 1);
        k_msleep(BLINK_INTERVAL_MS);
    }

    // Sleep afterwards so that it is easy to distinguish between different
    // sequences.
    k_msleep(BLINK_INTERVAL_MS * 4);
}
