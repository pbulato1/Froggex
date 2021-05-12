#ifndef HT16K33_H_
#define HT16K33_H_
#include "sl_i2cspm.h"

#define OFF 								0
#define RED 								1
#define GREEN 								2
#define YELLOW 							3

#define HT16K33_SLAVE_ADDRESS 	0xE0
#define HT16K33_BLINK_CMD 			0x80
#define HT16K33_BLINK_DISPLAYON 	0x01
#define HT16K33_BLINK_OFF 				0
#define HT16K33_BLINK_2HZ  			1
#define HT16K33_BLINK_1HZ  			2
#define HT16K33_BLINK_HALFHZ  		3
#define HT16K33_CMD_BRIGHTNESS 	0xE0

void HT16K33_init(void);
void HT16K33_setBrightness(uint8_t b);
void HT16K33_blinkRate(uint8_t b);
void HT16K33_writeDisplay(void);
void HT16K33_clear(void);
void HT16K33_putPixel(uint8_t x, uint8_t y, uint8_t color);

extern uint8_t display_buffer[17];
#endif 
