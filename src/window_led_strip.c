#include <window_led_strip.h>

#define LED_STRIP_TYPE TWR_LED_STRIP_TYPE_RGBW
#define LED_STRIP_COUNT (FLOOR_1 + FLOOR_2)

// Led strip
static uint32_t _twr_module_power_led_strip_dma_buffer[LED_STRIP_COUNT * LED_STRIP_TYPE * 2];

static const twr_led_strip_buffer_t led_strip_buffer =
{
    .type = LED_STRIP_TYPE,
    .count = LED_STRIP_COUNT,
    .buffer = _twr_module_power_led_strip_dma_buffer
};

static twr_led_strip_t full_led_strip;

static const twr_led_strip_buffer_t floor_1_led_strip_buffer =
{
    .type = LED_STRIP_TYPE,
    .count = FLOOR_1,
    .buffer = NULL
};

static twr_led_strip_t floor_1;

static const twr_led_strip_buffer_t floor_2_led_strip_buffer =
{
    .type = LED_STRIP_TYPE,
    .count = FLOOR_2,
    .buffer = NULL
};

static twr_led_strip_t floor_2;

static twr_scheduler_task_id_t led_strip_write_task_id;

void led_strip_write_task(void *param)
{
    (void) param;

    if (!twr_led_strip_write(&full_led_strip))
    {
        twr_scheduler_plan_current_now();
    }
}

bool driver_led_strip_init(const twr_led_strip_buffer_t *led_strip)
{
    (void) led_strip;

    return true;
}

bool driver_led_strip_write(void)
{
    twr_scheduler_plan_now(led_strip_write_task_id);

    return true;
}

static const twr_led_strip_driver_t floor_1_led_strip_driver =
{
    .init = driver_led_strip_init,
    .write = driver_led_strip_write,
    .set_pixel = twr_ws2812b_set_pixel_from_uint32,
    .set_pixel_rgbw = twr_ws2812b_set_pixel_from_rgb,
    .is_ready = twr_ws2812b_is_ready
};

static void driver_floor_2_set_pixel_from_uint32(int position, uint32_t color)
{
    twr_ws2812b_set_pixel_from_uint32(position + FLOOR_1, color);
}

static void driver_floor_2_set_pixel_from_rgb(int position, uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
{
    twr_ws2812b_set_pixel_from_rgb(position + FLOOR_1, red, green, blue, white);
}

static const twr_led_strip_driver_t floor_2_led_strip_driver =
{
    .init = driver_led_strip_init,
    .write = driver_led_strip_write,
    .set_pixel = driver_floor_2_set_pixel_from_uint32,
    .set_pixel_rgbw = driver_floor_2_set_pixel_from_rgb,
    .is_ready = twr_ws2812b_is_ready
};

void window_led_strip_init(void)
{
    twr_led_strip_init(&full_led_strip, twr_module_power_get_led_strip_driver(), &led_strip_buffer);

    twr_led_strip_init(&floor_1, &floor_1_led_strip_driver, &floor_1_led_strip_buffer);

    twr_led_strip_init(&floor_2, &floor_2_led_strip_driver, &floor_2_led_strip_buffer);

    led_strip_write_task_id = twr_scheduler_register(led_strip_write_task, NULL, 10);

    twr_led_strip_fill(&full_led_strip, 0);

    // twr_led_strip_write(&full_led_strip);

    // twr_led_strip_effect_rainbow(&full_led_strip, 50);
}

twr_led_strip_t *window_led_strip_get_full(void)
{
    return &full_led_strip;
}

twr_led_strip_t *window_led_strip_get_floor1(void)
{
    return &floor_1;
}

twr_led_strip_t *window_led_strip_get_floor2(void)
{
    return &floor_2;
}
