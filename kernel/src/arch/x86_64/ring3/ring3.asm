bits 64
section .text
global execute_ring3
execute_ring3:
	push 0x1B
	push rsi
	push 0x202
	push 0x23
	push rdi

	iretq ; make the switch
