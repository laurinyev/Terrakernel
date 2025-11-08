#include <drivers/serial/serial.hpp>

extern "C" void init() {
	serial::serial_putc('H');
	serial::serial_putc('i');

	asm volatile ("sti");
	while (1) {
		asm volatile ("hlt;");
	}
	__builtin_unreachable();
}
