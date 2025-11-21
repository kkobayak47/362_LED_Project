// Updated main.c with Mode 2 changed to Ocean Wave
// (Full implementation will need previous code context; placeholder structure below)

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include <math.h>

#define LED_PIN      2
#define LED_COUNT    72
#define IS_RGBW      false
#define BUTTON_PIN   10
#define SOUND_PIN    45
#define BRIGHTNESS   50

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
}

static inline void apply_brightness(uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (*r * BRIGHTNESS) / 255;
    *g = (*g * BRIGHTNESS) / 255;
    *b = (*b * BRIGHTNESS) / 255;
}

static inline void put_pixel_scaled(PIO pio, uint sm, uint8_t r, uint8_t g, uint8_t b) {
    apply_brightness(&r, &g, &b);
    pio_sm_put_blocking(pio, sm, urgb_u32(r, g, b));
}

// Rainbow helper (unchanged)
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

// MODE 1 (unchanged) - Rainbow Wave
static void mode_rainbow_wave(PIO pio, uint sm) {
    static uint8_t t = 0;
    t++;
    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t r, g, b;
        color_wheel(t + (i * 256 / LED_COUNT), &r, &g, &b);
        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// MODE 2 - NEW OCEAN WAVE MODE
static void mode_ocean_wave(PIO pio, uint sm) {
    static float t = 0;
    t += 0.08f; // wave speed

    for (int i = 0; i < LED_COUNT; i++) {
        float wave = (sinf(t + (i * 0.25f)) + 1.0f) * 0.5f; // 0–1 smooth wave

        // Ocean colors (deep blue → aqua → light blue)
        uint8_t r = 0;
        uint8_t g = (uint8_t)(50 + wave * 150);
        uint8_t b = (uint8_t)(100 + wave * 155);

        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// MODE 3 — Pulse Wave
static void mode_pulse_wave(PIO pio, uint sm) {
    static float t = 0;
    t += 0.10f; // speed of pulse

    for (int i = 0; i < LED_COUNT; i++) {
        float wave = (sinf(t - i * 0.15f) + 1.0f) * 0.5f; // 0–1

        uint8_t r = (uint8_t)(wave * 255);
        uint8_t g = (uint8_t)(wave * 80);
        uint8_t b = (uint8_t)(wave * 20);

        put_pixel_scaled(pio, sm, r, g, b);
    }
}
    else if (pos >= LED_COUNT - 1) { pos = LED_COUNT - 1; dir = -1; }

    for (int i = 0; i < LED_COUNT; i++) {
        int dist = abs(pos - i);
        if (dist >= tail_len) {
            put_pixel_scaled(pio, sm, 0, 0, 0);
            continue;
        }
        int factor = tail_len - dist;

        uint8_t r, g, b;
        color_wheel((pos * 4) & 0xFF, &r, &g, &b);

        r = (r * factor) / tail_len;
        g = (g * factor) / tail_len;
        b = (b * factor) / tail_len;

        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// MODE 4 (unchanged placeholder) - Sound Sparkles
static void mode_sound(PIO pio, uint sm) {
    // placeholder sparkle logic
    for (int i = 0; i < LED_COUNT; i++) {
        int sparkle = rand() % 100;
        if (sparkle < 3) put_pixel_scaled(pio, sm, 255, 255, 255);
        else put_pixel_scaled(pio, sm, 0, 0, 20);
    }
}

int main() {
    stdio_init_all();

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, IS_RGBW);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    int mode = 0;

    while (true) {
        if (!gpio_get(BUTTON_PIN)) {
            sleep_ms(40);
            if (!gpio_get(BUTTON_PIN)) {
                mode++;
                if (mode > 3) mode = 0;
                while (!gpio_get(BUTTON_PIN)) sleep_ms(10);
            }
        }

        if (mode == 0) mode_rainbow_wave(pio, sm);
        else if (mode == 1) mode_ocean_wave(pio, sm);
        else if (mode == 2) mode_pulse_wave(pio, sm)
        else if (mode == 3) mode_sound(pio, sm);

        sleep_ms(20);
    }
}
