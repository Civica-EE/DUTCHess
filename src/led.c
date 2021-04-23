#include <drivers/gpio.h>

#include "led.h"

#define LED0_NODE DT_ALIAS(led0)
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

#define BLINK_INTERVAL_MS 500

static const struct device *dev = NULL;

void dutchess_led_init ()
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    if ((dev = device_get_binding(LED0)) == NULL)
    {
        return;
    }

    if (gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS) < 0)
    {
        return;
    }
#endif
}

void dutchess_led_blink (int32_t *durations_ms, int32_t blinks)
{
#ifdef CONFIG_BOARD_MIMXRT1020_EVK
    if (dev)
    {
        // Blink the LED for the specified durations and number of blinks.
        for (int i = 0; i < blinks; i++)
        {
            gpio_pin_set(dev, PIN, 1);
            k_msleep(durations_ms[i]);
            gpio_pin_set(dev, PIN, 0);
            k_msleep(BLINK_INTERVAL_MS);
        }

        // Sleep afterwards so that it is easy to distinguish between different
        // sequences.
        k_msleep(BLINK_INTERVAL_MS * 4);
    }
#endif
}
