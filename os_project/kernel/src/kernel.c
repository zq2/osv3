#include "kernel.h"
#include "string.h"

#define MAX_INPUT_SIZE 256
#define MAX_ARGS 10

char input_buffer[MAX_INPUT_SIZE];

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t* const VGA_MEMORY = (uint16_t*) 0xB8000;

static size_t terminal_row;
static size_t terminal_column;
static uint8_t terminal_color;
static uint16_t* terminal_buffer;

// Function declarations
void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_writestring(const char* data);
char get_char(void);
void read_input(void);
void parse_input(char* input, char** argv);
void execute_command(char** argv);
void shell(void);
char scancode_to_char(uint8_t scancode);
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t value);
void idt_install(void);
void isr_install(void);
void keyboard_install(void);
void enable_interrupts(void);

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_IRQ 1

#define BUFFER_SIZE 128
static char key_buffer[BUFFER_SIZE];
static size_t buffer_head = 0;
static size_t buffer_tail = 0;

static inline bool buffer_is_empty() {
    return buffer_head == buffer_tail;
}

static inline bool buffer_is_full() {
    return ((buffer_head + 1) % BUFFER_SIZE) == buffer_tail;
}

uint8_t inb(uint16_t port) {
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void keyboard_isr() {
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    // Convert scancode to ASCII character and store in buffer
    char c = scancode_to_char(scancode);
    if (c && !buffer_is_full()) {
        key_buffer[buffer_head] = c;
        buffer_head = (buffer_head + 1) % BUFFER_SIZE;
    }
    // Send End of Interrupt (EOI) signal to the PIC
    outb(0x20, 0x20);
}

char get_char() {
    while (buffer_is_empty()) {
        // Wait for a character to be available
    }
    char c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % BUFFER_SIZE;
    return c;
}

void read_input() {
    size_t index = 0;
    char c;
    while (1) {
        c = get_char();
        if (c == '\n' || c == '\r') {
            input_buffer[index] = '\0';
            break;
        } else if (c == '\b' && index > 0) {
            index--;
            terminal_putchar('\b'); // Handle backspace
        } else if (index < MAX_INPUT_SIZE - 1) {
            input_buffer[index++] = c;
            terminal_putchar(c);
        }
    }
}

void parse_input(char* input, char** argv) {
    size_t argc = 0;
    char* token = strtok(input, " ");
    while (token != NULL && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;
}

void execute_command(char** argv) {
    if (strcmp(argv[0], "echo") == 0) {
        for (size_t i = 1; argv[i] != NULL; i++) {
            terminal_writestring(argv[i]);
            terminal_putchar(' ');
        }
        terminal_putchar('\n');
    } else if (strcmp(argv[0], "clear") == 0) {
        terminal_initialize();
    } else {
        terminal_writestring("Unknown command: ");
        terminal_writestring(argv[0]);
        terminal_putchar('\n');
    }
}

void terminal_initialize(void) {
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = VGA_MEMORY;
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_putchar(char c) {
    unsigned char uc = c;
    terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] = vga_entry(uc, terminal_color);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

void terminal_writestring(const char* data) {
    size_t i = 0;
    while (data[i] != '\0') {
        terminal_putchar(data[i]);
        i++;
    }
}

void shell() {
    while (1) {
        terminal_writestring("> ");
        read_input();
        char* argv[MAX_ARGS];
        parse_input(input_buffer, argv);
        if (argv[0] != NULL) {
            execute_command(argv);
        }
    }
}

void isr_install() {
    idt_set_gate(33, (uint32_t)keyboard_isr, 0x08, 0x8E);
}

void keyboard_install() {
    // Enable keyboard interrupts
    outb(0x21, inb(0x21) & ~0x02);
}

void enable_interrupts() {
    __asm__ __volatile__("sti");
}

void kernel_main(void) {
    terminal_initialize();
    idt_install();
    isr_install();
    keyboard_install();
    enable_interrupts();
    terminal_writestring("Welcome to MyOS!\n");
    shell();
}

char scancode_to_char(uint8_t scancode) {
    static char scancode_map[128] = {
        0, 27, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
        '9', '0', '-', '=', '\b', /* Backspace */
        '\t', /* Tab */
        'q', 'w', 'e', 'r', /* 19 */
        't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
        0, /* 29   - Control */
        'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
        '\'', '`', 0, /* Left shift */
        '\\', 'z', 'x', 'c', 'v', 'b', 'n', /* 49 */
        'm', ',', '.', '/', 0, /* Right shift */
        '*',
        0, /* Alt */
        ' ', /* Space bar */
        0, /* Caps lock */
        0, /* 59 - F1 key ... > */
        0, 0, 0, 0, 0, 0, 0, 0,
        0, /* < ... F10 */
        0, /* 69 - Num lock*/
        0, /* Scroll Lock */
        0, /* Home key */
        0, /* Up Arrow */
        0, /* Page Up */
        '-',
        0, /* Left Arrow */
        0,
        0, /* Right Arrow */
        '+',
        0, /* 79 - End key*/
        0, /* Down Arrow */
        0, /* Page Down */
        0, /* Insert Key */
        0, /* Delete Key */
        0, 0, 0, 0, /* F11 Key */
        0, /* F12 Key */
        0, /* All other keys are undefined */
    };
    return scancode_map[scancode];
}