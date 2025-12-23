#ifndef PS2K_HPP
#define PS2K_HPP 1

#include <cstdint>
#include <cstddef>

#include "ps2k.hpp"
#include <panic.hpp>
#include <arch/arch.hpp>
#include <mem/mem.hpp>

namespace driver::input::ps2k {

void initialise();
size_t read(size_t n, void* out, bool canonical);

}

#endif
