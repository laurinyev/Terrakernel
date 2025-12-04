bits 64
section .text
global syscall_func
extern syscall_handler
extern _tss_rsp
extern _tss_rbp

syscall_func:
    swapgs
    mov rsp, [_tss_rsp]
    mov rbp, [_tss_rbp]

    push rcx
    push r11

    push rax
    push rdi
    push rsi
    push rdx
    push r10
    push r8
    push r9
    push rbp

    mov rdi, [rsp+56]
    mov rsi, [rsp+48]
    mov rdx, [rsp+40]
    mov r10, [rsp+32]
    mov r8, [rsp+24]
    mov r9, [rsp+16]

    call syscall_handler

    pop rbp
    pop r9
    pop r8
    pop r10
    pop rdx
    pop rsi
    pop rdi
    pop rax

    pop r11
    pop rcx

    swapgs
    sysretq
