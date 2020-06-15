#include <asm/cpu.h>
#include <yaos/types.h>
#include <asm/pgtable.h>
#include <yaos/spinlock.h>
#include <yaos/irq.h>
#define VGABASE         ((const char *)P2V(0xb8000))
void uart_putc(int);

static inline void writew(unsigned short c, const char *addr)
{
    *((unsigned short *)(addr)) = c;
}

static inline unsigned short readw(const char *addr)
{
    return *((unsigned short *)(addr));
}

static int max_ypos = 25, max_xpos = 80;
static int current_ypos = 0, current_xpos = 0;
spinlock_t spin_vga = 0;

void vga_puts_color(u8 back, u8 fore, const char *str, size_t size)
{
    uint8_t back_color = (uint8_t) back;
    uint8_t fore_color = (uint8_t) fore;

    uint8_t attribute_byte = (back_color << 4) | (fore_color & 0x0F);

    char c;
    int i, k, j;
    unsigned long flag = local_irq_save();
    spin_lock(&spin_vga);
    size_t cnt = 0;
    while ((c = *str++) != '\0') {
        uart_putc(c);
        if (current_ypos > max_ypos) {
            for (k = 1, j = 0; k < max_ypos; k++, j++) {
                for (i = 0; i < max_xpos; i++) {
                    writew(readw(VGABASE + 2 * (max_xpos * k + i)),
                           VGABASE + 2 * (max_xpos * j + i));
                }
            }
            for (i = 0; i < max_xpos; i++)
                writew(0x720, VGABASE + 2 * (max_xpos * j + i));
            current_ypos = max_ypos - 1;

        }
        if (c == '\n') {
            current_xpos = 0;
            current_ypos++;
        }
        else if (c != '\r') {
            writew(((attribute_byte << 8) | (unsigned short)c),
                   VGABASE + 2 * (max_xpos * current_ypos + current_xpos++));
            if (current_xpos >= max_xpos) {
                current_xpos = 0;
                current_ypos++;
            }
        }
        if (size && ++cnt>=size)break;

    }
    spin_unlock(&spin_vga);
    local_irq_restore(flag);

}

void vga_puts(const char *str)
{
    return vga_puts_color(0, 7, str, 0);
}
ssize_t vga_write(char *str, size_t size)
{
    if (!size)return 0;
    vga_puts_color(0, 7, str, size);
    return (ssize_t)size;
    
}
