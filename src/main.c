#include "led.h"
#include "relay.h"

void main (void)
{
    int relay = 0;

    dutchess_led_init();
    dutchess_relay_init();

    while (1)
    {
        int32_t durations_ms[] = {500, 500, 500, 1000, 1000, 1000, 500, 500, 500};
        dutchess_led_blink(durations_ms, 9);
        relay = !relay;
        dutchess_relay_set(relay);
    }
}
