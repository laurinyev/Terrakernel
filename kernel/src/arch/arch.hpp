#ifndef ARCH_HPP
#define ARCH_HPP 1

#include <cstdint>

namespace arch {
namespace x86_64 {
namespace io {
	void outb(uint16_t port, uint8_t byte);
}
}
}

#endif /* ARCH_HPP */
