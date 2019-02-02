#include <asf.h>
#include "lcd_driver.h"


void LCD_init()
{
	uint8_t cols = LCD_COLS;
	uint8_t lines = LCD_ROWS;
	delay_init();
	_rs_pin     = LCD_RS_PIN;
	_enable_pin = LCD_EN_PIN;
	
	_data_pins[0] = LCD_DATA_LINE_D4_PIN;
	_data_pins[1] = LCD_DATA_LINE_D5_PIN;
	_data_pins[2] = LCD_DATA_LINE_D6_PIN;
	_data_pins[3] = LCD_DATA_LINE_D7_PIN;

	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	
	if (lines > 1)
	{
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;
	LCD_setRowOffsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(_rs_pin,&config_port_pin);
	port_pin_set_config(_enable_pin,&config_port_pin);
	//port_pin_set_config(LCD_BACKLIGHT,&config_port_pin);
	
	for (int i=0; i<4; ++i)
	{
		port_pin_set_config(_data_pins[i],&config_port_pin);
	}
	delay_us(50000);
	port_pin_set_output_level(_rs_pin,LOW);
	port_pin_set_output_level(_enable_pin,LOW);
	//port_pin_set_output_level(LCD_BACKLIGHT,LOW);
	
	 write4bits(0x03);
	 delay_us(4500); 
	 
	 write4bits(0x03);
	 delay_us(4500); 
	
	 write4bits(0x03);
	 delay_us(150);

	 write4bits(0x02);

	 command(LCD_FUNCTIONSET | _displayfunction);
	 
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	LCD_display();
	LCD_clear();
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD_PWR_CONFIG()
{
	struct port_config pin_conf_gsm_status;
	port_get_config_defaults(&pin_conf_gsm_status);
	pin_conf_gsm_status.direction  = PORT_PIN_DIR_OUTPUT;
	pin_conf_gsm_status.input_pull = PORT_PIN_PULL_NONE;
	port_pin_set_config(LCD_PWR_CNTRL, &pin_conf_gsm_status);
}

void LCD_PWR_EN()
{
	port_pin_set_output_level(LCD_PWR_CNTRL,HIGH);
}
void LCD_PWR_DIS()
{
	port_pin_set_output_level(LCD_PWR_CNTRL,LOW);
}

void LCD_setRowOffsets(int row0, int row1, int row2, int row3)
{
	_row_offsets[0] = row0;
	_row_offsets[1] = row1;
	_row_offsets[2] = row2;
	_row_offsets[3] = row3;
}

void write4bits(uint8_t value)
{
	for (int i = 0; i < 4; i++) 
	{
		port_pin_set_output_level(_data_pins[i], (value >> i) & 0x01);
	}

	pulseEnable();
}



void pulseEnable(void)
{
 	port_pin_set_output_level(_enable_pin, LOW);
 	delay_us(1);
 	port_pin_set_output_level(_enable_pin, HIGH);
 	delay_us(1);
 	port_pin_set_output_level(_enable_pin, LOW);
 	delay_us(100);

	//port_pin_set_output_level(_enable_pin, HIGH);
	//delay_us(1);
	//port_pin_set_output_level(_enable_pin, LOW);
	//delay_us(500);
}

inline void command(uint8_t value) 
{
	send(value, LOW);
}

inline size_t LCD_write(uint8_t value)
{
	send(value, HIGH);
	return 1;
}

void send(uint8_t value, uint8_t mode) 
{
	port_pin_set_output_level(_rs_pin, mode);
	
	{
		write4bits(value>>4);
		write4bits(value);
	}
}

void LCD_clear(void)
{
	command(LCD_CLEARDISPLAY);
	delay_us(2000);
}


void LCD_Create_Custom_createChar(uint8_t location, uint8_t charmap[]) 
{
	location &= 0x7;
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++)
	{
		LCD_write(charmap[i]);
	}
}

void LCD_home(void)
{
	command(LCD_RETURNHOME);
	delay_us(2000);
}

void LCD_setCursor(uint8_t col, uint8_t row)
{
	const size_t max_lines = sizeof(_row_offsets) / sizeof(*_row_offsets);
	if ( row >= max_lines ) 
	{
		row = max_lines - 1; 
	}
	if ( row >= _numlines )
	{
		row = _numlines - 1;
	}
	command(LCD_SETDDRAMADDR | (col + _row_offsets[row]));
}

void LCD_noDisplay(void)
{
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_display(void) 
{
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD_noCursor(void) 
{
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_cursor(void) 
{
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

void LCD_noBlink(void) 
{
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCD_blink(void) 
{
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}


void LCD_scrollDisplayLeft(void) 
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LCD_scrollDisplayRight(void) 
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void LCD_leftToRight(void) 
{
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD_rightToLeft(void) 
{
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD_autoscroll(void) 
{
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

void LCD_noAutoscroll(void) 
{
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

//////////////////////////////////////////////////////////////////////////


size_t Buffer_writer(const char *buffer)
{
	size_t size = strlen(buffer);
	size_t n = 0;
	while (size--) {
		if (LCD_write(*buffer++)) n++;
		else break;
	}
	return n;
}

size_t print_ch_array(const char str[])
{
	return Buffer_writer(str);
}

size_t print_ch(char c)
{
	return LCD_write(c);
}

size_t print_uch(unsigned char b)
{
	return print_uln((unsigned long) b);
}
 
size_t print_int(int n)
{
	return print_ln((long) n);
}

size_t print_uint(unsigned int n)
{
	return print_uln((unsigned long) n);
}


size_t print_ln(long n)
{
	int base = 10;
	if (base == 0) {
		return LCD_write(n);
		} else if (base == 10) {
		if (n < 0) {
			int t = print_ch('-');
			n = -n;
			return LCD_printNumber(n, 10) + t;
		}
		return LCD_printNumber(n, 10);
		} else {
		return LCD_printNumber(n, base);
	}
}
 
size_t print_uln(unsigned long n)
{
	int base = 10;
	if (base == 0) return LCD_write(n);
	else return LCD_printNumber(n, base);
}


size_t LCD_printNumber(unsigned long n, uint8_t base) {
	char buf[8 * sizeof(long) + 1]; 
	char *str = &buf[sizeof(buf) - 1];

	*str = '\0';

	if (base < 2) base = 10;

	do {
		unsigned long m = n;
		n /= base;
		char c = m - base * n;
		*--str = c < 10 ? c + '0' : c + 'A' - 10;
	} while(n);

	return Buffer_writer(str);
}

size_t LCD_printFloat(double number, uint8_t digits)
{
	size_t n = 0;

	if (isnan(number)) return Buffer_writer("nan");
	if (isinf(number)) return Buffer_writer("inf");
	if (number > 4294967040.0) return Buffer_writer ("ovf");  
	if (number <-4294967040.0) return Buffer_writer ("ovf"); 

	if (number < 0.0)
	{
		n += print_ch('-');
		number = -number;
	}
	
	double rounding = 0.5;
	for (uint8_t i=0; i<digits; ++i)
	rounding /= 10.0;

	number += rounding;

	unsigned long int_part = (unsigned long)number;
	double remainder = number - (double)int_part;
	n += print_uln(int_part);

	if (digits > 0) {
		n += Buffer_writer(".");
	}

	while (digits-- > 0)
	{
		remainder *= 10.0;
		int toPrint = (int)(remainder);
		n += print_int(toPrint);
		remainder -= toPrint;
	}

	return n;
}


void lcd_printf(const  char *fmt, ...)
{
	int num_chars;
	char *lcd_buff;
	num_chars = strlen(fmt) + 3;
	lcd_buff = (char *)malloc(sizeof(char) * num_chars);
	va_list args;
	va_start(args, fmt);
	vsprintf(lcd_buff,fmt,args);
	Buffer_writer(lcd_buff);
	va_end(args);
	free(lcd_buff);
}


 
