#include <application.h>
#include <window_led_strip.h>
#include <at.h>

#define SEND_DATA_INTERVAL        (15 * 60 * 1000)
#define MEASURE_INTERVAL               (30 * 1000)

// LED instance
twr_led_t led;
twr_led_t leds[4];
// Button instance
twr_button_t button;
// Lora instance
twr_cmwx1zzabz_t lora;
// Thermometer instance
twr_tmp112_t tmp112;
// Humidity tag instance;
twr_tag_humidity_t humidity_tag;
// Barometer tag instance
twr_tag_barometer_t barometer_tag;

twr_led_strip_t *full;
twr_led_strip_t *floor_1;
twr_led_strip_t *floor_2;

bool radio_pairing_mode = false;

enum {
    HEADER_BOOT         = 0x00,
    HEADER_UPDATE       = 0x01,
    HEADER_BUTTON_CLICK = 0x02,
    HEADER_BUTTON_HOLD  = 0x03,

} header = HEADER_BOOT;

TWR_DATA_STREAM_FLOAT_BUFFER(sm_temperature_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
TWR_DATA_STREAM_FLOAT_BUFFER(sm_humidity_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
TWR_DATA_STREAM_FLOAT_BUFFER(sm_pressure_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))

twr_data_stream_t sm_temperature;
twr_data_stream_t sm_humidity;
twr_data_stream_t sm_pressure;

int mode = 0;

void all_led_set_mode(twr_led_mode_t mode)
{
    for (int i = 0; i < 4; i++)
    {
        twr_led_set_mode(&leds[i], mode);
    }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    if (event == TWR_TMP112_EVENT_UPDATE)
    {
        float value;

        twr_tmp112_get_temperature_celsius(self, &value);

        twr_data_stream_feed(&sm_temperature, &value);
    }
}

void humidity_tag_event_handler(twr_tag_humidity_t *self, twr_tag_humidity_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    if (event == TWR_TAG_HUMIDITY_EVENT_UPDATE)
    {
        float value;

        twr_tag_humidity_get_humidity_percentage(self, &value);

        twr_data_stream_feed(&sm_humidity, &value);
    }
}

void barometer_tag_event_handler(twr_tag_barometer_t *self, twr_tag_barometer_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    if (event == TWR_TAG_BAROMETER_EVENT_UPDATE)
    {
        float value;

        twr_tag_barometer_get_pressure_pascal(self, &value);

        twr_data_stream_feed(&sm_pressure, &value);
    }
}

void lora_callback(twr_cmwx1zzabz_t *self, twr_cmwx1zzabz_event_t event, void *event_param)
{
    if (event == TWR_CMWX1ZZABZ_EVENT_ERROR)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_BLINK_FAST);

        all_led_set_mode(TWR_LED_MODE_BLINK_FAST);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_ON);

        all_led_set_mode(TWR_LED_MODE_ON);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);

        all_led_set_mode(TWR_LED_MODE_BLINK_SLOW);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_READY)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);

        all_led_set_mode(TWR_LED_MODE_BLINK_SLOW);
    }
}

bool at_send(void)
{
    twr_scheduler_plan_now(0);

    return true;
}

bool at_status(void)
{
float value_avg = NAN;

    static const struct {
        twr_data_stream_t *stream;
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

        if (twr_data_stream_get_average(values[i].stream, &value_avg))
        {
            twr_atci_printf("$STATUS: \"%s\",%.*f", values[i].name, values[i].precision, value_avg);
        }
        else
        {
            twr_atci_printf("$STATUS: \"%s\",", values[i].name);
        }
    }

    return true;
}

void change_mode(void)
{
    switch (++mode) {
        case 1:
        {
            twr_led_strip_effect_rainbow(full, 50);
            break;
        }
        case 2:
        {
            twr_led_strip_effect_stop(full);
            twr_led_strip_effect_rainbow(floor_1, 50);
            twr_led_strip_effect_rainbow_cycle(floor_2, 50);
            break;
        }
        case 3:
        {
            twr_led_strip_effect_rainbow(floor_2, 50);
            twr_led_strip_effect_rainbow_cycle(floor_1, 50);
            break;
        }
        case 4:
        {
            twr_led_strip_effect_rainbow(floor_1, 50);
            twr_led_strip_effect_icicle(floor_2, 0x00aa00, 20);
            break;
        }
        case 5:
        {
            twr_led_strip_effect_rainbow(floor_1, 50);
            twr_led_strip_effect_icicle(floor_2, 0x00aa00, 20);
            break;
        }
        case 6:
        {
            twr_led_strip_effect_rainbow(floor_1, 50);
            twr_led_strip_effect_pulse_color(floor_2, 0xff000000, 20);
            break;
        }
        case 7:
        {
            twr_led_strip_effect_theater_chase_rainbow(floor_1, 50);
            twr_led_strip_effect_stroboscope(floor_2, 0x0000ff00, 20);
            break;
        }
        default:
        {
            mode = 0;
            twr_led_strip_effect_stop(full);
            twr_led_strip_effect_stop(floor_1);
            twr_led_strip_effect_stop(floor_2);
            twr_led_strip_fill(full, 0);
            twr_led_strip_write(floor_1);
            break;
        }
    }
}

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == TWR_BUTTON_EVENT_CLICK)
    {
        header = HEADER_BUTTON_CLICK;

        twr_scheduler_plan_now(0);

        change_mode();
    }
    else if (event == TWR_BUTTON_EVENT_HOLD)
    {
        header = HEADER_BUTTON_HOLD;

        twr_scheduler_plan_now(0);
    }
}


void application_init(void)
{
    twr_data_stream_init(&sm_temperature, 1, &sm_temperature_buffer);
    twr_data_stream_init(&sm_humidity, 1, &sm_humidity_buffer);
    twr_data_stream_init(&sm_pressure, 1, &sm_pressure_buffer);

    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_init(&leds[0], TWR_GPIO_P17, false, false);
    twr_led_init(&leds[1], TWR_GPIO_P15, false, false);
    twr_led_init(&leds[2], TWR_GPIO_P14, false, false);
    twr_led_init(&leds[3], TWR_GPIO_P12, false, false);

    all_led_set_mode(TWR_LED_MODE_BLINK_SLOW);

    window_led_strip_init();
    full = window_led_strip_get_full();
    floor_1 = window_led_strip_get_floor1();
    floor_2 = window_led_strip_get_floor2();

    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, false);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize Thermometer
    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, MEASURE_INTERVAL);

    twr_tag_humidity_init(&humidity_tag, TWR_TAG_HUMIDITY_REVISION_R3, TWR_I2C_I2C0, TWR_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    twr_tag_humidity_set_update_interval(&humidity_tag, 1000);
    twr_tag_humidity_set_event_handler(&humidity_tag, humidity_tag_event_handler, NULL);

    twr_tag_barometer_init(&barometer_tag, TWR_I2C_I2C0);
    twr_tag_barometer_set_update_interval(&barometer_tag, 1000);
    twr_tag_barometer_set_event_handler(&barometer_tag, barometer_tag_event_handler, NULL);

    // Initialize lora module
    twr_cmwx1zzabz_init(&lora, TWR_UART_UART1);
    twr_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    twr_cmwx1zzabz_set_class(&lora, TWR_CMWX1ZZABZ_CONFIG_CLASS_C);

    // Initialize AT command interface
    at_init(&led, &lora);
    static const twr_atci_command_t commands[] = {
            AT_LORA_COMMANDS,
            {"$SEND", at_send, NULL, NULL, NULL, "Immediately send packet"},
            {"$STATUS", at_status, NULL, NULL, NULL, "Show status"},
            AT_LED_COMMANDS,
            TWR_ATCI_COMMAND_CLAC,
            TWR_ATCI_COMMAND_HELP
    };
    twr_atci_init(commands, TWR_ATCI_COMMANDS_LENGTH(commands));

    header = HEADER_BOOT;

    twr_atci_printf("$BOOT");

    twr_led_pulse(&led, 2000);

    change_mode();

    twr_scheduler_plan_current_relative(10 * 1000);
}

void application_task(void)
{
    if (!twr_cmwx1zzabz_is_ready(&lora))
    {
        twr_scheduler_plan_current_relative(100);

        return;
    }

    static uint8_t buffer[6];

    memset(buffer, 0xff, sizeof(buffer));

    buffer[0] = header;

    float temperature_avg = NAN;

    twr_data_stream_get_average(&sm_temperature, &temperature_avg);

    if (!isnan(temperature_avg))
    {
        int16_t temperature_i16 = (int16_t) (temperature_avg * 10.f);

        buffer[1] = temperature_i16 >> 8;
        buffer[2] = temperature_i16;
    }

    float humidity_avg = NAN;

    twr_data_stream_get_average(&sm_humidity, &humidity_avg);

    if (!isnan(humidity_avg))
    {
        buffer[3] = humidity_avg * 2;
    }

    float pressure_avg = NAN;

    twr_data_stream_get_average(&sm_pressure, &pressure_avg);

    if (!isnan(pressure_avg))
    {
        uint16_t value = pressure_avg / 2.f;
        buffer[4] = value >> 8;
        buffer[5] = value;
    }

    twr_cmwx1zzabz_send_message(&lora, buffer, sizeof(buffer));

    static char tmp[sizeof(buffer) * 2 + 1];
    for (size_t i = 0; i < sizeof(buffer); i++)
    {
        sprintf(tmp + i * 2, "%02x", buffer[i]);
    }

    twr_atci_printf("$SEND: %s", tmp);

    header = HEADER_UPDATE;

    twr_scheduler_plan_current_relative(SEND_DATA_INTERVAL);
}
