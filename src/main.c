#include "led.h"

void main (void)
{
    dutchess_led_init();

    while (1)
    {
        int32_t durations_ms[] = {500, 500, 500, 1000, 1000, 1000, 500, 500, 500};
        dutchess_led_blink(durations_ms, 9);
    }
}
