#include <LiquidCrystal.h>
#include <Keyboard.h>
#include <avr/pgmspace.h>

// Input module
#define PIN_LATCH 6
#define PIN_BIT0 5
#define PIN_BIT1 4
#define PIN_BIT2 3
#define PIN_BIT3 2
// LCD 
#define PIN_LCD_RS 13
#define PIN_LCD_EN 12
#define PIN_LCD_D4 11
#define PIN_LCD_D5 10
#define PIN_LCD_D6 9
#define PIN_LCD_D7 8
// modifier bitmask
#define MOD_CTRL 0x01
#define MOD_SHIFT 0x02
#define MOD_ALT 0x04
#define MOD_GUI 0x08
// state values (iState)
#define STATE_HI 0
#define STATE_LO 1
// sprites for display
uint8_t arrowUp[] = {
  0b00000,
  0b00100,
  0b01110,
  0b10101,
  0b00100,
  0b00100,
  0b00000,
  0b00000,
};
uint8_t arrowDown[] = {
  0b00000,
  0b00100,
  0b00100,
  0b10101,
  0b01110,
  0b00100,
  0b00000,
  0b00000,
};
uint8_t modifier[] = {
  0b00000,
  0b00100,
  0b01110,
  0b11111,
  0b01110,
  0b00100,
  0b00000,
  0b00000,
};
uint8_t keyGui[] = {
  0b00000,
  0b11011,
  0b11011,
  0b00000,
  0b11011,
  0b11011,
  0b00000,
  0b00000,
};
uint8_t overscore[] = {
  0b11111,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
};
// LCD object
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

// Global variables
// Key state tracking
uint8_t curByte = 0;
uint8_t modStatus = 0;

// Rising edge detection
uint8_t prevLatch = 0;

// State tracking (switching nibbles, outputting bytes)
uint8_t iState = STATE_HI;

// Utility functions
void trySetMod(uint8_t mod, uint8_t modKey) {
  if (!(modStatus & mod))
    Keyboard.press(modKey);
}
void tryUnsetMod(uint8_t mod, uint8_t modKey) {
  if (!(modStatus & mod))
    Keyboard.release(modKey);
}

// Main display function
void scancodeDisplay() {
  // Clear the display and reset the cursor
	lcd.clear();
	lcd.setCursor(0, 0);
	
  // Get and print the bits of curByte
	uint8_t x = curByte;
	lcd.rightToLeft();
	lcd.setCursor(7, 0);
	for (uint8_t i = 0; i < 8; i++) {
		lcd.print((char) ((x & 1) + 0x30));
		x >>= 1;
	}

  // Print the key symbol
	lcd.leftToRight();
	lcd.setCursor(9, 0);
	lcd.print('[');
  switch (curByte) {
    // special cases: delete, arrow keys
    case 0x7F: lcd.print("^?"); break;
    case 0x80: lcd.print((char) 0); break;
    case 0x81: lcd.print((char) 1); break;
    case 0x82: lcd.print((char) 0x7F); break;
    case 0x83: lcd.print((char) 0x7E); break;
    case 0x90: lcd.print("\x02\x43"); break;
    case 0x91: lcd.print("\x02\x08"); break;
    case 0x92: lcd.print("\x02\x41"); break;
    case 0x93: lcd.print("\x02\x03"); break;
    case 0x94: lcd.print("\x02""CLR"); break;
    default: {
      if (curByte < 32) {
        lcd.print('^');
        lcd.print((char) (curByte + 0x40));
      }
      else if (curByte >= 0xF0 && curByte <= 0xFB) {
        int num = curByte - 0xEF;
        lcd.print('F');
        lcd.print(num);
      }
      else if (curByte >= 0x80) {
        lcd.print("???");
      }
      else {
        lcd.print((char) curByte);
      }
    } break;
  }
	lcd.print(']');

  // Underline the current nibble
	switch (iState) {
		case STATE_HI: {
			lcd.setCursor(0, 1);
		} break;
		case STATE_LO: {
			lcd.setCursor(4, 1);
		} break;
	}
	lcd.print(F("\x04\x04\x04\x04"));

  // Print the modifier status
  lcd.setCursor(12, 1);
  lcd.print((modStatus & MOD_CTRL)? 'C' : ' ');
  lcd.print((modStatus & MOD_SHIFT)? '\x00' : ' ');
  lcd.print((modStatus & MOD_ALT)? 'A' : ' ');
  lcd.print((modStatus & MOD_GUI)? '\x03' : ' ');

  // Wait for a bit before executing next function
	delay(10);
}

void interpretByte(uint8_t b) {
  switch (b) {
    // Special cases for bytes
    case 0x00: break;
    case 0x1B: {
      // escape
      Keyboard.press(KEY_ESC);
      delay(50);
      Keyboard.release(KEY_ESC);
    } break;
    case 0x7F: {
      // delete
      Keyboard.press(KEY_DELETE);
      delay(50);
      Keyboard.release(KEY_DELETE);
    } break;
    case 0x0A:
    case 0x09:
    case 0x08: {
      // tab, LF, backspace
      Keyboard.print((char) b);
    } break;
    // Arrow keys
    case 0x80:
    case 0x81:
    case 0x82:
    case 0x83: {
      int key = KEY_UP_ARROW - (b - 0x80);
      Keyboard.press(key);
      delay(50);
      Keyboard.release(key);
    } break;
    // Modifier keys
    case 0x90: {
      modStatus ^= MOD_CTRL;
      if (modStatus & MOD_CTRL)
        Keyboard.press(KEY_LEFT_CTRL);
      else
        Keyboard.release(KEY_LEFT_CTRL);
    } break;
    case 0x91: {
      modStatus ^= MOD_SHIFT;
      if (modStatus & MOD_SHIFT)
        Keyboard.press(KEY_LEFT_SHIFT);
      else
        Keyboard.release(KEY_LEFT_SHIFT);
    } break;
    case 0x92: {
      modStatus ^= MOD_ALT;
      if (modStatus & MOD_ALT)
        Keyboard.press(KEY_LEFT_ALT);
      else
        Keyboard.release(KEY_LEFT_ALT);
    } break;
    case 0x93: {
      modStatus ^= MOD_GUI;
      if (modStatus & MOD_GUI)
        Keyboard.press(KEY_LEFT_GUI);
      else
        Keyboard.release(KEY_LEFT_GUI);
    } break;
    // Modifier clear
    case 0x94: {
      if (modStatus & MOD_CTRL)
        Keyboard.release(KEY_LEFT_CTRL);
      if (modStatus & MOD_SHIFT)
        Keyboard.release(KEY_LEFT_SHIFT);
      if (modStatus & MOD_ALT)
        Keyboard.release(KEY_LEFT_ALT);
      if (modStatus & MOD_GUI)
        Keyboard.release(KEY_LEFT_GUI);

      modStatus = 0;
    } break;
    default: {
      if (b < 32) {
        // any control char = Ctrl-(b+0x60)
        trySetMod(MOD_CTRL, KEY_LEFT_CTRL);
        Keyboard.press(b + 0x60);
        delay(50);
        tryUnsetMod(MOD_CTRL, KEY_LEFT_CTRL);
        Keyboard.release(b + 0x60);
      } else if (b >= 0xF0 && b <= 0xFB) {
        Keyboard.press(KEY_F1 + (b - 0xF0));
      } else if (b >= 0x80) {}
      else {
        Keyboard.print((char) b);
        // special keys (e.g. arrow) do not unpress shift.
        // Keyboard.print is liable to releasing Shift, so we just assume
        // it always does.
        if (modStatus & MOD_SHIFT) {
          modStatus &= ~MOD_SHIFT;
          Keyboard.release(KEY_LEFT_SHIFT);
        }
      }
    }
  }
}

void setup() {
  // Setup the pins
	pinMode(PIN_LATCH, INPUT);
	pinMode(PIN_BIT0, INPUT);
	pinMode(PIN_BIT1, INPUT);
	pinMode(PIN_BIT2, INPUT);
	pinMode(PIN_BIT3, INPUT);

  // Set up and initialize the LCD
	lcd.noAutoscroll();
	lcd.noBlink();
	lcd.noCursor();
	lcd.begin(16, 2);

  // Load custom glyphs to LCD memory
  lcd.createChar(0, arrowUp);
  lcd.createChar(1, arrowDown);
  lcd.createChar(2, modifier);
  lcd.createChar(3, keyGui);
  lcd.createChar(4, overscore);
  
  // Initialize keyboard emulation
	Keyboard.begin();
}


void loop() {
	// accumulate bits
	uint8_t lo4 = digitalRead(PIN_BIT3);
	lo4 = (lo4 << 1) | digitalRead(PIN_BIT2);
	lo4 = (lo4 << 1) | digitalRead(PIN_BIT1);
	lo4 = (lo4 << 1) | digitalRead(PIN_BIT0);
  // Set the low or high nibble depending on state!
	switch (iState) {
		case STATE_HI: {
			curByte = (lo4 << 4) | (curByte & 0x0F);
		} break;
		case STATE_LO: {
			curByte = (lo4) | (curByte & 0xF0);
		} break;
	}

	// Rising-edge trigger: "latch" button
	uint8_t curLatch = digitalRead(PIN_LATCH);
	if (curLatch && !prevLatch) {
		switch (iState) {
			case STATE_HI: {
				iState = STATE_LO;
			} break;
			case STATE_LO: {
				iState = STATE_HI;
				interpretByte(curByte);
				curByte = 0;
			} break;
		}
	}
	prevLatch = curLatch;

  // Display the status
	scancodeDisplay();
}