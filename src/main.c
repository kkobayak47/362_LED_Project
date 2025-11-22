// FULL WORKING main.c (RP2350 / Proton)
// LED_PIN = GP2, 72 WS2812 LEDs, BRIGHTNESS=50
// BUTTON_PIN = GP10 cycles modes
// MAX4466 OUT on GP46 using FREE-RUNNING ADC channel 5

#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "ws2812.pio.h"

#define LED_PIN      2
#define LED_COUNT    72
#define IS_RGBW      false
#define BUTTON_PIN   10
#define BRIGHTNESS   50

// MAX4466 wiring: OUT -> GP46, mapped to ADC input 5
#define MIC_GPIO     46
#define MIC_ADC_CH   5

// -----------------------------
// WS2812 Helper Functions
// -----------------------------
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
}

static inline void apply_brightness(uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (*r * BRIGHTNESS) / 255;
    *g = (*g * BRIGHTNESS) / 255;
    *b = (*b * BRIGHTNESS) / 255;
}

static inline void put_pixel_scaled(PIO pio, uint sm,
                                    uint8_t r, uint8_t g, uint8_t b) {
    apply_brightness(&r, &g, &b);
    pio_sm_put_blocking(pio, sm, urgb_u32(r, g, b));
}

static void color_wheel(uint8_t pos, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (pos < 85) {
        *r = 255 - pos * 3;
        *g = pos * 3;
        *b = 0;
    } else if (pos < 170) {
        pos -= 85;
        *r = 0;
        *g = 255 - pos * 3;
        *b = pos * 3;
    } else {
        pos -= 170;
        *r = pos * 3;
        *g = 0;
        *b = 255 - pos * 3;
    }
}

// -----------------------------
// MODE 1 — Rainbow Wave
// -----------------------------
static void mode_rainbow_wave(PIO pio, uint sm) {
    static uint8_t t = 0;
    t++;

    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t r, g, b;
        color_wheel(t + (i * 256 / LED_COUNT), &r, &g, &b);
        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// -----------------------------
// MODE 2 — Rainbow VU Meter
// -----------------------------
static void mode_rainbow_vu(PIO pio, uint sm) {
    static float smooth = 0;

    uint16_t raw = adc_hw->result;
    int centered = (int)raw - 1800;
    if (centered < 0) centered = -centered;

    smooth = smooth * 0.85f + centered * 0.15f;

    int level = (int)(smooth * LED_COUNT / 800.0f);
    if (level < 0) level = 0;
    if (level > LED_COUNT) level = LED_COUNT;

    for (int i = 0; i < LED_COUNT; i++) {
        if (i < level) {
            uint8_t r, g, b;
            color_wheel((i * 256 / LED_COUNT) & 0xFF, &r, &g, &b);
            put_pixel_scaled(pio, sm, r, g, b);
        } else {
            put_pixel_scaled(pio, sm, 0, 0, 0);
        }
    }
}

// -----------------------------
// MODE 3 — Volume Heatmap (blue→red)
// -----------------------------
static void mode_heat(PIO pio, uint sm) {
    uint16_t raw = adc_hw->result;
    int centered = (int)raw - 1800;
    if (centered < 0) centered = -centered;

    float level = centered / 1200.0f;
    if (level < 0) level = 0;
    if (level > 1) level = 1;

    uint8_t r = (uint8_t)(level * 255);
    uint8_t g = (uint8_t)(level * 180);
    uint8_t b = (uint8_t)((1 - level) * 120);

    for (int i = 0; i < LED_COUNT; i++)
        put_pixel_scaled(pio, sm, r, g, b);
}

// -----------------------------
// MAIN
// -----------------------------
int main() {
    stdio_init_all();
    srand((unsigned) time_us_32());

    // LED Init
    PIO pio = pio0;
    const uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, IS_RGBW);

    // Button Init
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // ADC Free-Run Init
    adc_init();
    adc_hw->cs = ADC_CS_EN_BITS;
    gpio_set_function(MIC_GPIO, GPIO_FUNC_NULL);
    gpio_disable_pulls(MIC_GPIO);
    gpio_set_input_enabled(MIC_GPIO, false);

    hw_write_masked(&adc_hw->cs,
                    MIC_ADC_CH << ADC_CS_AINSEL_LSB,
                    ADC_CS_AINSEL_BITS);

    hw_set_bits(&adc_hw->cs, ADC_CS_START_MANY_BITS);

    int mode = 0;

    while (true) {
        if (!gpio_get(BUTTON_PIN)) {
            sleep_ms(40);
            if (!gpio_get(BUTTON_PIN)) {
                mode = (mode + 1) % 3;  // 0-2 only
                while (!gpio_get(BUTTON_PIN)) sleep_ms(10);
            }
        }

        if (mode == 0)
            mode_rainbow_wave(pio, sm);
        else if (mode == 1)
            mode_rainbow_vu(pio, sm);
        else if (mode == 2)
            mode_heat(pio, sm);

        sleep_ms(20);
    }
}
