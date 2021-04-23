#ifndef __DUTCHESS_LED_H__
#define __DUTCHESS_LED_H__

#include <stdint.h>

void dutchess_led_init ();
void dutchess_led_blink (int32_t *durations_ms, int32_t blinks);

#endif // __DUTCHESS_LED_H__
