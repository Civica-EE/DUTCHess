#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define SLEEP_TIME_MS   500

#define LED0_NODE DT_ALIAS(led0)

#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

void main()
{
    const struct device *dev;

    if ((dev = device_get_binding(LED0)) == NULL)
    {
        return;
    }

    if (gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS) < 0)
    {
        return;
    }

    while (true)
    {
        gpio_pin_set(dev, PIN, !gpio_pin_get(dev, PIN));
        k_msleep(SLEEP_TIME_MS);
    }
}
