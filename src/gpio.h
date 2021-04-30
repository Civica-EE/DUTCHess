#ifndef __DUTCHESS_GPIO_H__
#define __DUTCHESS_GPIO_H__

#include <stdint.h>

void dutchess_gpio_init ();
void dutchess_gpio_configure (int pin);
void dutchess_gpio_set (int pin, int value);
int dutchess_gpio_get (int pin);

#endif // __DUTCHESS_GPIO_H__
