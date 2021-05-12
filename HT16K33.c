#include "HT16K33.h"

static I2C_TransferSeq_TypeDef seq;
static uint8_t dataW;
static const uint8_t xPos[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
uint8_t display_buffer[17];

void HT16K33_init(void)
{
	dataW = 0x21;
	seq.addr = HT16K33_SLAVE_ADDRESS;
	seq.buf[0].data = &dataW;
	seq.buf[0].len = 1;
	seq.flags = I2C_FLAG_WRITE;
	I2CSPM_Transfer(I2C1, &seq);
	HT16K33_blinkRate(0);
	HT16K33_setBrightness(15);
	HT16K33_clear();
}

void HT16K33_setBrightness(uint8_t b)
{
	if (b > 15) b = 15;
	b |= HT16K33_CMD_BRIGHTNESS;
	dataW = b;
	seq.buf[0].data = &dataW;
	seq.buf[0].len = 1;
	I2CSPM_Transfer(I2C1, &seq);
}

void HT16K33_blinkRate(uint8_t b)
{
	if (b > 3) b = 0; 									// turn off if not sure
	b = b << 1;
	b |= HT16K33_BLINK_CMD | HT16K33_BLINK_DISPLAYON;
	dataW = b;
	seq.buf[0].data = &dataW;
	seq.buf[0].len = 1;
	I2CSPM_Transfer(I2C1, &seq);
}

void HT16K33_writeDisplay(void)
{
	seq.buf[0].data = display_buffer;
	seq.buf[0].len = 17;
	I2CSPM_Transfer(I2C1, &seq);
}

void HT16K33_clear(void)
{
	for (uint8_t i=0; i<=16; i++) 
		display_buffer[i] = 0;
	HT16K33_writeDisplay();
}

void HT16K33_putPixel(uint8_t x, uint8_t y, uint8_t color)
{
	y = (y<<1) + 1;
	x = xPos[x];
	switch (color) {
		case OFF:
			display_buffer[y] &= ~x;					// Turn off green and red LED.
			display_buffer[y+1] &= ~x;
		break;

		case RED:
			display_buffer[y] &= ~x;					// Turn off green LED.
			display_buffer[y+1] |= x;					// Turn on red LED.
		break;

		case GREEN:
			display_buffer[y+1] &= ~x;					// Turn off red LED.
			display_buffer[y] |= x;						// Turn on green LED.
		break;

		case YELLOW:
			display_buffer[y] |= x;						// Turn on green and red LED.
			display_buffer[y+1] |= x;
		break;
	}
}
