//#include <asm/cpu.h>
#include <yaos/types.h>
#define VGABASE         ((const char *)0xb8000)
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
void vga_puts_color(u8 back, u8 fore, const char *str)
{
    uint8_t back_color = (uint8_t) back;
    uint8_t fore_color = (uint8_t) fore;

    uint8_t attribute_byte = (back_color << 4) | (fore_color & 0x0F);

    char c;
    int i, k, j;
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

    }
}

void vga_puts(const char *str)
{
    return vga_puts_color(0, 7, str);
}
