/*
 * USB RetroPad Adapter - PC/PS3 USB adapter for retro-controllers!
 * Copyright (c) 2012 Bruno Freitas - bruno@brunofreitas.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <WProgram.h>
#include "USBVirtuaStick.h"
#include "PS2Pad.h"
#include "genesis.h"
#include "saturn.h"
#include "NESPad.h"
#include "GCPad_16Mhz.h"
#include "tg16.h"

// Arcade mode detection pin
#define ARCADE_DB9_PIN	12

// Extension cable detection pins
#define DETPIN0 6  // DB9P2
#define DETPIN1	8  // DB9P4
#define DETPIN2	9  // DB9P6
#define DETPIN3	10 // DB9P7
#define DETPIN4	11 // DB9P9

// Possible values (as of today) returned by the detectPad() routine
// Normal pads
#define PAD_ARCADE		-1
#define PAD_GENESIS		0b00111
#define PAD_NES 		0b00110
#define PAD_SNES 		0b00101
#define PAD_PS2 		0b00100
#define PAD_GC	 		0b00011
#define PAD_N64			0b00010
#define PAD_NEOGEO		0b00001
#define PAD_WIICC		0b00000
// Extended pads (uses DB9 pin 4 and/or 2 for identification)
#define PAD_SATURN		0b01111
#define PAD_TG16		0b10111
#define PAD_DFU_DONGLE	0b01110 // Reserved for USBRA DFU dongle


// Pad directions vector
byte pad_dir[16] = {8, 2, 6, 8, 4, 3, 5, 8, 0, 1, 7, 8, 8, 8, 8, 8};

/*
 * This is the new auto-detect function (non jumper based) which detects the extension
 * cable plugged in the DB9 port. It uses grounded pins from DB9 (4, 6, 7 and 9) for
 * the detection.
 *
 *  -1 - Arcade
 * 00111 - Sega Genesis (Default)
 * 00110 - NES
 * 00101 - SNES
 * 00100 - PS2
 * 00011 - Game Cube
 * 00010 - Nintendo 64
 * 00001 - Neo Geo
 * 00000 - Reserved 1
 * 01111 - Sega Saturn
 * 10111 - TurboGrafx 16
 */
int detectPad() {
	int pad;

	// Set pad/arcade detection pins as input, turning pull-ups on
	pinMode(DETPIN0, INPUT);
	digitalWrite(DETPIN0, HIGH);

	pinMode(DETPIN1, INPUT);
	digitalWrite(DETPIN1, HIGH);

	pinMode(DETPIN2, INPUT);
	digitalWrite(DETPIN2, HIGH);

	pinMode(DETPIN3, INPUT);
	digitalWrite(DETPIN3, HIGH);

	pinMode(DETPIN4, INPUT);
	digitalWrite(DETPIN4, HIGH);

	pinMode(ARCADE_DB9_PIN, INPUT);
	digitalWrite(ARCADE_DB9_PIN, HIGH);

	// Check switch for Arcade position
	if(digitalRead(ARCADE_DB9_PIN))
		return PAD_ARCADE;

	pad = (!digitalRead(DETPIN0) << 4) | (!digitalRead(DETPIN1) << 3) | (digitalRead(DETPIN2) << 2) | (digitalRead(DETPIN3) << 1) | (digitalRead(DETPIN4));

	if((pad >> 3) & 0b11) {
		switch(pad) {
		case 0b11011:
		case 0b10111:
			return PAD_TG16;
			break;
		case 0b11111:
		case 0b01111:
			return PAD_SATURN;
			break;
		case 0b11100:
			return PAD_PS2;
			break;
		default:
			return PAD_GENESIS;
			break;
		}
	}

	return (pad & 0b111);
}

void setup() {
	// Initialize USB joystick driver
	vs_init(true);
}

void genesis_loop() {
	int button_data;

	genesis_init();

	for (;;) {
		vs_reset_watchdog();

		button_data = genesis_read();

		gamepad_state.l_x_axis = 0x80;
		gamepad_state.l_y_axis = 0x80;

		if(button_data & GENESIS_LEFT) {
			gamepad_state.l_x_axis = 0x00;
		} else if (button_data & GENESIS_RIGHT) {
			gamepad_state.l_x_axis = 0xFF;
		}

		if(button_data & GENESIS_UP) {
			gamepad_state.l_y_axis = 0x00;
		} else if (button_data & GENESIS_DOWN) {
			gamepad_state.l_y_axis = 0xFF;
		}

		gamepad_state.square_btn = (button_data & GENESIS_A) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data & GENESIS_B) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.circle_btn = (button_data & GENESIS_C) > 0;
		gamepad_state.circle_axis = (gamepad_state.circle_btn ? 0xFF : 0x00);

		gamepad_state.l1_btn = (button_data & GENESIS_X) > 0;
		gamepad_state.l1_axis = (gamepad_state.l1_btn ? 0xFF : 0x00);

		gamepad_state.triangle_btn = (button_data & GENESIS_Y) > 0;
		gamepad_state.triangle_axis = (gamepad_state.triangle_btn ? 0xFF : 0x00);

		gamepad_state.r1_btn = (button_data & GENESIS_Z) > 0;
		gamepad_state.r1_axis = (gamepad_state.r1_btn ? 0xFF : 0x00);

		gamepad_state.select_btn = (button_data & GENESIS_MODE) > 0;

		gamepad_state.start_btn = (button_data & GENESIS_START) > 0;

		gamepad_state.ps_btn = (button_data & GENESIS_UP) && (button_data & GENESIS_START);

		vs_send_pad_state();
	}
}

void arcade_loop() {
	int button_data;

	NESPad::init(6, 7, 13);

	for (;;) {
		vs_reset_watchdog();

		button_data = NESPad::read(16);

		gamepad_state.l_x_axis = 0x80;
		gamepad_state.l_y_axis = 0x80;

		if(button_data & 0x01) {
			gamepad_state.l_y_axis = 0x00;
		} else if (button_data & 0x02) {
			gamepad_state.l_y_axis = 0xFF;
		}


		if(button_data & 0x04) {
			gamepad_state.l_x_axis = 0x00;
		} else if (button_data & 0x08) {
			gamepad_state.l_x_axis = 0xFF;
		}

		gamepad_state.square_btn = (button_data & 0x10) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data & 0x20) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.triangle_btn = (button_data & 0x40) > 0;
		gamepad_state.triangle_axis = (gamepad_state.triangle_btn ? 0xFF : 0x00);

		gamepad_state.circle_btn = (button_data & 0x80) > 0;
		gamepad_state.circle_axis = (gamepad_state.circle_btn ? 0xFF : 0x00);

		gamepad_state.l1_btn = (button_data & 0x100) > 0;
		gamepad_state.l1_axis = (gamepad_state.l1_btn ? 0xFF : 0x00);

		gamepad_state.r1_btn = (button_data & 0x200) > 0;
		gamepad_state.r1_axis = (gamepad_state.r1_btn ? 0xFF : 0x00);

		gamepad_state.l2_btn = (button_data & 0x400) > 0;
		gamepad_state.l2_axis = (gamepad_state.l2_btn ? 0xFF : 0x00);

		gamepad_state.r2_btn = (button_data & 0x800) > 0;
		gamepad_state.r2_axis = (gamepad_state.r2_btn ? 0xFF : 0x00);

		gamepad_state.select_btn = (button_data & 0x1000) > 0;

		gamepad_state.start_btn = (button_data & 0x2000) > 0;

		gamepad_state.l3_btn = (button_data & 0x4000) > 0;

		gamepad_state.ps_btn = (button_data & 0x8000) > 0;

		vs_send_pad_state();
	}
}

void nes_loop() {
	int button_data;

	NESPad::init(5, 6, 7);

	for (;;) {
		vs_reset_watchdog();

		button_data = NESPad::read(8);

		gamepad_state.l_x_axis = 0x80;
		gamepad_state.l_y_axis = 0x80;

		if(button_data & 64) {
			gamepad_state.l_x_axis = 0x00;
		} else if (button_data & 128) {
			gamepad_state.l_x_axis = 0xFF;
		}

		if(button_data & 16) {
			gamepad_state.l_y_axis = 0x00;
		} else if (button_data & 32) {
			gamepad_state.l_y_axis = 0xFF;
		}

		gamepad_state.square_btn = (button_data & 2) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data & 1) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.select_btn = (button_data & 4) > 0;

		gamepad_state.start_btn = (button_data & 8) > 0;

		gamepad_state.ps_btn = (button_data & 4) && (button_data & 8); // SELECT + START = PS Button

		vs_send_pad_state();
	}
}

void snes_loop() {
	int button_data;

	NESPad::init(5, 6, 7);

	for (;;) {
		vs_reset_watchdog();

		button_data = NESPad::read(16);

		gamepad_state.l_x_axis = 0x80;
		gamepad_state.l_y_axis = 0x80;

		if(button_data & 64) {
			gamepad_state.l_x_axis = 0x00;
		} else if (button_data & 128) {
			gamepad_state.l_x_axis = 0xFF;
		}

		if(button_data & 16) {
			gamepad_state.l_y_axis = 0x00;
		} else if (button_data & 32) {
			gamepad_state.l_y_axis = 0xFF;
		}

		gamepad_state.square_btn = (button_data & 2) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data & 1) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.circle_btn = (button_data & 256) > 0;
		gamepad_state.circle_axis = (gamepad_state.circle_btn ? 0xFF : 0x00);

		gamepad_state.l1_btn = (button_data & 1024) > 0;
		gamepad_state.l1_axis = (gamepad_state.l1_btn ? 0xFF : 0x00);

		gamepad_state.triangle_btn = (button_data & 512) > 0;
		gamepad_state.triangle_axis = (gamepad_state.triangle_btn ? 0xFF : 0x00);

		gamepad_state.r1_btn = (button_data & 2048) > 0;
		gamepad_state.r1_axis = (gamepad_state.r1_btn ? 0xFF : 0x00);

		gamepad_state.select_btn = (button_data & 4) > 0;

		gamepad_state.start_btn = (button_data & 8) > 0;

		gamepad_state.ps_btn = (button_data & 4) && (button_data & 8); // SELECT + START = PS Button

		vs_send_pad_state();
	}
}

void ps2_loop() {
	byte dir = 0;

	while (PS2Pad::init(true)) {
		vs_reset_watchdog();
		delayMicroseconds(10000); // 10ms delay
		vs_send_pad_state();
	}

	for (;;) {
		vs_reset_watchdog();

		PS2Pad::read();

		if(PS2Pad::type() == 0) {
			gamepad_state.r_x_axis = 0x80;
			gamepad_state.r_y_axis = 0x80;

			if(PS2Pad::button(PSB_PAD_LEFT)) {
				gamepad_state.l_x_axis = 0x00;
			} else if (PS2Pad::button(PSB_PAD_RIGHT)) {
				gamepad_state.l_x_axis = 0xFF;
			} else {
				gamepad_state.l_x_axis = 0x80;
			}

			if(PS2Pad::button(PSB_PAD_UP)) {
				gamepad_state.l_y_axis = 0x00;
			} else if (PS2Pad::button(PSB_PAD_DOWN)) {
				gamepad_state.l_y_axis = 0xFF;
			} else {
				gamepad_state.l_y_axis = 0x80;
			}

		} else {
			gamepad_state.l_x_axis = PS2Pad::stick(PSS_LX);
			gamepad_state.l_y_axis = PS2Pad::stick(PSS_LY);
			gamepad_state.r_x_axis = PS2Pad::stick(PSS_RX);
			gamepad_state.r_y_axis = PS2Pad::stick(PSS_RY);

			dir = PS2Pad::button(PSB_PAD_UP) << 3 | PS2Pad::button(PSB_PAD_DOWN) << 2 | PS2Pad::button(PSB_PAD_LEFT) << 1 | PS2Pad::button(PSB_PAD_RIGHT);

			gamepad_state.direction = pad_dir[dir];
		}

		gamepad_state.square_btn = PS2Pad::button(PSB_SQUARE);
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = PS2Pad::button(PSB_CROSS);
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.circle_btn = PS2Pad::button(PSB_CIRCLE);
		gamepad_state.circle_axis = (gamepad_state.circle_btn ? 0xFF : 0x00);

		gamepad_state.l1_btn =PS2Pad::button(PSB_L1);
		gamepad_state.l1_axis = (gamepad_state.l1_btn ? 0xFF : 0x00);

		gamepad_state.l2_btn =PS2Pad::button(PSB_L2);
		gamepad_state.l2_axis = (gamepad_state.l2_btn ? 0xFF : 0x00);

		gamepad_state.triangle_btn = PS2Pad::button(PSB_TRIANGLE);
		gamepad_state.triangle_axis = (gamepad_state.triangle_btn ? 0xFF : 0x00);

		gamepad_state.r1_btn = PS2Pad::button(PSB_R1);
		gamepad_state.r1_axis = (gamepad_state.r1_btn ? 0xFF : 0x00);

		gamepad_state.r2_btn = PS2Pad::button(PSB_R2);
		gamepad_state.r2_axis = (gamepad_state.r2_btn ? 0xFF : 0x00);

		gamepad_state.l3_btn = PS2Pad::button(PSB_L3);

		gamepad_state.r3_btn = PS2Pad::button(PSB_R3);

		gamepad_state.select_btn = PS2Pad::button(PSB_SELECT);

		gamepad_state.start_btn = PS2Pad::button(PSB_START);

		gamepad_state.ps_btn = PS2Pad::button(PSB_SELECT) && PS2Pad::button(PSB_START); // SELECT + START = PS Button

		vs_send_pad_state();
	}
}

void gc_loop() {
	byte *button_data;
	byte dir = 0;

	while(GCPad_init() == 0) {
		vs_reset_watchdog();
		delayMicroseconds(10000); // 10ms delay
		vs_send_pad_state();
	}

	for(;;) {
		vs_reset_watchdog();

		button_data = GCPad_read();

		dir = ((button_data[1] & 0x08) > 0) << 3 | ((button_data[1] & 0x04) > 0) << 2 | ((button_data[1] & 0x01) > 0) << 1 | ((button_data[1] & 0x02) > 0);

		gamepad_state.direction = pad_dir[dir];

		gamepad_state.square_btn = (button_data[0] & 0x08) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data[0] & 0x02) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.triangle_btn = (button_data[0] & 0x04) > 0;
		gamepad_state.triangle_axis = (gamepad_state.triangle_btn ? 0xFF : 0x00);

		gamepad_state.circle_btn = (button_data[0] & 0x01) > 0;
		gamepad_state.circle_axis = (gamepad_state.circle_btn ? 0xFF : 0x00);

		gamepad_state.start_btn = (button_data[0] & 0x10) > 0;

		gamepad_state.l1_btn = (button_data[1] & 0x40) > 0;
		gamepad_state.l1_axis = (gamepad_state.l1_btn ? 0xFF : 0x00);

		gamepad_state.r1_btn = (button_data[1] & 0x20) > 0;
		gamepad_state.r1_axis = (gamepad_state.r1_btn ? 0xFF : 0x00);

		gamepad_state.l2_btn = (button_data[1] & 0x10) > 0;
		gamepad_state.l2_axis = (gamepad_state.l2_btn ? 0xFF : 0x00);

		gamepad_state.ps_btn = (button_data[1] & 0x08) && (button_data[0] & 0x10); // UP + START = PS button

		gamepad_state.l_x_axis = map(button_data[2], 32, 223, 0, 255);
		gamepad_state.l_y_axis = map((byte)~button_data[3], 32, 223, 0, 255);
		gamepad_state.r_x_axis = map(button_data[4], 32, 223, 0, 255);
		gamepad_state.r_y_axis = map((byte)~button_data[5], 32, 223, 0, 255);

		gamepad_state.slider = 0x80 - (button_data[6] >> 1) + (button_data[7] >> 1);

		vs_send_pad_state();

		delayMicroseconds(5000);
	}
}

void n64_loop() {
	byte *button_data;
	byte dir = 0;

	while(GCPad_init() == 0) {
		vs_reset_watchdog();
		delayMicroseconds(10000); // 10ms delay
		vs_send_pad_state();
	}

	for(;;) {
		vs_reset_watchdog();

		button_data = N64Pad_read();

		dir = ((button_data[0] & 0x08) > 0) << 3 | ((button_data[0] & 0x04) > 0) << 2 | ((button_data[0] & 0x02) > 0) << 1 | ((button_data[0] & 0x01) > 0);

		gamepad_state.direction = pad_dir[dir];

		gamepad_state.square_btn = (button_data[0] & 0x40) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data[0] & 0x80) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.start_btn = (button_data[0] & 0x10) > 0;

		gamepad_state.l1_btn = (button_data[1] & 0x20) > 0;
		gamepad_state.l1_axis = (gamepad_state.l1_btn ? 0xFF : 0x00);

		gamepad_state.r1_btn = (button_data[1] & 0x10) > 0;
		gamepad_state.r1_axis = (gamepad_state.r1_btn ? 0xFF : 0x00);

		gamepad_state.l2_btn = (button_data[0] & 0x20) > 0;
		gamepad_state.l2_axis = (gamepad_state.l2_btn ? 0xFF : 0x00);

		gamepad_state.ps_btn = (button_data[0] & 0x08) && (button_data[0] & 0x10);  // UP + START = PS button

		gamepad_state.l_x_axis = ((button_data[2] >= 128) ? button_data[2] - 128 : button_data[2] + 128);
		gamepad_state.l_y_axis = ~((button_data[3] >= 128) ? button_data[3] - 128 : button_data[3] + 128);

		// Fix analog logical data since N64 pad doesn't use the full range
		gamepad_state.l_x_axis = map(gamepad_state.l_x_axis, 32, 223, 0, 255);
		gamepad_state.l_y_axis = map(gamepad_state.l_y_axis, 32, 223, 0, 255);

		gamepad_state.r_x_axis = 0x80;
		gamepad_state.r_y_axis = 0x80;

		if(button_data[1] & 0x08) { // C Up
			gamepad_state.r_y_axis = 0x00;
		} else if(button_data[1] & 0x04) { // C Down
			gamepad_state.r_y_axis = 0xFF;
		}

		if(button_data[1] & 0x02) { // C Left
			gamepad_state.r_x_axis = 0x00;;
		} else if(button_data[1] & 0x01) { // C Right
			gamepad_state.r_x_axis = 0xFF;
		}

		vs_send_pad_state();
	}
}

void neogeo_loop() {
	int button_data;

	NESPad::init(5, 6, 7);

	for (;;) {
		vs_reset_watchdog();

		button_data = NESPad::read(16);

		gamepad_state.l_x_axis = 0x80;
		gamepad_state.l_y_axis = 0x80;

		if(button_data & 0x02) {
			gamepad_state.l_x_axis = 0x00; // LEFT
		} else if (button_data & 0x800) {
			gamepad_state.l_x_axis = 0xFF; // RIGHT
		}

		if(button_data & 0x04) {
			gamepad_state.l_y_axis = 0x00; // UP
		} else if (button_data & 0x1000) {
			gamepad_state.l_y_axis = 0xFF; // DOWN
		}

		gamepad_state.square_btn = (button_data & 0x8000) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data & 0x01) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.circle_btn = (button_data & 0x400) > 0;
		gamepad_state.circle_axis = (gamepad_state.circle_btn ? 0xFF : 0x00);

		gamepad_state.triangle_btn = (button_data & 0x200) > 0; // D button is also 0x2000
		gamepad_state.triangle_axis = (gamepad_state.triangle_btn ? 0xFF : 0x00);

		gamepad_state.select_btn = (button_data & 0x100) > 0;

		gamepad_state.start_btn = (button_data & 0x4000) > 0;

		gamepad_state.ps_btn = (button_data & 0x100) && (button_data & 0x4000); // SELECT + START = PS Button

		vs_send_pad_state();
	}
}

void saturn_loop() {
	int button_data;

	saturn_init();

	for (;;) {
		vs_reset_watchdog();

		button_data = saturn_read();

		gamepad_state.l_x_axis = 0x80;
		gamepad_state.l_y_axis = 0x80;

		if(button_data & SATURN_LEFT) {
			gamepad_state.l_x_axis = 0x00;
		} else if (button_data & SATURN_RIGHT) {
			gamepad_state.l_x_axis = 0xFF;
		}

		if(button_data & SATURN_UP) {
			gamepad_state.l_y_axis = 0x00;
		} else if (button_data & SATURN_DOWN) {
			gamepad_state.l_y_axis = 0xFF;
		}

		gamepad_state.square_btn = (button_data & SATURN_A) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data & SATURN_B) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.circle_btn = (button_data & SATURN_C) > 0;
		gamepad_state.circle_axis = (gamepad_state.circle_btn ? 0xFF : 0x00);

		gamepad_state.l1_btn = (button_data & SATURN_X) > 0;
		gamepad_state.l1_axis = (gamepad_state.l1_btn ? 0xFF : 0x00);

		gamepad_state.triangle_btn = (button_data & SATURN_Y) > 0;
		gamepad_state.triangle_axis = (gamepad_state.triangle_btn ? 0xFF : 0x00);

		gamepad_state.r1_btn = (button_data & SATURN_Z) > 0;
		gamepad_state.r1_axis = (gamepad_state.r1_btn ? 0xFF : 0x00);

		gamepad_state.l2_btn = (button_data & SATURN_L) > 0;
		gamepad_state.l2_axis = (gamepad_state.l2_btn ? 0xFF : 0x00);

		gamepad_state.r2_btn = (button_data & SATURN_R) > 0;
		gamepad_state.r2_axis = (gamepad_state.r2_btn ? 0xFF : 0x00);

		gamepad_state.start_btn = (button_data & SATURN_START) > 0;

		gamepad_state.ps_btn = (button_data & SATURN_UP) && (button_data & SATURN_START);

		vs_send_pad_state();
	}
}

void tg16_loop(void) {
	int button_data;

	tg16_init();

	for (;;) {
		vs_reset_watchdog();

		button_data = tg16_read();

		gamepad_state.l_x_axis = 0x80;
		gamepad_state.l_y_axis = 0x80;

		if(button_data & (1 << TG16_LEFT)) {
			gamepad_state.l_x_axis = 0x00;
		} else if (button_data & (1 << TG16_RIGHT)) {
			gamepad_state.l_x_axis = 0xFF;
		}

		if(button_data & (1 << TG16_UP)) {
			gamepad_state.l_y_axis = 0x00;
		} else if (button_data & (1 << TG16_DOWN)) {
			gamepad_state.l_y_axis = 0xFF;
		}

		gamepad_state.square_btn = (button_data & (1 << TG16_II)) > 0;
		gamepad_state.square_axis = (gamepad_state.square_btn ? 0xFF : 0x00);

		gamepad_state.cross_btn = (button_data & (1 << TG16_I)) > 0;
		gamepad_state.cross_axis = (gamepad_state.cross_btn ? 0xFF : 0x00);

		gamepad_state.start_btn = (button_data & (1 << TG16_RUN)) > 0;

		gamepad_state.select_btn = (button_data & (1 << TG16_SELECT)) > 0;

		gamepad_state.ps_btn = gamepad_state.start_btn && gamepad_state.select_btn;

		vs_send_pad_state();
	}
}

void unsupported_pad(void) {
	for(;;);
}

void loop() {
	switch (detectPad()) {
	case PAD_ARCADE:
		arcade_loop();
		break;
	case PAD_NES:
		nes_loop();
		break;
	case PAD_SNES:
		snes_loop();
		break;
	case PAD_PS2:
		ps2_loop();
		break;
	case PAD_GC:
		gc_loop();
		break;
	case PAD_N64:
		n64_loop();
		break;
	case PAD_NEOGEO:
		neogeo_loop();
		break;
	case PAD_SATURN:
		saturn_loop();
		break;
	case PAD_TG16:
		tg16_loop();
		break;
	case PAD_WIICC:
		unsupported_pad();
		break;
	default:
		genesis_loop();
		break;
	}
}
