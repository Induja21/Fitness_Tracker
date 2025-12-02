#include "MIP.h"

#include "em_gpio.h"
#include "em_usart.h"
#include "em_cmu.h"
#include "FreeMono12pt7b.h"
/*
 * Pins from your schematic:
 *   PA4  = LCD_VCOM
 *   PC6  = LCD_RST
 *   PC7  = LCD_SDI (MOSI, USART1_TX)
 *   PC8  = LCD_CS  (SCS)
 *   PC9  = LCD_SCK (USART1_CLK)
 */

#define KYOCERA_USART              USART1

#define KYOCERA_LCD_VCOM_PORT      gpioPortA
#define KYOCERA_LCD_VCOM_PIN       4

#define KYOCERA_LCD_RST_PORT       gpioPortC
#define KYOCERA_LCD_RST_PIN        6

#define KYOCERA_LCD_CS_PORT        gpioPortC
#define KYOCERA_LCD_CS_PIN         8

/* 256×256 → 32 bytes per row */
uint8_t kyocera_fb[KYOCERA_LCD_HEIGHT][KYOCERA_LCD_WIDTH / 8];

/* ---------------------------------------------------------
 * BIT REVERSE (LCD REQUIRES LSB FIRST)
 * --------------------------------------------------------- */
static inline uint8_t reverse8(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

/* ---------------------------------------------------------
 * SPI INIT
 * --------------------------------------------------------- */
static void kyocera_spi_init(void)
{
    CMU_ClockEnable(cmuClock_USART1, true);
    CMU_ClockEnable(cmuClock_GPIO,  true);

    // Idle states required by mode 0
    GPIO_PinModeSet(gpioPortC, 7, gpioModePushPull, 0);   // MOSI idle low
    GPIO_PinModeSet(gpioPortC, 9, gpioModePushPull, 0);   // SCK idle low

    USART_InitSync_TypeDef cfg = USART_INITSYNC_DEFAULT;

    cfg.enable     = usartDisable;
    cfg.baudrate   = 500000; // safe
    cfg.msbf       = true;
    cfg.clockMode  = usartClockMode0; // CPOL0 CPHA0
    cfg.databits   = usartDatabits8;

    USART_InitSync(KYOCERA_USART, &cfg);

    KYOCERA_USART->ROUTEPEN =
        USART_ROUTEPEN_TXPEN |
        USART_ROUTEPEN_CLKPEN;

    KYOCERA_USART->ROUTELOC0 =
        USART_ROUTELOC0_TXLOC_LOC12 |
        USART_ROUTELOC0_CLKLOC_LOC12;

    USART_Enable(KYOCERA_USART, usartEnable);
}

/* ---------------------------------------------------------
 * LOW LEVEL HELPERS
 * --------------------------------------------------------- */

static inline void lcd_select(void)
{
    GPIO_PinOutSet(KYOCERA_LCD_CS_PORT, KYOCERA_LCD_CS_PIN);   // ACTIVE HIGH
}

void lcd_deselect(void)
{
    GPIO_PinOutClear(KYOCERA_LCD_CS_PORT, KYOCERA_LCD_CS_PIN); // INACTIVE LOW
}

static inline void lcd_tx(uint8_t data)
{
    USART_Tx(KYOCERA_USART, data);
    while (!(KYOCERA_USART->STATUS & USART_STATUS_TXC));
}

/* ---------------------------------------------------------
 * RESET
 * --------------------------------------------------------- */
void kyocera_lcd_reset(void)
{
    GPIO_PinOutClear(KYOCERA_LCD_VCOM_PORT, KYOCERA_LCD_VCOM_PIN);

    // RST HIGH = off
    GPIO_PinOutSet(KYOCERA_LCD_RST_PORT, KYOCERA_LCD_RST_PIN);
    for (volatile uint32_t i = 0; i < 200000; i++) __NOP(); // 10ms

    // RST LOW = on
    GPIO_PinOutClear(KYOCERA_LCD_RST_PORT, KYOCERA_LCD_RST_PIN);
    for (volatile uint32_t i = 0; i < 50000; i++) __NOP(); // 2ms
}

/* ---------------------------------------------------------
 * INIT
 * --------------------------------------------------------- */
void kyocera_lcd_init(void)
{
    CMU_ClockEnable(cmuClock_GPIO, true);

    GPIO_PinModeSet(KYOCERA_LCD_CS_PORT, KYOCERA_LCD_CS_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(KYOCERA_LCD_RST_PORT, KYOCERA_LCD_RST_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(KYOCERA_LCD_VCOM_PORT, KYOCERA_LCD_VCOM_PIN, gpioModePushPull, 0);

    kyocera_spi_init();
    kyocera_lcd_reset();
}

void kyocera_lcd_flush(void)
{
    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    lcd_select(); // SCS goes HIGH

    // [SETUP DELAY]
    for(volatile int i = 0; i < 2000; i++) { __NOP(); }

    for (uint16_t y = 0; y < KYOCERA_LCD_HEIGHT; y++)
    {
        // 1. Address MUST be reversed (LSB First)
        lcd_tx(reverse8((uint8_t)y));

        // 2. Pixel Data MUST NOT be reversed (MSB First = D0 First)
        for (uint8_t b = 0; b < 32; b++)
        {
            // >>> CHANGE IS HERE: REMOVE reverse8() <<<
            lcd_tx(kyocera_fb[y][b]);
        }

        // 3. Dummy Cycles
        lcd_tx(0x00);
        lcd_tx(0x00);
        lcd_tx(0x00);
        lcd_tx(0x00);
    }

    // [HOLD DELAY]
    for(volatile int i = 0; i < 2000; i++) { __NOP(); }

    lcd_deselect(); // SCS goes LOW

    // [INTERVAL DELAY]
    for(volatile int i = 0; i < 2000; i++) { __NOP(); }

    CORE_EXIT_CRITICAL();
}
/* ---------------------------------------------------------
 * FULL REFRESH
 * --------------------------------------------------------- */
/* ---------------------------------------------------------
 * CLEAR
 * --------------------------------------------------------- */
void kyocera_lcd_clear(bool white)
{
    uint8_t fill = white ? 0xFF : 0x00;

    // Fill buffer
    for (uint16_t y = 0; y < KYOCERA_LCD_HEIGHT; y++)
        for (uint16_t b = 0; b < 32; b++)
            kyocera_fb[y][b] = fill;

    // Push to screen in one continuous stream
    kyocera_lcd_flush();
}
/* ---------------------------------------------------------
 * VCOM TOGGLE
 * --------------------------------------------------------- */
void kyocera_lcd_toggle_vcom(void)
{
    static bool s = false;

    if (s)
        GPIO_PinOutSet(KYOCERA_LCD_VCOM_PORT, KYOCERA_LCD_VCOM_PIN);
    else
        GPIO_PinOutClear(KYOCERA_LCD_VCOM_PORT, KYOCERA_LCD_VCOM_PIN);

    s = !s;
}
/* ---------------------------------------------------------
 * DIAGNOSTIC: DRAW 3 TEST LINES
 * Draws lines at Y=10, Y=128, Y=240 with a zebra pattern.
 * --------------------------------------------------------- */
void kyocera_lcd_draw_test_lines(void)
/* ---------------------------------------------------------
 * DIAGNOSTIC: DRAW 5 TEST LINES
 * Draws 5 solid black lines at fixed Y positions.
 * --------------------------------------------------------- */
{
    // The 5 specific lines we want to draw
    uint16_t lines[5] = {10, 70, 128, 190, 240};

    CORE_DECLARE_IRQ_STATE;
    CORE_ENTER_CRITICAL();

    for (int i = 0; i < 5; i++)
    {
        uint16_t y = lines[i];

        lcd_select(); // SCS High

        // [SETUP DELAY] Wait >4us (Datasheet tsSCS)
        for(volatile int d = 0; d < 2000; d++) { __NOP(); }

        // 1. Send Address (Reversed)
        // Datasheet: Gate Address is LSB first
        lcd_tx(reverse8((uint8_t)y));

        // 2. Send Data (32 bytes of 0x00 = BLACK)
        // Datasheet: 0 = Black, 1 = White
        for (uint8_t b = 0; b < 32; b++)
        {
            lcd_tx(0x00);
        }

        // 3. Dummy Cycles (4 bytes)
        lcd_tx(0x00);
        lcd_tx(0x00);
        lcd_tx(0x00);
        lcd_tx(0x00);

        // [HOLD DELAY] Wait >4us (Datasheet thSCS)
        for(volatile int d = 0; d < 2000; d++) { __NOP(); }

        lcd_deselect(); // SCS Low

        // [INTERVAL DELAY] Wait >10us (Datasheet twSCSL)
        for(volatile int d = 0; d < 5000; d++) { __NOP(); }
    }

    CORE_EXIT_CRITICAL();
}

/* ---------------------------------------------------------
 * HELPER: SET PIXEL
 * 0 = Black, 1 = White
 * --------------------------------------------------------- */
void kyocera_set_pixel(uint16_t x, uint16_t y, bool white)
{
    if (x >= KYOCERA_LCD_WIDTH || y >= KYOCERA_LCD_HEIGHT) return;

    uint8_t bit_mask = 1 << (7 - (x % 8)); // MSB mapping

    if (white) {
        kyocera_fb[y][x / 8] |= bit_mask;  // Set bit to 1
    } else {
        kyocera_fb[y][x / 8] &= ~bit_mask; // Clear bit to 0
    }
}





void kyocera_draw_gfx_char(int16_t x, int16_t y, unsigned char c, const GFXfont *font)
{
    if (c < font->first || c > font->last) return;

    const GFXglyph *glyph = &(font->glyph[c - font->first]);
    const uint8_t  *bitmap = font->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width, h = glyph->height;
    int8_t   xo = glyph->xOffset, yo = glyph->yOffset;

    uint8_t  bits = 0, bit = 0;

    for(int yy = 0; yy < h; yy++) {
        for(int xx = 0; xx < w; xx++) {
            if(!(bit++ & 7)) bits = bitmap[bo++];

            if(bits & 0x80) {
                // Draw Pixel (False = Black)
                kyocera_set_pixel(x + xo + xx, y + yo + yy, false);
            }
            bits <<= 1;
        }
    }
}

/* ---------------------------------------------------------
 * DRAW STRING HELPER
 * --------------------------------------------------------- */
void kyocera_draw_string(int16_t x, int16_t y, const char *str)
{
    int16_t cursor_x = x;
    int16_t cursor_y = y;
    unsigned char c;

    while((c = *str++)) {
        if (c == '\n') {
            cursor_x = x;
            cursor_y += FreeMono12pt7b.yAdvance;
        } else {
            kyocera_draw_gfx_char(cursor_x, cursor_y, c, &FreeMono12pt7b);
            const GFXglyph *glyph = &(FreeMono12pt7b.glyph[c - FreeMono12pt7b.first]);
            cursor_x += glyph->xAdvance;
        }
    }
}

/* ---------------------------------------------------------
 * HEART BITMAPS (16x16)
 * 1 = Black Pixel, 0 = White Pixel
 * --------------------------------------------------------- */
static const uint16_t heart_large[16] = {
    0x0000, 0x0000, 0x1C38, 0x3E7C,
    0x7FFE, 0x7FFE, 0x7FFE, 0x3FFC,
    0x1FF8, 0x0FF0, 0x07E0, 0x03C0,
    0x0180, 0x0000, 0x0000, 0x0000
};

static const uint16_t heart_small[16] = {
    0x0000, 0x0000, 0x0000, 0x0C30,
    0x1E78, 0x3FFC, 0x3FFC, 0x1FF8,
    0x0FF0, 0x07E0, 0x03C0, 0x0180,
    0x0000, 0x0000, 0x0000, 0x0000
};
static const uint8_t ble_icon_bitmap[12] = {
  0b00001000,
  0b00011100,
  0b00101010,
  0b01001001,
  0b01001001,
  0b00101010,
  0b00011100,
  0b00001000,
  0b00011100,
  0b00101010,
  0b01001001,
  0b10000000
};

static const uint8_t ble_icon_disconnected[12] = {
  0b10001000,
  0b01011100,
  0b00101010,
  0b01011001,
  0b01001001,
  0b00101110,
  0b00111110,
  0b00001001,
  0b00011110,
  0b00101100,
  0b01001001,
  0b10010000
};


// Clear rectangular area in framebuffer (false = black, true = white)
void kyocera_clear_rect(uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h,
                        bool white)
{
    uint8_t fill = white ? 0xFF : 0x00;

    for (uint16_t yy = y; yy < (y + h); yy++)
    {
        if (yy >= KYOCERA_LCD_HEIGHT) break;

        for (uint16_t xx = x; xx < (x + w); xx++)
        {
            if (xx >= KYOCERA_LCD_WIDTH) break;

            uint8_t bit_mask = 1 << (7 - (xx % 8));

            if (white)
                kyocera_fb[yy][xx / 8] |= bit_mask;
            else
                kyocera_fb[yy][xx / 8] &= ~bit_mask;
        }
    }
}
#define BLE_ICON_W   12
#define BLE_ICON_H   12
uint16_t ble_icon_x = KYOCERA_LCD_WIDTH - BLE_ICON_W - 10;   // 256 - 12 - 10 = 234
uint16_t ble_icon_y = 10;                                    // top margin
void draw_ble_icon()
{
    for (uint8_t row = 0; row < BLE_ICON_H; row++)
    {
        uint8_t bits = ble_icon_bitmap[row];

        for (uint8_t col = 0; col < BLE_ICON_W; col++)
        {
            uint8_t pixel = (bits >> col) & 1;

            if (pixel)
            {
                kyocera_set_pixel(
                    ble_icon_x + col,
                    ble_icon_y + row,
                    false    // BLACK (same as heart)
                );
            }
        }
    }
}


void draw_ble_icon_disconnected()
{
    for (uint8_t row = 0; row < BLE_ICON_H; row++)
    {
        uint8_t bits = ble_icon_disconnected[row];

        for (uint8_t col = 0; col < BLE_ICON_W; col++)
        {
            uint8_t pixel = (bits >> col) & 1;

            if (pixel)
            {
                kyocera_set_pixel(
                    ble_icon_x + col,
                    ble_icon_y + row,
                    false    // BLACK (same as heart)
                );
            }
        }
    }
}


/* ---------------------------------------------------------
 * DRAW HEART (SCALED 4X)
 * Draws a 16x16 bitmap as a 64x64 image so it looks big!
 * --------------------------------------------------------- */
void kyocera_draw_heart(uint16_t x, uint16_t y, bool large)
{
    const uint16_t *bitmap = large ? heart_large : heart_small;

    for (int row = 0; row < 16; row++)
    {
        uint16_t line_data = bitmap[row];

        for (int col = 0; col < 16; col++)
        {
            // Check if the bit is set (Scan from MSB to LSB)
            if (line_data & (0x8000 >> col))
            {
                // SCALE 4X: Draw a 4x4 block of black pixels for every 1 bitmap pixel
                for(int i=0; i<4; i++) {
                    for(int j=0; j<4; j++) {
                        kyocera_set_pixel(x + col*4 + j, y + row*4 + i, false); // False = Black
                    }
                }
            }
        }
    }
}
void kyocera_clear_buffer(bool white)
{
    uint8_t fill = white ? 0xFF : 0x00;

    for (uint16_t y = 0; y < KYOCERA_LCD_HEIGHT; y++)
        for (uint16_t b = 0; b < 32; b++)
            kyocera_fb[y][b] = fill;
}


/* ---------------------------------------------------------
 * WALKER BITMAPS (16x24) - Frame 1 and Frame 2
 * --------------------------------------------------------- */
/* ---------------------------------------------------------
 * SOLID WALKER BITMAPS (16x24) - Bold/Filled Style
 * --------------------------------------------------------- */
static const uint16_t walker_1[24] = {
    // FRAME 1: STRIDE (Legs Open)
    0x0000,
    0x03C0, 0x07E0, 0x07E0, 0x03C0, // Head (Solid Circle)
    0x0000,
    0x03C0, 0x0FC0, 0x1FC0, 0x3FC0, // Torso + Arms Swing Left
    0x3FC0, 0x0FC0, 0x0FC0, 0x0FC0,
    0x0FC0, 0x1EE0, 0x3C70, 0x7038, // Upper Legs Split
    0xE01C, 0xC00E, 0xC006, 0x8000, // Lower Legs Wide
    0x0000, 0x0000
};

static const uint16_t walker_2[24] = {
    // FRAME 2: PASS (Legs Straight)
    0x0000,
    0x03C0, 0x07E0, 0x07E0, 0x03C0, // Head (Solid Circle)
    0x0000,
    0x03C0, 0x03C0, 0x07E0, 0x07E0, // Torso (Straight)
    0x07E0, 0x03C0, 0x03C0, 0x03C0,
    0x03C0, 0x03C0, 0x03C0, 0x03C0, // Legs (Solid Column)
    0x03C0, 0x03C0, 0x03C0, 0x03C0,
    0x0000, 0x0000
};

/* ---------------------------------------------------------
 * ECG BITMAP (64x16) - One "PQRST" Complex
 * We will draw this multiple times to make a long graph.
 * --------------------------------------------------------- */

static const int8_t ecg_pattern[64] = {
     0,  0,  0,  0,  0,  0,  0,  0, // Flat
    -2, -4, -2,  0,  0,  0,  0,  0, // P-Wave (Small bump)
     0,  1,  2,  0, -5,-15,-30,-10, // Q (Dip) -> R (Spike Up)
    10,  5,  0,  0,  0,  0,  0,  0, // S (Dip Down) -> Recovery
     0,  0,  0, -2, -3, -4, -3, -2, // T-Wave (Round bump)
     0,  0,  0,  0,  0,  0,  0,  0, // Flat
     0,  0,  0,  0,  0,  0,  0,  0,
     0,  0,  0,  0,  0,  0,  0,  0
};
/* ---------------------------------------------------------
 * DRAW WALKER (Scaled 2x)
 * Draws the person at x,y. Use frame 0 or 1.
 * --------------------------------------------------------- */
void kyocera_draw_walker(uint16_t x, uint16_t y, bool frame_stride)
{
    const uint16_t *bitmap = frame_stride ? walker_1 : walker_2;

    for (int row = 0; row < 24; row++) {
        uint16_t line = bitmap[row];
        for (int col = 0; col < 16; col++) {
            if (line & (0x8000 >> col)) {
                // Scale 2x: Draw 2x2 blocks
                kyocera_set_pixel(x + col*2,     y + row*2,     false);
                kyocera_set_pixel(x + col*2 + 1, y + row*2,     false);
                kyocera_set_pixel(x + col*2,     y + row*2 + 1, false);
                kyocera_set_pixel(x + col*2 + 1, y + row*2 + 1, false);
            }
        }
    }
}

/* ---------------------------------------------------------
 * DRAW ECG GRAPH
 * Draws a static heartbeat graph at the bottom.
 * --------------------------------------------------------- */
/* ---------------------------------------------------------
 * DRAW ECG WAVE (Line Graph)
 * Draws a sharp, continuous medical-style wave.
 * --------------------------------------------------------- */
void kyocera_draw_ecg_graph(uint16_t y_baseline)
{
    // 1. Draw the Graph Line
    for (int x = 0; x < 256; x++) {
        // Get the height offset from our pattern
        // The '%' operator makes the pattern repeat every 64 pixels
        int8_t offset = ecg_pattern[x % 64];

        // Draw the main pixel (False = Black)
        kyocera_set_pixel(x, y_baseline + offset, false);

        // Optional: Make the vertical spikes (QRS) thicker/connected
        // If the jump is big, fill in the gap slightly
        if (offset < -5 || offset > 5) {
             kyocera_set_pixel(x, y_baseline + offset + 1, false);
             kyocera_set_pixel(x, y_baseline + offset - 1, false);
        }
    }

    // 2. Draw a flat baseline at the bottom for reference (optional)
    for (int x = 0; x < 256; x++) {
        kyocera_set_pixel(x, y_baseline + 20, false);
    }
}
