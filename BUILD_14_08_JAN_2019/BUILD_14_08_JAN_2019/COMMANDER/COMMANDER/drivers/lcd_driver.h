
#ifndef LCD_DRIVER_H_
#define LCD_DRIVER_H_

#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>
#include "Binaray.h"
#include "conf_lcd.h"
#include "yalgo.h"

							
// commands
#define LCD_CLEARDISPLAY					0x01
#define LCD_RETURNHOME						0x02
#define LCD_ENTRYMODESET					0x04
#define LCD_DISPLAYCONTROL					0x08
#define LCD_CURSORSHIFT						0x10
#define LCD_FUNCTIONSET						0x20
#define LCD_SETCGRAMADDR					0x40
#define LCD_SETDDRAMADDR					0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT						0x00
#define LCD_ENTRYLEFT						0x02
#define LCD_ENTRYSHIFTINCREMENT				0x01
#define LCD_ENTRYSHIFTDECREMENT				0x00

// flags for display on/off control
#define LCD_DISPLAYON						0x04
#define LCD_DISPLAYOFF						0x00
#define LCD_CURSORON						0x02
#define LCD_CURSOROFF						0x00
#define LCD_BLINKON							0x01
#define LCD_BLINKOFF						0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE						0x08
#define LCD_CURSORMOVE						0x00
#define LCD_MOVERIGHT						0x04
#define LCD_MOVELEFT						0x00

// flags for function set
#define LCD_8BITMODE						0x10
#define LCD_4BITMODE						0x00
#define LCD_2LINE							0x08
#define LCD_1LINE							0x00
#define LCD_5x10DOTS						0x04
#define LCD_5x8DOTS							0x00





uint8_t _rs_pin;										
uint8_t _rw_pin;
uint8_t _enable_pin;
uint8_t _data_pins[4];

uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;

uint8_t _initialized;

uint8_t _numlines;
uint8_t _row_offsets[4];

void LCD_init(void);
void LCD_PWR_CONFIG(void);
void LCD_PWR_EN(void);
void LCD_PWR_DIS(void);
void LCD_setRowOffsets(int row0, int row1, int row2, int row3);
void LCD_clear(void);
void LCD_home(void);
void LCD_noDisplay(void);
void LCD_display(void);
void LCD_noBlink(void);
void LCD_blink(void);
void LCD_noCursor(void);
void LCD_cursor(void);
void LCD_scrollDisplayLeft(void);
void LCD_scrollDisplayRight(void);
void LCD_leftToRight(void);
void LCD_rightToLeft(void);
void LCD_autoscroll(void);
void LCD_noAutoscroll(void);
void LCD_Create_Custom_createChar(uint8_t, uint8_t[]);
void LCD_setCursor(uint8_t, uint8_t);
size_t LCD_write(uint8_t);
void command(uint8_t);
void send(uint8_t, uint8_t);
void write4bits(uint8_t);
void pulseEnable(void);

void lcd_printf(const char *fmt, ...);


//////////////////////////////////////////////////////////////////////////
   
   size_t Buffer_writer(const char *buffer);
   size_t LCD_printNumber(unsigned long, uint8_t);
   size_t LCD_printFloat(double, uint8_t);
   size_t print_ch_array(const char[]);
   size_t print_ch(char);
   size_t print_uch(unsigned char);
   size_t print_int(int);
   size_t print_uint(unsigned int);
   size_t print_ln(long n);
   size_t print_uln(unsigned long n);
   
   
   #define LCD_Print(a)   _Generic(a,								\
								   const char*:print_ch_array,		\
								   char:print_ch,					\
								   unsigned char:print_uch,			\
								   int:print_int,					\
								   unsigned int:print_uint,			\
								   long:print_ln,					\
								   unsigned long:print_uln)(a)
   
   
#endif 