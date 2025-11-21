#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include <math.h>

#define LED_PIN      2
#define LED_COUNT    72
#define IS_RGBW      false
#define BUTTON_PIN   10
#define SOUND_PIN    11      // W104 DO pin

// Safe brightness (USB) — 0–255
#define BRIGHTNESS   50

// -----------------------------------------------------------------------------
// Helper Functions
// -----------------------------------------------------------------------------

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 24) |
           ((uint32_t)r << 16) |
           ((uint32_t)b << 8);
}

// brightness limiter
static inline void apply_brightness(uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (*r * BRIGHTNESS) / 255;
    *g = (*g * BRIGHTNESS) / 255;
    *b = (*b * BRIGHTNESS) / 255;
}

static inline void put_pixel_scaled(PIO pio, uint sm,
                                    uint8_t r, uint8_t g, uint8_t b) {
    apply_brightness(&r, &g, &b);
    uint32_t packed = urgb_u32(r, g, b);
    pio_sm_put_blocking(pio, sm, packed);
}

// color wheel for rainbow
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

// -----------------------------------------------------------------------------
// MODE 1 — Smooth Rainbow Wave
// -----------------------------------------------------------------------------
static void mode_rainbow_wave(PIO pio, uint sm) {
    static uint8_t t = 0;
    t++;

    for (int i = 0; i < LED_COUNT; i++) {
        uint8_t idx = t + (i * 256 / LED_COUNT);
        uint8_t r, g, b;
        color_wheel(idx, &r, &g, &b);
        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// -----------------------------------------------------------------------------
// MODE 2 — Multi-Color Comet
// -----------------------------------------------------------------------------
static void mode_comet(PIO pio, uint sm) {
    static int head = 0;
    const int tail_len = 14;

    head = (head + 1) % LED_COUNT;

    for (int i = 0; i < LED_COUNT; i++) {
        int dist = abs(head - i);

        if (dist >= tail_len) {
            put_pixel_scaled(pio, sm, 0, 0, 0);
            continue;
        }

        int factor = tail_len - dist;

        uint8_t r, g, b;
        color_wheel((head * 4) & 0xFF, &r, &g, &b);

        r = (r * factor) / tail_len;
        g = (g * factor) / tail_len;
        b = (b * factor) / tail_len;

        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// -----------------------------------------------------------------------------
// MODE 3 — Rainbow Larson Scanner
// -----------------------------------------------------------------------------
static void mode_larson(PIO pio, uint sm) {
    static int pos = 0;
    static int dir = 1;
    const int tail_len = 12;

    pos += dir;
    if (pos <= 0) { pos = 0; dir = 1; }
    else if (pos >= LED_COUNT - 1) { pos = LED_COUNT - 1; dir = -1; }

    for (int i = 0; i < LED_COUNT; i++) {
        int dist = abs(pos - i);

        if (dist >= tail_len) {
            put_pixel_scaled(pio, sm, 0, 0, 0);
            continue;
        }

        int factor = tail_len - dist;

        uint8_t r, g, b;
        uint8_t idx = (pos * 4) & 0xFF;
        color_wheel(idx, &r, &g, &b);

        r = (r * factor) / tail_len;
        g = (g * factor) / tail_len;
        b = (b * factor) / tail_len;

        put_pixel_scaled(pio, sm, r, g, b);
    }
}

// -----------------------------------------------------------------------------
// MODE 4 — SOUND-REACTIVE FLASH (W104 DO pin)
// -----------------------------------------------------------------------------
static void mode_sound(PIO pio, uint sm) {
    bool loud = (gpio_get(SOUND_PIN) == 0);  // W104 DO is active LOW

    if (loud) {
        // flash white on sound event
        for (int i = 0; i < LED_COUNT; i++)
            put_pixel_scaled(pio, sm, 255, 255, 255);

    } else {
        // quiet → low glow
        for (int i = 0; i < LED_COUNT; i++)
            put_pixel_scaled(pio, sm, 10, 10, 10);
    }
}

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------
int main() {
    stdio_init_all();

    // LED init
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, IS_RGBW);

    // Button init
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    // W104 DO pin
    gpio_init(SOUND_PIN);
    gpio_set_dir(SOUND_PIN, GPIO_IN);
    gpio_pull_up(SOUND_PIN);

    // Mode:
    // 0 = rainbow
    // 1 = comet
    // 2 = larson
    // 3 = sound reactive
    int mode = 0;

    while (true) {

        // ---- BUTTON CHECK ----
        if (!gpio_get(BUTTON_PIN)) {
            sleep_ms(40);
            if (!gpio_get(BUTTON_PIN)) {
                mode++;
                if (mode > 3) mode = 0;

                while (!gpio_get(BUTTON_PIN)) sleep_ms(10);
            }
        }

        // ---- RUN ACTIVE MODE ----
        if (mode == 0)      mode_rainbow_wave(pio, sm);
        else if (mode == 1) mode_comet(pio, sm);
        else if (mode == 2) mode_larson(pio, sm);
        else if (mode == 3) mode_sound(pio, sm);

        sleep_ms(20);  // animation speed
    }
}
