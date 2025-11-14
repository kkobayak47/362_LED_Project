#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"

#define LED_PIN 2
#define LED_COUNT 8
#define IS_RGBW false

int main() {
    stdio_init_all();

    PIO pio = pio0;
    const uint sm = 0;

    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_PIN, 800000, IS_RGBW);

    while (true) {
        uint32_t red = 0x00FF0000;  // RGB format (grb shifted in .pio.h)
        pio_sm_put_blocking(pio, sm, red >> 8);
        sleep_ms(200);
    }
}
