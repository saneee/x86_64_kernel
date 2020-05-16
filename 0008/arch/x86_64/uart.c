// Intel 8250 serial port (UART).

#include <asm/cpu.h>
#define COM1    0x3f8
void micro_delay(int d)
{
};

static int uart;                // is there a uart?
void uart_putc(int);
void uart_early_init(void)
{
    char *p;

    // Turn off the FIFO
    outb(0, COM1 + 2);

    // 9600 baud, 8 data bits, 1 stop bit, parity off.
    outb(0x80, COM1 + 3);       // Unlock divisor
    outb(115200 / 9600, COM1 + 0);
    outb(0, COM1 + 1);
    outb(0x03, COM1 + 3);       // Lock divisor, 8 data bits.
    outb(0, COM1 + 4);
    outb(0x01, COM1 + 1);       // Enable receive interrupts.

    // If status is 0xFF, no serial port.
    if (inb(COM1 + 5) == 0xFF)
        return;
    uart = 1;

    // Announce that we're here.
    for (p = "yaos...\n"; *p; p++)
        uart_putc(*p);
}

void uart_init(void)
{
    if (!uart)
        return;

    // Acknowledge pre-existing interrupt conditions;
    // enable interrupts.
    inb(COM1 + 2);
    inb(COM1 + 0);
    //pic_enable(IRQ_COM1);
    //ioapic_enable(IRQ_COM1, 0);
}

void uart_putc(int c)
{
    int i;

    if (!uart)
        return;
    for (i = 0; i < 128 && !(inb(COM1 + 5) & 0x20); i++)
        micro_delay(10);
    outb(c, COM1 + 0);
}

static int uart_getc(void)
{
    if (!uart)
        return -1;
    if (!(inb(COM1 + 5) & 0x01))
        return -1;
    return inb(COM1 + 0);
}

void console_intr(int (*getc) (void))
{
}

void uart_intr(void)
{
    console_intr(uart_getc);
}
