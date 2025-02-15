/*
 * lcd.c
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
 UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards
 Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved

 This library is the continuation of my ITDB02_Graph, ITDB02_Graph16
 and RGB_GLCD libraries for Arduino and chipKit. As the number of
 supported display modules and controllers started to increase I felt
 it was time to make a single, universal library as it will be much
 easier to maintain in the future.

 Basic functionality of this library was origianlly based on the
 demo-code provided by ITead studio (for the ITDB02 modules) and
 NKC Electronics (for the RGB GLCD module/shield).

 This library supports a number of 8bit, 16bit and serial graphic
 displays, and will work with both Arduino, chipKit boards and select
 TI LaunchPads. For a full list of tested display modules and controllers,
 see the document UTFT_Supported_display_modules_&_controllers.pdf.

 When using 8bit and 16bit display modules there are some
 requirements you must adhere to. These requirements can be found
 in the document UTFT_Requirements.pdf.
 There are no special requirements when using serial displays.

 You can find the latest version of the library at
 http://www.RinkyDinkElectronics.com/

 This library is free software; you can redistribute it and/or
 modify it under the terms of the CC BY-NC-SA 3.0 license.
 Please see the included documents for further information.

 Commercial use of this library requires you to buy a license that
 will allow commercial use. This includes using the library,
 modified or not, as a tool to sell products.

 The license applies to all part of the library including the
 examples and tools supplied with the library.
 */
#include <stdio.h>
#include "lcd.h"
#include "note.h"
#include "xil_printf.h"
// Global variables
int fch;
int fcl;
int bch;
int bcl;


struct _current_font cfont;
int scale = 4;


// Write command to LCD controller
void LCD_Write_COM(char VL) {
	Xil_Out32(SPI_DC, 0x0);
	Xil_Out32(SPI_DTR, VL);

	while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK))
		;
	Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}

// Write 8-bit data to LCD controller
void LCD_Write_DATA(char VL) {
	Xil_Out32(SPI_DC, 0x01);
	Xil_Out32(SPI_DTR, VL);

	while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK))
		;
	Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}

// Initialize LCD controller
void initLCD(void) {
	int i;

	// Reset
	LCD_Write_COM(0x01);
	for (i = 0; i < 500000; i++)
		; //Must wait > 5ms

	LCD_Write_COM(0xCB);
	LCD_Write_DATA(0x39);
	LCD_Write_DATA(0x2C);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x34);
	LCD_Write_DATA(0x02);

	LCD_Write_COM(0xCF);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0XC1);
	LCD_Write_DATA(0X30);

	LCD_Write_COM(0xE8);
	LCD_Write_DATA(0x85);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x78);

	LCD_Write_COM(0xEA);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xED);
	LCD_Write_DATA(0x64);
	LCD_Write_DATA(0x03);
	LCD_Write_DATA(0X12);
	LCD_Write_DATA(0X81);

	LCD_Write_COM(0xF7);
	LCD_Write_DATA(0x20);




	LCD_Write_COM(0xC0);   //Power control
	LCD_Write_DATA(0x23);  //VRH[5:0]

	LCD_Write_COM(0xC1);   //Power control
	LCD_Write_DATA(0x10);  //SAP[2:0];BT[3:0]

	LCD_Write_COM(0xC5);   //VCM control
	LCD_Write_DATA(0x3e);  //Contrast
	LCD_Write_DATA(0x28);

	LCD_Write_COM(0xC7);   //VCM control2
	LCD_Write_DATA(0x86);  //--

	LCD_Write_COM(0x36);   // Memory Access Control
	LCD_Write_DATA(0x48);

	LCD_Write_COM(0x3A);
	LCD_Write_DATA(0x55);

	LCD_Write_COM(0xB1);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x18);

	LCD_Write_COM(0xB6);   // Display Function Control
	LCD_Write_DATA(0x08);
	LCD_Write_DATA(0x82);
	LCD_Write_DATA(0x27);

	LCD_Write_COM(0x11);   //Exit Sleep
	for (i = 0; i < 100000; i++)
		;

	LCD_Write_COM(0x29);   //Display on
	LCD_Write_COM(0x2c);

	//for (i = 0; i < 100000; i++);

	// Default color and fonts
	fch = 0xFF;
	fcl = 0xFF;
	bch = 0x00;
	bcl = 0x00;
	setFont(SmallFont);
}

// Set boundary for drawing
void setXY(int x1, int y1, int x2, int y2) {
	LCD_Write_COM(0x2A);
	LCD_Write_DATA(x1 >> 8);
	LCD_Write_DATA(x1);
	LCD_Write_DATA(x2 >> 8);
	LCD_Write_DATA(x2);
	LCD_Write_COM(0x2B);
	LCD_Write_DATA(y1 >> 8);
	LCD_Write_DATA(y1);
	LCD_Write_DATA(y2 >> 8);
	LCD_Write_DATA(y2);
	LCD_Write_COM(0x2C);
}

// Remove boundry
void clrXY(void) {
	setXY(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

// Set foreground RGB color for next drawing
void setColor(u8 r, u8 g, u8 b) {
	// 5-bit r, 6-bit g, 5-bit b
	fch = (r & 0x0F8) | g >> 5;
	fcl = (g & 0x1C) << 3 | b >> 3;
}

// Set background RGB color for next drawing
void setColorBg(u8 r, u8 g, u8 b) {
	// 5-bit r, 6-bit g, 5-bit b
	bch = (r & 0x0F8) | g >> 5;
	bcl = (g & 0x1C) << 3 | b >> 3;
}

// Clear display
void clrScr(void) {
	// Black screen
	setColor(0, 0, 0);

	fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

// Draw horizontal line
void drawHLine(int x, int y, int l) {
	int i;

	if (l < 0) {
		l = -l;
		x -= l;
	}

	setXY(x, y, x + l, y);
	for (i = 0; i < l + 1; i++) {
		LCD_Write_DATA(fch);
		LCD_Write_DATA(fcl);
	}

	clrXY();
}

// Fill a rectangular 
void fillRect(int x1, int y1, int x2, int y2) {
	int i;

	if (x1 > x2)
		swap(int, x1, x2);

	if (y1 > y2)
		swap(int, y1, y2);

	setXY(x1, y1, x2, y2);
	for (i = 0; i < (x2 - x1 + 1) * (y2 - y1 + 1); i++) {
		LCD_Write_DATA(fch);
		LCD_Write_DATA(fcl);
	}

	clrXY();
}

void fillRectBG(int x1, int y1, int x2, int y2) {
	int i;

	if (x1 > x2)
		swap(int, x1, x2);

	if (y1 > y2)
		swap(int, y1, y2);

	setXY(x1, y1, x2, y2);
	for (i = 0; i < (x2 - x1 + 1) * (y2 - y1 + 1); i++) {
		LCD_Write_DATA(bch);
		LCD_Write_DATA(bcl);
	}

	clrXY();
}


// Select the font used by print() and printChar()
void setFont(u8* font) {
	cfont.font = font;
	cfont.x_size = font[0];
	cfont.y_size = font[1];
	cfont.offset = font[2];
	cfont.numchars = font[3];
}




// Print a character
void printChar(u8 c, int x, int y) {
	u8 ch;
	int i, j, pixelIndex;

	setXY(x, y, x + cfont.x_size - 1, y + cfont.y_size - 1);

	pixelIndex = (c - cfont.offset) * (cfont.x_size >> 3) * cfont.y_size + 4;
	for (j = 0; j < (cfont.x_size >> 3) * cfont.y_size; j++) {
		ch = cfont.font[pixelIndex];
		for (i = 0; i < 8; i++) {
			if ((ch & (1 << (7 - i))) != 0) {
				LCD_Write_DATA(fch);
				LCD_Write_DATA(fcl);
			} else {
				LCD_Write_DATA(bch);
				LCD_Write_DATA(bcl);
			}
		}
		pixelIndex++;
	}

	clrXY();
}

inline void print_big_char(u8 c, int x, int y) {
	u8 ch;
	int i, j, pixelIndex;

	setXY(x, y, x + (scale * cfont.x_size - 1), y + (scale * cfont.y_size - 1));

	pixelIndex = (c - cfont.offset) * (cfont.x_size >> 3) * cfont.y_size + 4;
	for (j = 0; j < (cfont.x_size >> 3) * cfont.y_size; j++) {
		ch = cfont.font[pixelIndex];
		for (i = 0; i < 8; i++) {
			if ((ch & (1 << (7 - i))) != 0) {
//				LCD_Write_DATA(fch);
//				LCD_Write_DATA(fcl);
				fillRect(x + scale * i, y + scale * j, x + scale * i + 2, y + scale * j + scale);
			} else {
//				LCD_Write_DATA(bch);
//				LCD_Write_DATA(bcl);
				fillRectBG(x + scale * i, y + scale * j, x + scale * i + 2, y + scale * j + scale);
			}
		}
		pixelIndex++;
	}
	clrXY();
}

// Print string
inline void lcdPrint(char *st, int x, int y) {
	int i = 0;
	while (*st != '\0')
		printChar(*st++, x + cfont.x_size * i++, y);


}

inline void lcdPrintBig(char *st, int x, int y) {
	int i = 0;
	while (*st != '\0')
		print_big_char(*st++, x + cfont.x_size * scale * i++, y);

}

inline void draw_tuner_screen() {
	setColor(0,0,0);
	fillRect(65, 275, 175, 255);
	// set font
//	setFont(SmallFont);
	// set color
//	setColor(160, 150, 150);
	// fillrect
//	fillRect(40, 145, 200, 175);
	// setxy
	setColor(255, 135, 0);
	setColorBg(0, 0, 0);
	scale = 4;
	lcdPrintBig(note_buf, 20, 90);
	scale = 3;
	lcdPrintBig(octave, 140, 140);
	lcdPrintBig(freq_buf, 20, 190);
	lcdPrintBig(cents_buf, 130, 90);
	lcdPrint(a4_buf, 20, 240);
	setColor(255, 135, 0);

	// If +-5 green, +-20 yellow, +-50 red

	if (cents_err <= 5 && cents_err >= -5) {
		setColor(0, 255, 42);
	} else if (cents_err <= 25 && cents_err >= -25) {
		setColor(255, 238, 0);
	} else {
		setColor(255, 0, 0);
	}
//
//	if(5 >= cents_err && cents_err >= -5){
//
//	}
//	else if(25 >= cents_err && cents_err >= -25){
//
//	}
//	else if(26 >= cents_err || cents_err <= -26){
//
//	}

	fillRect(120+cents_err, 275, 120, 255); // draw bar to screen

}

inline void init_tuner_screen() {
	scale = 3;
	setColor(0, 0, 0);
	fillRect(10, 75, 230, 275);
	setColor(255, 135, 0);
	setColorBg(0, 0, 0);
	lcdPrintBig("Tuner", 60, 30);
//	setColorBg(20, 50, 100);
	lcdPrintBig("Oct:", 20, 140);
//	fillRect(40, 275, 200, 40);
	scale = 4;


}
