// FULL WORKING main.c WITH UPDATED MODES
// Mode 1: Rainbow Wave
// Mode 2: Ocean Wave
// Mode 3: Pulse Wave
// Mode 4: Sparkles (sound reactive on W104 DO)

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include <math.h>
#include <stdlib.h>

#define LED_PIN      2
#define LED_COUNT    72
#define IS_RGBW      false
#define BUTTON_PIN   10
#define SOUND_PIN    45
#define BRIGHTNESS   50

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

static inline void put_pixel_scaled(PIO pio, uint sm, uint8_t r, uint8_t g, uint8_t b) {
    apply_brightness(&r, &g, &b);
    pio_sm_put_blocking(pio, sm, urgb_u32(r, g, b));
}

// Rainbow helper
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
// MODE 2 — Ocean Wave
// -----------------------------
static void mode_ocean_wave(PIO pio, uint sm) {
    static float t = 0;
    t += 0.08f;

    for (int i = 0; i < LED_COUNT; i++) {
        float wave = (sinf(t + (i * 0.25f)) + 1.0f) * 0.5f;

        uint8_t r = 0;
        uint8_t g = (uint8_t)(50 + wave * 150);
        uint8_t b = (uint8_t)(100 + wave * 155);

        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// -----------------------------
// MODE 3 — Pulse Wave
// -----------------------------
static void mode_pulse_wave(PIO pio, uint sm) {
    static float t = 0;
    t += 0.10f;

    for (int i = 0; i < LED_COUNT; i++) {
        float wave = (sinf(t - i * 0.15f) + 1.0f) * 0.5f;

        uint8_t r = (uint8_t)(wave * 255);
        uint8_t g = (uint8_t)(wave * 80);
        uint8_t b = (uint8_t)(wave * 20);

        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// -----------------------------
// MODE 4 — Sound Reactive Sparkles
// -----------------------------
static void mode_sound(PIO pio, uint sm) {
    static uint8_t sr[LED_COUNT] = {0};
    static uint8_t sg[LED_COUNT] = {0};
    static uint8_t sb[LED_COUNT] = {0};

    bool loud = (gpio_get(SOUND_PIN) == 0);

    if (loud) {
        for (int i = 0; i < 10; i++) {
            int index = rand() % LED_COUNT;
            sr[index] = 255;
            sg[index] = 255;
            sb[index] = 255;
        }
    } else {
        if (rand() % 25 == 0) {
            int index = rand() % LED_COUNT;
            sr[index] = 100;
            sg[index] = 100;
            sb[index] = 150;
        }
    }

    for (int i = 0; i < LED_COUNT; i++) {
        if (sr[i] > 2) sr[i] -= 2;
        if (sg[i] > 2) sg[i] -= 2;
        if (sb[i] > 2) sb[i] -= 2;

        put_pixel_scaled(pio, sm, sr[i], sg[i], sb[i]);
    }
}

// -----------------------------
// MAIN
// -----------------------------
int main() {
    stdio_init_all();

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, IS_RGBW);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_init(SOUND_PIN);
    gpio_set_dir(SOUND_PIN, GPIO_IN);
    gpio_pull_up(SOUND_PIN);

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

        if (mode == 0)      mode_rainbow_wave(pio, sm);
        else if (mode == 1) mode_ocean_wave(pio, sm);
        else if (mode == 2) mode_pulse_wave(pio, sm);
        else if (mode == 3) mode_sound(pio, sm);

        sleep_ms(20);
    }
}