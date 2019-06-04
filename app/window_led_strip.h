#ifndef _WINDOW_LED_STRIP_H
#define _WINDOW_LED_STRIP_H
#include <bcl.h>
#include <bc_ws2812b.h>

#define FLOOR_1 51
#define FLOOR_2 43

void window_led_strip_init(void);

bc_led_strip_t *window_led_strip_get_full(void);

bc_led_strip_t *window_led_strip_get_floor1(void);

bc_led_strip_t *window_led_strip_get_floor2(void);

#endif
