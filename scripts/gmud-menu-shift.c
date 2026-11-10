/* Portable GBDK implementations of the missing GMUD_GB HOME routines. */
#include <stdint.h>
#include <string.h>

#define FB_STRIDE 20

extern uint8_t *menu_shift_dst;
extern const uint8_t *menu_shift_src;
extern uint8_t menu_shift_width;
extern uint8_t menu_shift_rows;

void menu_shift_copy_up(void)
{
    for (uint8_t row = 0; row < menu_shift_rows; ++row) {
        uint16_t offset = (uint16_t)row * FB_STRIDE;
        memcpy(menu_shift_dst + offset, menu_shift_src + offset, menu_shift_width);
    }
}

void menu_shift_copy_down(void)
{
    for (uint8_t row = 0; row < menu_shift_rows; ++row) {
        uint16_t offset = (uint16_t)row * FB_STRIDE;
        memcpy(menu_shift_dst - offset, menu_shift_src - offset, menu_shift_width);
    }
}

void menu_shift_clear(void)
{
    for (uint8_t row = 0; row < menu_shift_rows; ++row) {
        uint16_t offset = (uint16_t)row * FB_STRIDE;
        memset(menu_shift_dst + offset, 0, menu_shift_width);
    }
}
