#include <application.h>
#include <bc_config.h>
#include <window_led_strip.h>
#include <at.h>
#include <bc_ws2812b.h>

#define SEND_DATA_INTERVAL        (15 * 60 * 1000)
#define MEASURE_INTERVAL               (30 * 1000)

// LED instance
bc_led_t led;
bc_led_t leds[4];
// Button instance
bc_button_t button;
// Lora instance
bc_cmwx1zzabz_t lora;
// Thermometer instance
bc_tmp112_t tmp112;
// Humidity tag instance;
bc_tag_humidity_t humidity_tag;
// Barometer tag instance
bc_tag_barometer_t barometer_tag;

bc_led_strip_t *full;
bc_led_strip_t *floor_1;
bc_led_strip_t *floor_2;

bool radio_pairing_mode = false;

enum {
    HEADER_BOOT         = 0x00,
    HEADER_UPDATE       = 0x01,
    HEADER_BUTTON_CLICK = 0x02,
    HEADER_BUTTON_HOLD  = 0x03,

} header = HEADER_BOOT;

BC_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_humidity_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
BC_DATA_STREAM_FLOAT_BUFFER(sm_pressure_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))

bc_data_stream_t sm_temperature;
bc_data_stream_t sm_humidity;
bc_data_stream_t sm_pressure;

int mode = 0;

void all_led_set_mode(bc_led_mode_t mode)
{
    for (int i = 0; i < 4; i++)
    {
        bc_led_set_mode(&leds[i], mode);
    }
}

void tmp112_event_handler(bc_tmp112_t *self, bc_tmp112_event_t event, void *event_param)
{
    if (event == BC_TMP112_EVENT_UPDATE)
    {
        float value;

        bc_tmp112_get_temperature_celsius(self, &value);

        bc_data_stream_feed(&sm_temperature, &value);
    }
}

void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    if (event == BC_TAG_HUMIDITY_EVENT_UPDATE)
    {
        float value;

        bc_tag_humidity_get_humidity_percentage(self, &value);

        bc_data_stream_feed(&sm_humidity, &value);
    }
}

void barometer_tag_event_handler(bc_tag_barometer_t *self, bc_tag_barometer_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    if (event == BC_TAG_BAROMETER_EVENT_UPDATE)
    {
        float value;

        bc_tag_barometer_get_pressure_pascal(self, &value);

        bc_data_stream_feed(&sm_pressure, &value);
    }
}

void lora_callback(bc_cmwx1zzabz_t *self, bc_cmwx1zzabz_event_t event, void *event_param)
{
    if (event == BC_CMWX1ZZABZ_EVENT_ERROR)
    {
        bc_led_set_mode(&led, BC_LED_MODE_BLINK_FAST);

        all_led_set_mode(BC_LED_MODE_BLINK_FAST);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        bc_led_set_mode(&led, BC_LED_MODE_ON);

        all_led_set_mode(BC_LED_MODE_ON);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);

        all_led_set_mode(BC_LED_MODE_BLINK_SLOW);
    }
    else if (event == BC_CMWX1ZZABZ_EVENT_READY)
    {
        bc_led_set_mode(&led, BC_LED_MODE_OFF);

        all_led_set_mode(BC_LED_MODE_BLINK_SLOW);
    }
}

bool at_send(void)
{
    bc_scheduler_plan_now(0);

    return true;
}

bool at_status(void)
{
float value_avg = NAN;

    static const struct {
        bc_data_stream_t *stream;
        const char *name;
        int precision;
    } values[] = {
        {&sm_temperature, "Temperature", 1},
        {&sm_humidity, "Humidity", 1},
        {&sm_pressure, "Pressure", 0},
    };

    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
        value_avg = NAN;

        if (bc_data_stream_get_average(values[i].stream, &value_avg))
        {
            bc_atci_printf("$STATUS: \"%s\",%.*f", values[i].name, values[i].precision, value_avg);
        }
        else
        {
            bc_atci_printf("$STATUS: \"%s\",", values[i].name);
        }
    }

    return true;
}

void change_mode(void)
{
    switch (++mode) {
        case 1:
        {
            bc_led_strip_effect_rainbow(full, 50);
            break;
        }
        case 2:
        {
            bc_led_strip_effect_stop(full);
            bc_led_strip_effect_rainbow(floor_1, 50);
            bc_led_strip_effect_rainbow_cycle(floor_2, 50);
            break;
        }
        case 3:
        {
            bc_led_strip_effect_rainbow(floor_2, 50);
            bc_led_strip_effect_rainbow_cycle(floor_1, 50);
            break;
        }
        case 4:
        {
            bc_led_strip_effect_rainbow(floor_1, 50);
            bc_led_strip_effect_icicle(floor_2, 0x00aa00, 20);
            break;
        }
        case 5:
        {
            bc_led_strip_effect_rainbow(floor_1, 50);
            bc_led_strip_effect_icicle(floor_2, 0x00aa00, 20);
            break;
        }
        case 6:
        {
            bc_led_strip_effect_rainbow(floor_1, 50);
            bc_led_strip_effect_pulse_color(floor_2, 0xff000000, 20);
            break;
        }
        case 7:
        {
            bc_led_strip_effect_theater_chase_rainbow(floor_1, 50);
            bc_led_strip_effect_stroboscope(floor_2, 0x0000ff00, 20);
            break;
        }
        default:
        {
            mode = 0;
            bc_led_strip_effect_stop(full);
            bc_led_strip_effect_stop(floor_1);
            bc_led_strip_effect_stop(floor_2);
            bc_led_strip_fill(full, 0);
            bc_led_strip_write(floor_1);
            break;
        }
    }
}

void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_BUTTON_EVENT_CLICK)
    {
        header = HEADER_BUTTON_CLICK;

        bc_scheduler_plan_now(0);

        change_mode();
    }
    else if (event == BC_BUTTON_EVENT_HOLD)
    {
        header = HEADER_BUTTON_HOLD;

        bc_scheduler_plan_now(0);
    }
}


void application_init(void)
{
    bc_data_stream_init(&sm_temperature, 1, &sm_temperature_buffer);
    bc_data_stream_init(&sm_humidity, 1, &sm_humidity_buffer);
    bc_data_stream_init(&sm_pressure, 1, &sm_pressure_buffer);

    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_init(&leds[0], BC_GPIO_P17, false, false);
    bc_led_init(&leds[1], BC_GPIO_P15, false, false);
    bc_led_init(&leds[2], BC_GPIO_P14, false, false);
    bc_led_init(&leds[3], BC_GPIO_P12, false, false);

    all_led_set_mode(BC_LED_MODE_BLINK_SLOW);

    window_led_strip_init();
    full = window_led_strip_get_full();
    floor_1 = window_led_strip_get_floor1();
    floor_2 = window_led_strip_get_floor2();

    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, false);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize Thermometer
    bc_tmp112_init(&tmp112, BC_I2C_I2C0, 0x49);
    bc_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    bc_tmp112_set_update_interval(&tmp112, MEASURE_INTERVAL);

    bc_tag_humidity_init(&humidity_tag, BC_TAG_HUMIDITY_REVISION_R3, BC_I2C_I2C0, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity_tag, 1000);
    bc_tag_humidity_set_event_handler(&humidity_tag, humidity_tag_event_handler, NULL);

    bc_tag_barometer_init(&barometer_tag, BC_I2C_I2C0);
    bc_tag_barometer_set_update_interval(&barometer_tag, 1000);
    bc_tag_barometer_set_event_handler(&barometer_tag, barometer_tag_event_handler, NULL);

    // Initialize lora module
    bc_cmwx1zzabz_init(&lora, BC_UART_UART1);
    bc_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    bc_cmwx1zzabz_set_class(&lora, BC_CMWX1ZZABZ_CONFIG_CLASS_C);

    // Initialize AT command interface
    at_init(&led, &lora);
    static const bc_atci_command_t commands[] = {
            AT_LORA_COMMANDS,
            {"$SEND", at_send, NULL, NULL, NULL, "Immediately send packet"},
            {"$STATUS", at_status, NULL, NULL, NULL, "Show status"},
            AT_LED_COMMANDS,
            BC_ATCI_COMMAND_CLAC,
            BC_ATCI_COMMAND_HELP
    };
    bc_atci_init(commands, BC_ATCI_COMMANDS_LENGTH(commands));

    header = HEADER_BOOT;

    bc_atci_printf("$BOOT");

    bc_led_pulse(&led, 2000);

    change_mode();

    bc_scheduler_plan_current_relative(10 * 1000);
}

void application_task(void)
{
    if (!bc_cmwx1zzabz_is_ready(&lora))
    {
        bc_scheduler_plan_current_relative(100);

        return;
    }

    static uint8_t buffer[6];

    memset(buffer, 0xff, sizeof(buffer));

    buffer[0] = header;

    float temperature_avg = NAN;

    bc_data_stream_get_average(&sm_temperature, &temperature_avg);

    if (!isnan(temperature_avg))
    {
        int16_t temperature_i16 = (int16_t) (temperature_avg * 10.f);

        buffer[1] = temperature_i16 >> 8;
        buffer[2] = temperature_i16;
    }

    float humidity_avg = NAN;

    bc_data_stream_get_average(&sm_humidity, &humidity_avg);

    if (!isnan(humidity_avg))
    {
        buffer[3] = humidity_avg * 2;
    }

    float pressure_avg = NAN;

    bc_data_stream_get_average(&sm_pressure, &pressure_avg);

    if (!isnan(pressure_avg))
    {
        uint16_t value = pressure_avg / 2.f;
        buffer[4] = value >> 8;
        buffer[5] = value;
    }

    bc_cmwx1zzabz_send_message(&lora, buffer, sizeof(buffer));

    static char tmp[sizeof(buffer) * 2 + 1];
    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        sprintf(tmp + i * 2, "%02x", buffer[i]);
    }

    bc_atci_printf("$SEND: %s", tmp);

    header = HEADER_UPDATE;

    bc_scheduler_plan_current_relative(SEND_DATA_INTERVAL);
}
