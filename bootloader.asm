[bits 16]
_start:										 
	mov ax, 021fh								; | read op:sector count
	mov bx, 7e00h								; | memory mapped on ram
	mov cx, 2									; | cylinder:sector to start
	xor dh, dh									; | head:drive
	int 13h										; load segments on the disk
exit_16_bit_mode:								; finish everything needed on 16 bit
	mov eax, cr0								; | set the protected bit
	or eax, 1									; | on the control register
	mov cr0, eax								; | to enable 32 bit
	lgdt [desc]									; load the gdt table
	jmp 0x0008:init_32bit						; and jump to the 32 bit zone
gdt:											; gdt table start
	dq 0, 0xcf9A000000ffff, 0xcf92000000ffff	; | all the gdt table lol
desc:											; | the gdt descriptor that is loaded
	dw $-gdt-1, gdt, 0							; gdt table end
[bits 32]										; ------------------------------------------------------------
init_32bit:										; start 32 bit by checking A20 line
	mov esi,0x012345							; | even megabyte address.
	mov edi,0x112345							; | odd megabyte address.
	mov [esi],esi								; | making sure that both addresses contain diffrent values.
	mov [edi],edi								; | (if A20 line is cleared the two pointers would point to the address 0x012345 that would contain 0x112345 (edi)) 
	cmpsd										; | compare addresses to see if they're equivalent.
	jne setup_segments							; end booting if a20 line is off (no better method for now lol)
	hlt
setup_segments:									;
	mov al, 0x10								; update segment registers
	mov ds, ax									; |
	mov ss, ax									; | 
setup_stack:									;
	mov ebp, 80000h								; update stack 
	mov esp, ebp								; because now the kernel is gonna be called and we need the stack for that
	sti											; enable interrupts back
remove_cursor:
	mov eax, 0xa								; put the cursor else where
	mov edx, 0x3d4								; |
	out dx, al									; |
	mov eax, 0x20								; |
	mov edx, 0x3d5								; |
	out dx, al									; |
kernel_call:									;
	call clear_screen							; clear the screen before entering the kernel
	call _startk								; call the "kernel"
	jmp $										; loop forever the computer can cope with this

; ============================================== kernel functions ==============================================
putcat:
	mov	al, byte [esp+4]						; move the char to al
	mov	edx, dword [esp+8]						; get the offset
	mov	ah, 15									; white on black:char
	mov	[edx+753664+edx], ax					; write the char to the screen
	ret
clear_screen:
	mov     eax, 757662
clear_screen_loop:
	mov     word [eax], 3872
	sub     eax, 2
	cmp     eax, 753664
	jne     clear_screen_loop
	xor     eax, eax
	mov     [cursor_offset], eax
	ret
irq0:
	push 32
	jmp irq_common_stub
irq1:
	push 33
irq_common_stub:
	call irq_handler							; and call the handler (i want it here so bad)
	add esp, 4									; pop back error codes
	iret	 	; and might triple fault.
irq_handler:
	mov eax, dword [esp+4]
	mov eax, dword interrupt_handlers[0+eax*4]
	test eax, eax
	je no_handler
	call eax
no_handler:
	mov eax, 32
	mov edx, 32
	out dx, al	
	ret
IRQ_Array:
	dw irq0, irq1

times 510-($-$$) db 0
dw 0xaa55
[extern _startk]
[extern cursor_offset]
[extern interrupt_handlers]
global putcat, IRQ_Array, _start, clear_screen