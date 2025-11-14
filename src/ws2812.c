#include "ws2812.pio.h"
#include "hardware/pio.h"

void put_pixel(PIO pio, uint sm, uint32_t color) {
    pio_sm_put_blocking(pio, sm, color << 8u);
}
