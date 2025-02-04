/*
 *
 *  Videoton TV Computer C stub
 *  Sandor Vass - 2019
 *
 *  Headerfile for TVC specific stuff
 *
 */

#ifndef __TVC_H__
#define __TVC_H__

#include <sys/types.h>

// constants
#define TVC_CHAR_RETURN 0x0D
#define TVC_CHAR_ESC    0x1B

enum video_mode {VMODE_2C=0x00, VMODE_4C=0x01, VMODE_16C=0x02};

#define CBLUE      0x01
#define CRED       0x04
#define CGREEN     0x10
#define CINTENSITY 0x40

enum colors {black  = 0x00, blue    = 0x01, red     = 0x04, purple = 0x05,
             green  = 0x10, cyan    = 0x11, brown   = 0x14, grey   = 0x15,
             lblue  = 0x41, lred    = 0x44, lpurple = 0x45, lgreen = 0x50,
             lcyan  = 0x51, yellow  = 0x54, white   = 0x55};

#define MMAP_P0_SYS   0x00
#define MMAP_P0_U0    0x10
#define MMAP_P0_CART  0x08
#define MMAP_P0_U3    0x18  ; valid only in 64k+ or 32k machines with 32k mem expansion
#define MMAP_P1_U1    0x00
#define MMAP_P1_VID   0x04  ; valid only in 64k+ machines
#define MMAP_P2_VID   0x00
#define MMAP_P2_U2    0x20
#define MMAP_P3_CART  0x00
#define MMAP_P3_SYS   0x40
#define MMAP_P3_U3    0x80
#define MMAP_P3_IO_EXT  0xC0

typedef union composite_color {
    enum colors color;
    int paletteIndex;
} color_or_index;

// TVC Editor functions
/**
 * Gets a string from the console using TVC's screen editor
 */
char *tvc_fgets_cons(char* str, size_t max);

/**
 * Starts the editor and gets a character on pressing enter (func $A1)
 */
char __LIB__ tvc_ed_getch();

/**
 * Before editor's CHIN this method fixes the character position
 * from where the ccharcters are returned. (func $A4)
 */
void __LIB__ tvc_ed_cfix();

/**
 * Sets the current character position of the editor (1,1) is upper left
 * (16/32/64, 24) is the lower right, depending on the actual resolution
 * returns 0xF6 in case of invalid position provided (func $A3)
 */
void __LIB__ tvc_ed_cpos(char col, char row);

/**
 * Prints one character to the console from the actual
 * character position (func $A4)
 */
#define tvc_ed_chout     fputc_cons_native
char __LIB__ fputc_cons_native(int character);


// TVC Keyboard functions

/**
 * Get a character from the console. If there is no pressed char
 * earlier this call will block (func $A1)
 */
#define tvc_kbd_chin     fgetc_cons

/**
 * Checks if a key was pressed earlier or not
 */
int __LIB__ tvc_kbd_status();

/**
 * Gets a character from the console. If there is no pressed char
 * earlier this call returns 0x00.
 */
#define tvc_getkey getk

// screen, video functions

/**
 * Clears the screen and sets the cursors to their base position
 * (editor cursor to the upper left, graphic cursor to the lower left)
 */
void __LIB__ tvc_cls(void);

/**
 * Sets the video mode of TVC (2 colors, 4 colors, 16 colors), clears screen
 * and initialize cursor positions. Returns the error code.
 */
#define tvc_set_vmode tvc_vmode
int __LIB__ tvc_vmode(enum video_mode mode);

enum video_mode __LIB__ tvc_get_vmode();

/**
  * Returns the border color.
 */
enum colors tvc_get_border();

/**
 * Sets the border color
 */
void tvc_set_border(enum colors c);

/**
 * Sets the palette color for the specified index. The index must be 0-3, inclusive,
 * otherwise nothing is set
 */
void __LIB__ tvc_set_palette(enum colors, int palette_index);

/**
 * Sets the current paper (character background color).
 * In case of 2 or 4 color modes this method only sets the provided palette
 * index, in case of 16 color mode this method sets the provided color.
 * Before calling this method the caller shall know what is the current resolution.
 */
void tvc_set_paper(color_or_index c);

/**
 * Fills the provided color_or_index union according to the previously set
 * paper value, depending on the current resolution
 */
void tvc_get_paper(color_or_index *retVal);

/**
 * Sets the current ink (character foreground color).
 * In case of 2 or 4 color modes this method only sets the provided palette
 * index, in case of 16 color mode this method sets the provided color.
 * Before calling this method the caller shall know what is the current resolution.
 */
void tvc_set_ink(color_or_index c);

/**
 * Fills the provided color_or_index union according to the previously set
 * ink value, depending on the current resolution
 */
void tvc_get_ink(color_or_index *retVal);


/** Graphics related routines */

enum line_mode {LM_NORM=0, LM_OR, LM_AND, LM_XOR};

/**
 * Sets the L_STYLE system variable. Valid values are 0-16.
 * 
 */
void __LIB__ tvc_set_linestyle(char line_style) __z88dk_fastcall;

/**
 * Sets the L_MODE system variable. Valid values are 0-3.
 * 
 */
void __LIB__ tvc_set_linemode(enum line_mode lm) __z88dk_fastcall;

/**
 * Gets the L_STYLE system variable.
 * 
 */
char __LIB__ tvc_get_linestyle();

/**
 * Gets the L_MODE system variable.
 * 
 */
char __LIB__ tvc_get_linemode();

/**
 * Sets the memory mapping of TVC. Please make sure that U0 is mapped to PAGE0.
 * One MMAP_P0_, one MMAP_P1_, one MMAP_P2_ and one MMAP_P3_ value must be selected
 * and OR'ed (added) together!
 */
void __LIB__ tvc_set_memorymap(char map) __z88dk_fastcall;

/**
 *  Maps in the videoRAM to the addr 0x8000-0xbfff
 */
void __LIB__ tvc_mapin_vram() __z88dk_fastcall;

/**
 *  Maps out the videoRAM so U3 RAM to be used on addr 0x8000-0xbfff
 */
void __LIB__ tvc_mapout_vram() __z88dk_fastcall;

#endif
