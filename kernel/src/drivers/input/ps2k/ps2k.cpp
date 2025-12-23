#include "ps2k.hpp"
#include <config.hpp>
#include <panic.hpp>
#include <arch/arch.hpp>
#include <mem/mem.hpp>

namespace driver::input::ps2k {

static char* __buf = nullptr;
static size_t buf_size = 0;
static volatile size_t head = 0;
static volatile size_t tail = 0;
static volatile bool locked = false;

static void lock() {
	while (locked) {
		asm("pause");
	}

	locked = true;

}
static void unlock() {
	locked = false;
}

static bool push_char(char c) {
    lock();
    size_t next = (head + 1) % buf_size;
    if (next == tail) { unlock(); return false; }
    __buf[head] = c;
    head = next;
    unlock();
    return true;
}

static bool pop_char(char &c) {
    lock();
    if (tail == head) { unlock(); return false; }
    c = __buf[tail];
    tail = (tail + 1) % buf_size;
    unlock();
    return true;
}

static void push_backspace() {
    lock();
    if (head != tail) head = (head + buf_size - 1) % buf_size;
    unlock();
}

static void flush() {
    lock();
    head = tail = 0;
    unlock();
}

char us_layout_normal[128] = {
    0,'`','1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,
    '\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' ',0
};

char us_layout_caps[128] = {
    0,'`','1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','[',']','\n',
    0,'A','S','D','F','G','H','J','K','L',';','\'',0,
    '\\','Z','X','C','V','B','N','M',',','.','/',0,
    '*',0,' ',0
};

char us_layout_shift[128] = {
    0,'~','!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','\"','`',0,
    '|','Z','X','C','V','B','N','M','<','>','?',0,
    '*',0,' ',0
};

bool modifiers[9] = { false };
bool extended = false;

#define LEFT_CTL   0
#define LEFT_SHFT  1
#define RIGHT_SHFT 2
#define LEFT_ALT   3
#define CAPS_LOCK  4
#define NUM_LOCK   5
#define SCROLL_LOCK 6
#define RIGHT_CTL  7
#define RIGHT_ALT  8

__attribute__((interrupt))
void ps2k_interrupt_handler(void*) {
    uint8_t data = arch::x86_64::io::inb(0x60);
    bool released = data & 0x80;
    uint8_t key = data & 0x7F;

    char n = us_layout_normal[key];
    char c = us_layout_caps[key];
    char s = us_layout_shift[key];

    if (data == 0xE0) { extended = true; goto end; }

    if (!extended) {
        switch (key) {
            case 0x1D: modifiers[LEFT_CTL] = !released; break;
            case 0x2A: modifiers[LEFT_SHFT] = !released; break;
            case 0x36: modifiers[RIGHT_SHFT] = !released; break;
            case 0x38: modifiers[LEFT_ALT] = !released; break;
            case 0x3A: if (!released) modifiers[CAPS_LOCK] = !modifiers[CAPS_LOCK]; break;
            case 0x45: if (!released) modifiers[NUM_LOCK] = !modifiers[NUM_LOCK]; break;
            case 0x46: if (!released) modifiers[SCROLL_LOCK] = !modifiers[SCROLL_LOCK]; break;
            default: if (!released) goto handle_normal; break;
        }
    } else {
        switch (key) {
            case 0x1D: modifiers[RIGHT_CTL] = !released; break;
            case 0x38: modifiers[RIGHT_ALT] = !released; break;
        }
        extended = false;
    }

handle_normal:
    if (n == '\b') { push_backspace(); goto end; }

    if (modifiers[LEFT_SHFT] || modifiers[RIGHT_SHFT]) push_char(s);
    else if (modifiers[CAPS_LOCK]) push_char(c);
    else push_char(n);

end:
    arch::x86_64::cpu::idt::send_eoi(1);
}

void initialise() {
#ifdef PS2K_CFG_ALLOC_BUF_MALLOC
    __buf = (char*)mem::heap::malloc(PS2K_CFG_INITIAL_BUF_SIZE);
#else
    __buf = (char*)mem::vmm::valloc((PS2K_CFG_INITIAL_BUF_SIZE + 0xFFF)/0x1000);
#endif
    if (!__buf) panic("PS2K buffer allocation failed");
    buf_size = PS2K_CFG_INITIAL_BUF_SIZE;
    head = tail = 0;

    arch::x86_64::cpu::idt::set_descriptor(0x21, (uint64_t)ps2k_interrupt_handler, 0x8E);
}

static char read_c(bool canonical) {
    char c;
    while (!pop_char(c));
    return c;
}

size_t read(size_t n, void* out, bool canonical) {
    size_t read_count = 0;
    char* buf_out = (char*)out;

    while (read_count < n - 1) {
        char c = read_c(canonical);
        buf_out[read_count++] = c;
        if (canonical && c == '\n') break;
    }

    buf_out[read_count] = 0;
    return read_count;
}

}
