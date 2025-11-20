#ifndef SCHED_HPP
#define SCHED_HPP 1

#include <cstdint>

struct Registers {
    uint64_t rax, rbx, rcx, rdx, rsi, rdi;
    uint64_t rsp, rbp, rip, rflags, cr3;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
};

struct Thread {
    Registers regs;
    uint32_t id;
    bool terminated;
    uint64_t* stack;
    Thread* next;
};

namespace scheduler {

void initialise();
Thread* create(void (*fn)());
void yield();
void schedule();
extern "C" void switch_thread(Registers* old_regs, Registers* new_regs);

}

#endif
