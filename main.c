#include <stdint.h>
#define BACKSPACE 0x0E
#define ENTER 0x1C
#define IRQ1 33
#define IDT_ENTRIES 64
#define MAX_ROWS 25
#define MAX_COLS 80
#define outw(port, data) __asm__ ("out %%ax, %%dx" : : "a" (data), "d" (port))
#define SC_MAX 57
#define VIDEO_ADDRESS 0xb8000
#define WHITE_ON_BLACK 0x0f

typedef struct {
    uint16_t low_offset, sel; 
    uint8_t always0, flags;
    uint16_t high_offset; 
} __attribute__((packed)) idt_gate_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

extern uint16_t IRQ_Array[3], irq1, cursor_offset;
extern void clear_screen(), putcat(char, int);

idt_gate_t idt[IDT_ENTRIES];
idt_register_t idt_reg;
void * interrupt_handlers[64];
char key_buffer[64];

static void outb (int port, int data) {
    __asm__ ("out %%al, %%dx" : : "a" (data), "d" (port));
}

static unsigned char inb(uint16_t port) {
    unsigned char result;
    __asm__ ("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;
}

int strlenk(char s[]) {
    register int i = 0;
    while (s[i]) ++i;
    return i;
}

unsigned int strcmpk(char s1[], char s2[]) {
    for (register int i = 0; s1[i] == s2[i]; ++i) if (!s1[i]) return 0;
    return 1;
}

void set_idt_gate(int n, uint16_t handler) {
    idt[n].low_offset = handler;
    idt[n].sel = 0x08;
    idt[n].always0 = 0;
    idt[n].flags = 0x8E;
    idt[n].high_offset = 0x0000;
}

unsigned int scroll_ln() {
    for (register uint16_t i = 0; i < MAX_COLS * (MAX_ROWS - 1) * 2; ++i) *(uint16_t *)(VIDEO_ADDRESS + i) = *(uint16_t *)(VIDEO_ADDRESS + 160 + i);
    for (register int col = MAX_COLS; --col;) putcat(' ', (MAX_ROWS - 1) * MAX_COLS + col);
    return MAX_COLS * (MAX_ROWS - 1);
}

void isr_install() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0xA1, 0x02);
    outb(0x21, 0x04);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x00);
    outb(0xA1, 0x00);

    for (register int i = 0; i < 2; i++) set_idt_gate(i + 32, IRQ_Array[i]);

    idt_reg = (idt_register_t) {.base = (uint32_t) &idt, .limit = IDT_ENTRIES * 8 - 1};
    __asm__ volatile("lidt (%0)" : : "r" (&idt_reg));
}

void puts(char *string) {
    for (register int i = 0; string[i]; ++i) {
        if (cursor_offset >= MAX_ROWS * MAX_COLS) cursor_offset = scroll_ln();
        if (string[i] == '\n') cursor_offset = (cursor_offset / MAX_COLS + 1) * MAX_COLS;
        else putcat(string[i], cursor_offset++);
    }
}

void keyboard_callback() {
    uint8_t scancode = inb(0x60);
    register int val;
    if (scancode > SC_MAX) return;
    else if (scancode == BACKSPACE && (val = strlenk(key_buffer))) {
        key_buffer[val - 1] = 0;
        putcat(' ', --cursor_offset); 
    } else if (scancode == ENTER) {
        cursor_offset = cursor_offset / MAX_COLS >= MAX_ROWS - 1 ? scroll_ln() : (cursor_offset / MAX_COLS + 1) * MAX_COLS;
        if (strcmpk(key_buffer, "exit") == 0) outw(0x0604, 0x2000); // send shutdown to qemu
        else if (strcmpk(key_buffer, "reboot") == 0) outb(0x64, 0xFE);
        else if (strcmpk(key_buffer, "clear") == 0) clear_screen();
        else puts("bad prompt\n");
        puts("> ");
        *key_buffer = 0;
    } else {
        if (!(val = "\0001234567890-=\0\0qwertyuiop[]\0\0asdfghjkl;'`\0\\zxcvbnm,./\0\0\0 "[(int) scancode - 1])) return;
        val = strlenk(key_buffer);
        *(uint16_t *)(key_buffer + val) = val;
        puts(key_buffer + val);
    }      
}

void _startk() {
    interrupt_handlers[IRQ1] = keyboard_callback;
    isr_install();
    puts("> ");
}