#include "../PS2Keyboard.h"

void ps2_keyboard_handler(uint8_t scancode) {
    outb(0x3F8, 'K');
}
