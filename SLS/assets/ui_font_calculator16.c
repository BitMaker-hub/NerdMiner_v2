/*******************************************************************************
 * Size: 16 px
 * Bpp: 1
 * Opts: --bpp 1 --size 16 --font /home/valerio/SquareLine/assets/Calculator.ttf -o /home/valerio/SquareLine/assets/ui_font_calculator16.c --format lvgl -r 0x20-0x7f --no-compress --no-prefilter
 ******************************************************************************/

#include "../ui.h"

#ifndef UI_FONT_CALCULATOR16
#define UI_FONT_CALCULATOR16 1
#endif

#if UI_FONT_CALCULATOR16

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xd0,

    /* U+0022 "\"" */
    0xfc,

    /* U+0023 "#" */
    0x12, 0x4, 0x81, 0x23, 0xb7, 0x12, 0x4, 0x8e,
    0xdc, 0x48, 0x12, 0x4, 0x80,

    /* U+0024 "$" */
    0x21, 0x1d, 0x5a, 0x52, 0x8e, 0x29, 0x4b, 0x57,
    0x10, 0x80,

    /* U+0025 "%" */
    0x1, 0xc, 0x22, 0x48, 0x49, 0x9, 0x20, 0xc8,
    0x1, 0x0, 0x46, 0x1, 0x21, 0x24, 0x44, 0x88,
    0x61, 0x0,

    /* U+0026 "&" */
    0x1c, 0x1, 0x10, 0x8, 0x80, 0x44, 0x1, 0x40,
    0xe, 0x0, 0x63, 0x87, 0x87, 0x64, 0x46, 0x14,
    0x20, 0x1, 0x5, 0x28, 0x46, 0x20, 0x0, 0xc0,
    0x0,

    /* U+0027 "'" */
    0xf8,

    /* U+0028 "(" */
    0x16, 0xaa, 0xaa, 0xaa, 0x54,

    /* U+0029 ")" */
    0x2a, 0x55, 0x55, 0x56, 0xa8,

    /* U+002A "*" */
    0x4a, 0xd0,

    /* U+002B "+" */
    0x10, 0x20, 0x47, 0x71, 0x2, 0x4, 0x0,

    /* U+002C "," */
    0xe0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x8, 0x44, 0x20, 0x10, 0x84, 0x2, 0x11, 0x8,
    0x0,

    /* U+0030 "0" */
    0x7a, 0x39, 0x65, 0x94, 0xa, 0x69, 0xc7, 0x17,
    0x80,

    /* U+0031 "1" */
    0x6a, 0x22, 0x22, 0x22, 0x22, 0x50,

    /* U+0032 "2" */
    0x7a, 0x10, 0x41, 0x5, 0xe8, 0x20, 0x82, 0x7,
    0x80,

    /* U+0033 "3" */
    0xf0, 0x42, 0x10, 0xf8, 0x21, 0x8, 0x7c,

    /* U+0034 "4" */
    0x86, 0x18, 0x61, 0x85, 0xe0, 0x41, 0x4, 0x10,
    0x40,

    /* U+0035 "5" */
    0x7a, 0x8, 0x20, 0x81, 0xe0, 0x41, 0x6, 0x17,
    0x80,

    /* U+0036 "6" */
    0x7a, 0x8, 0x20, 0x81, 0xe8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0037 "7" */
    0xf0, 0x42, 0x10, 0x84, 0x21, 0x8, 0x42,

    /* U+0038 "8" */
    0x7a, 0x18, 0x61, 0x85, 0xe8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0039 "9" */
    0x7a, 0x18, 0x61, 0x85, 0xe0, 0x41, 0x4, 0x17,
    0x80,

    /* U+003A ":" */
    0x82,

    /* U+003B ";" */
    0x83, 0x0,

    /* U+003C "<" */
    0x12, 0x4c, 0x88, 0x42, 0x11,

    /* U+003D "=" */
    0xf0, 0xf0,

    /* U+003E ">" */
    0x84, 0x23, 0x11, 0x24, 0x88,

    /* U+003F "?" */
    0xf0, 0x42, 0x10, 0x88, 0x84, 0x20, 0x8,

    /* U+0040 "@" */
    0x7f, 0xa0, 0x18, 0x6, 0x9, 0x8c, 0x64, 0x99,
    0x26, 0x49, 0x8d, 0xa0, 0x8, 0x1, 0xfe,

    /* U+0041 "A" */
    0x7a, 0x18, 0x61, 0x85, 0xe8, 0x61, 0x86, 0x18,
    0x40,

    /* U+0042 "B" */
    0xf4, 0xa3, 0x19, 0x32, 0x51, 0x8c, 0xbc,

    /* U+0043 "C" */
    0x7a, 0x18, 0x60, 0x80, 0x8, 0x20, 0x86, 0x17,
    0x80,

    /* U+0044 "D" */
    0xf2, 0x28, 0x61, 0x84, 0x18, 0x61, 0x86, 0x2f,
    0x0,

    /* U+0045 "E" */
    0x7c, 0x21, 0x8, 0x7e, 0x10, 0x84, 0x1e,

    /* U+0046 "F" */
    0x7c, 0x21, 0x8, 0x3e, 0x10, 0x84, 0x20,

    /* U+0047 "G" */
    0x79, 0xa, 0x14, 0x8, 0x3, 0x61, 0x42, 0x85,
    0x9, 0xe0,

    /* U+0048 "H" */
    0x86, 0x18, 0x61, 0x85, 0xe8, 0x61, 0x86, 0x18,
    0x40,

    /* U+0049 "I" */
    0xd9, 0x8, 0x42, 0x0, 0x84, 0x21, 0x36,

    /* U+004A "J" */
    0x6e, 0x20, 0x40, 0x81, 0x0, 0x4, 0x48, 0x91,
    0x21, 0x80,

    /* U+004B "K" */
    0x84, 0x65, 0x4c, 0x62, 0x92, 0x94, 0x60,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0x84, 0x1e,

    /* U+004D "M" */
    0xcf, 0x3d, 0x6d, 0x86, 0x18, 0x61, 0x86, 0x18,
    0x40,

    /* U+004E "N" */
    0x87, 0x1c, 0x69, 0xa4, 0x49, 0x65, 0x8e, 0x18,
    0x40,

    /* U+004F "O" */
    0x7a, 0x18, 0x61, 0x84, 0x8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0050 "P" */
    0x7a, 0x18, 0x61, 0x85, 0xe8, 0x20, 0x82, 0x8,
    0x0,

    /* U+0051 "Q" */
    0x79, 0xa, 0x14, 0x28, 0x40, 0x21, 0x4a, 0x8d,
    0xf, 0xf8,

    /* U+0052 "R" */
    0x79, 0xa, 0x14, 0x28, 0x4f, 0x22, 0x42, 0x85,
    0x6, 0x0,

    /* U+0053 "S" */
    0x7a, 0x18, 0x20, 0x81, 0xe0, 0x41, 0x6, 0x17,
    0x80,

    /* U+0054 "T" */
    0xee, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,
    0x20, 0x40,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x61, 0x86, 0x17,
    0x80,

    /* U+0056 "V" */
    0x1, 0x6, 0x14, 0x24, 0x49, 0xa, 0x14, 0x30,

    /* U+0057 "W" */
    0x86, 0x18, 0x61, 0x86, 0x1b, 0x6d, 0xcf, 0x38,
    0x40,

    /* U+0058 "X" */
    0x9a, 0xa6, 0x6, 0xaa, 0x90,

    /* U+0059 "Y" */
    0x8c, 0x94, 0xa0, 0x10, 0x84, 0x21, 0x0,

    /* U+005A "Z" */
    0xf8, 0x42, 0x21, 0x19, 0x88, 0x84, 0x3e,

    /* U+005B "[" */
    0x72, 0x48, 0x24, 0x91, 0x80,

    /* U+005C "\\" */
    0x84, 0x20, 0x84, 0x20, 0x84, 0x0, 0x84, 0x10,
    0x80,

    /* U+005D "]" */
    0xc4, 0x92, 0x9, 0x25, 0x80,

    /* U+005E "^" */
    0x6a, 0xa9,

    /* U+005F "_" */
    0xff, 0x80,

    /* U+0060 "`" */
    0x99, 0x80,

    /* U+0061 "a" */
    0xf0, 0x42, 0xf8, 0xf8,

    /* U+0062 "b" */
    0x82, 0x8, 0x20, 0x81, 0xe8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0063 "c" */
    0x7c, 0x21, 0x8, 0x3c,

    /* U+0064 "d" */
    0x4, 0x10, 0x41, 0x5, 0xe8, 0x61, 0x86, 0x17,
    0x80,

    /* U+0065 "e" */
    0x7a, 0x1f, 0xa0, 0x81, 0xe0,

    /* U+0066 "f" */
    0x1c, 0x82, 0x8, 0x23, 0x62, 0x8, 0x20, 0x82,
    0x0,

    /* U+0067 "g" */
    0x7a, 0x18, 0x61, 0x85, 0xe0, 0x41, 0x78,

    /* U+0068 "h" */
    0x82, 0x8, 0x20, 0x81, 0xe8, 0x61, 0x86, 0x18,
    0x40,

    /* U+0069 "i" */
    0xfc,

    /* U+006A "j" */
    0x11, 0x11, 0x11, 0x11, 0x19, 0xe0,

    /* U+006B "k" */
    0x84, 0x21, 0x3b, 0x62, 0x92, 0x8c, 0x0,

    /* U+006C "l" */
    0xff, 0xe0,

    /* U+006D "m" */
    0x7d, 0x26, 0x4c, 0x99, 0x32, 0x40,

    /* U+006E "n" */
    0x7a, 0x18, 0x61, 0x86, 0x10,

    /* U+006F "o" */
    0x7a, 0x18, 0x61, 0x85, 0xe0,

    /* U+0070 "p" */
    0x7a, 0x18, 0x61, 0x85, 0xe8, 0x20, 0x80,

    /* U+0071 "q" */
    0x79, 0xa, 0x14, 0x28, 0x4f, 0x1, 0x3, 0x6,

    /* U+0072 "r" */
    0x78, 0x88, 0x88,

    /* U+0073 "s" */
    0x78, 0x88, 0xf1, 0x1f,

    /* U+0074 "t" */
    0x21, 0x8, 0x42, 0x6c, 0x84, 0x21, 0x8,

    /* U+0075 "u" */
    0x99, 0x99, 0x96,

    /* U+0076 "v" */
    0x19, 0xa6, 0x40,

    /* U+0077 "w" */
    0xad, 0x6b, 0x5a, 0xfc,

    /* U+0078 "x" */
    0x8a, 0x80, 0xa8, 0x80,

    /* U+0079 "y" */
    0x9, 0xaa, 0x64, 0x48, 0x80,

    /* U+007A "z" */
    0xf1, 0x24, 0x8f,

    /* U+007B "{" */
    0x1c, 0x82, 0x8, 0x20, 0x84, 0x20, 0xc1, 0x82,
    0x8, 0x20, 0x82, 0x7,

    /* U+007C "|" */
    0xff, 0xfe,

    /* U+007D "}" */
    0xf0, 0x41, 0x4, 0x10, 0x40, 0x81, 0x8, 0x61,
    0x4, 0x10, 0x41, 0x38,

    /* U+007E "~" */
    0x62, 0xae, 0x20
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 75, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 37, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 62, .box_w = 2, .box_h = 3, .ofs_x = 1, .ofs_y = 13},
    {.bitmap_index = 4, .adv_w = 173, .box_w = 10, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 17, .adv_w = 112, .box_w = 5, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 27, .adv_w = 199, .box_w = 11, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 45, .adv_w = 234, .box_w = 13, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 70, .adv_w = 37, .box_w = 1, .box_h = 5, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 71, .adv_w = 59, .box_w = 2, .box_h = 19, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 76, .adv_w = 59, .box_w = 2, .box_h = 19, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 81, .adv_w = 86, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 83, .adv_w = 125, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 90, .adv_w = 37, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 91, .adv_w = 73, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 92, .adv_w = 37, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 112, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 102, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 111, .adv_w = 86, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 117, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 126, .adv_w = 106, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 133, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 160, .adv_w = 105, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 167, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 176, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 185, .adv_w = 37, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 186, .adv_w = 38, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 188, .adv_w = 87, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 193, .adv_w = 98, .box_w = 4, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 195, .adv_w = 87, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 200, .adv_w = 105, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 187, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 222, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 231, .adv_w = 109, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 122, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 106, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 263, .adv_w = 106, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 135, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 280, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 289, .adv_w = 90, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 296, .adv_w = 135, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 306, .adv_w = 107, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 313, .adv_w = 105, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 320, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 329, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 338, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 347, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 356, .adv_w = 138, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 366, .adv_w = 122, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 376, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 385, .adv_w = 128, .box_w = 7, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 404, .adv_w = 141, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 412, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 421, .adv_w = 87, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 426, .adv_w = 91, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 433, .adv_w = 109, .box_w = 5, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 440, .adv_w = 61, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 445, .adv_w = 109, .box_w = 5, .box_h = 13, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 454, .adv_w = 61, .box_w = 3, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 459, .adv_w = 89, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 9},
    {.bitmap_index = 461, .adv_w = 172, .box_w = 9, .box_h = 1, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 463, .adv_w = 62, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 10},
    {.bitmap_index = 465, .adv_w = 110, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 478, .adv_w = 105, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 482, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 491, .adv_w = 113, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 496, .adv_w = 105, .box_w = 6, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 505, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 512, .adv_w = 112, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 37, .box_w = 1, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 522, .adv_w = 85, .box_w = 4, .box_h = 11, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 528, .adv_w = 100, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 535, .adv_w = 37, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 537, .adv_w = 144, .box_w = 7, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 543, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 548, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 553, .adv_w = 112, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 560, .adv_w = 135, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 568, .adv_w = 81, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 571, .adv_w = 84, .box_w = 4, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 575, .adv_w = 86, .box_w = 5, .box_h = 11, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 582, .adv_w = 90, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 585, .adv_w = 91, .box_w = 4, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 588, .adv_w = 112, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 592, .adv_w = 102, .box_w = 5, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 596, .adv_w = 90, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = -4},
    {.bitmap_index = 601, .adv_w = 98, .box_w = 4, .box_h = 6, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 604, .adv_w = 111, .box_w = 6, .box_h = 16, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 616, .adv_w = 37, .box_w = 1, .box_h = 15, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 618, .adv_w = 110, .box_w = 6, .box_h = 16, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 630, .adv_w = 144, .box_w = 7, .box_h = 3, .ofs_x = 1, .ofs_y = 5}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t ui_font_calculator16 = {
#else
lv_font_t ui_font_calculator16 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 20,          /*The maximum line height required by the font*/
    .base_line = 4,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -2,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if UI_FONT_CALCULATOR16*/

