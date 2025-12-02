#ifndef KYOCERA_LCD_H
#define KYOCERA_LCD_H

#include <stdint.h>
#include <stdbool.h>
// 1. Define PROGMEM as empty (EFR32 stores const in Flash automatically)
#ifndef PROGMEM
#define PROGMEM
#endif

#define KYOCERA_LCD_WIDTH   256u
#define KYOCERA_LCD_HEIGHT  256u

/* MIP.h - Add this section */




// 2. Define the Adafruit Font Structures
typedef struct {
    uint16_t bitmapOffset;     // Pointer into the bitmap array
    uint8_t  width;            // Bitmap dimensions in pixels
    uint8_t  height;           // Bitmap dimensions in pixels
    uint8_t  xAdvance;         // Distance to advance cursor (x axis)
    int8_t   xOffset;          // X dist from cursor pos to UL corner
    int8_t   yOffset;          // Y dist from cursor pos to UL corner
} GFXglyph;

typedef struct {
    const uint8_t  *bitmap;    // Glyph bitmaps, concatenated
    const GFXglyph *glyph;     // Glyph array
    uint16_t  first;           // ASCII extents (first char)
    uint16_t  last;            // ASCII extents (last char)
    uint8_t   yAdvance;        // Newline distance (y axis)
} GFXfont;

// 3. Prototype your drawing functions
void kyocera_draw_gfx_char(int16_t x, int16_t y, unsigned char c, const GFXfont *font);
void kyocera_draw_string(int16_t x, int16_t y, const char *str);
/*
 * Framebuffer: 1 bit per pixel
 * 1 = WHITE, 0 = BLACK   (per Kyocera datasheet)
 */
extern uint8_t kyocera_fb[KYOCERA_LCD_HEIGHT][KYOCERA_LCD_WIDTH / 8u];

/* Init + basic control */
void kyocera_lcd_init(void);           // call once after sl_system_init()
void kyocera_lcd_reset(void);          // optional extra reset

/* Drawing / refresh */
void kyocera_lcd_clear(bool white);    // clear FB + push to panel
void kyocera_lcd_flush(void);          // push whole framebuffer


/* Pixel access */
void kyocera_lcd_set_pixel(uint16_t x, uint16_t y, bool white);

/* VCOM toggle – call from a ~1 Hz timer ISR/callback */
void kyocera_lcd_toggle_vcom(void);
void kyocera_lcd_flush_line(uint16_t y);
void lcd_deselect(void);
void kyocera_lcd_write_raw_test(void);
void kyocera_clear_rect(uint16_t x, uint16_t y,
                        uint16_t w, uint16_t h,
                        bool white);
void kyocera_lcd_draw_test_lines(void);
void kyocera_write_abc_test(void);
void kyocera_draw_ecg_and_heart(int frame);
void kyocera_draw_heart(uint16_t x, uint16_t y, bool large);
void kyocera_clear_buffer(bool white);

void kyocera_draw_walker(uint16_t x, uint16_t y, bool frame_stride);
void kyocera_draw_ecg_graph(uint16_t y_baseline);
void draw_ble_icon_disconnected();
void draw_ble_icon();

#endif // KYOCERA_LCD_H
