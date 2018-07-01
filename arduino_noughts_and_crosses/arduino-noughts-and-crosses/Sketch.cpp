/*Begining of Auto generated code by Atmel studio */
#include <Arduino.h>

/*End of auto generated code by Atmel studio */

/***************************************************
 Touchscreen Noughts and Crosses Project

 Using Adafruit TFT Capacitive Touch Shield and Arduino Uno

 WIP:
 - Quit button
 - Settings
   - Game wins for match win
 - Match win banner
 - Bug
 - Minimise libraries, also maybe rename
 - Consider better implmentations

 Written by Sam Ellis
 ****************************************************/

#include <SPI.h>    // Communication with display
#include <Adafruit_GFX.h>   // Graphics library
#include <Adafruit_ILI9341.h>   // tftDisplay display library
#include <SD.h> // Library for accessing SD cards, needed for loading bitmaps
#include <Wire.h>   // Needed by FT6206
#include <Adafruit_FT6206.h>    // Capacitive touchscreen library

// New type for keeping track of game state
enum State {
	empty, nought, cross
};

//Beginning of Auto generated function prototypes by Atmel Studio
void startScreen();
void playMatch(int maxGames);
State game(uint8_t noughtsScore, uint8_t crossesScore);
void updateScore(int noughtsScore, int crossesScore);
State addMove(uint8_t square, TS_Point marker, State player);
TS_Point getPoint();
void drawBitmap(char fileInput, int16_t x, int16_t y);
uint16_t read16(File& f);
uint32_t read32(File& f);
//End of Auto generated function prototypes by Atmel Studio

// Debugging
#define verbose false

// Use hardware SPI (on 13, 12, 11) and below for CS/DC
#define tftDisplay_DC 9
#define tftDisplay_CS 10
Adafruit_ILI9341 tftDisplay = Adafruit_ILI9341(tftDisplay_CS, tftDisplay_DC);

// Touchscreen (uses I2C, on SCL/SDA)
Adafruit_FT6206 touchScreen = Adafruit_FT6206();

// SD card CS pin
#define SD_CS 4

// Image drawing pixel buffer
#define BUFFPIXEL 20

// Array of arrays of three indexes in a row on a 3x3 grid
uint8_t winIndexes[][3] = { {0, 4, 8}, {2, 4, 6}, {0, 1, 2}, {3, 4, 5},
{6, 7, 8}, {0, 3, 6}, {1, 4, 7}, {2, 5, 8}};
	
// These read 16-bit and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
	uint16_t result;
	((uint8_t *) &result)[0] = f.read(); // LSB
	((uint8_t *) &result)[1] = f.read(); // MSB
	return result;
}

uint32_t read32(File &f) {
	uint32_t result;
	((uint8_t *) &result)[0] = f.read(); // LSB
	((uint8_t *) &result)[1] = f.read();
	((uint8_t *) &result)[2] = f.read();
	((uint8_t *) &result)[3] = f.read(); // MSB
	return result;
}

void drawBitmap(char fileInput, int16_t x, int16_t y) {

	File bmpFile;
	int bmpWidth, bmpHeight;   // W+H in pixels
	uint8_t bmpDepth;              // Bit depth (currently must be 24)
	uint32_t bmpImageoffset;        // Start of image data in file
	uint32_t rowSize;               // Not always = bmpWidth; may have padding
	uint8_t sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
	uint8_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
	boolean goodBmp = false;       // Set to true on valid header parse
	boolean flip = true;        // BMP is stored bottom-to-top
	int w, h, row, col, x2, y2, bx1, by1;
	uint8_t r, g, b;
	uint32_t pos = 0, startTime = millis();
	char filename[6] = {fileInput, '.', 'b', 'm', 'p'};

	if ((x >= tftDisplay.width()) || (y >= tftDisplay.height())) return;

	SerialUSB.println();
	SerialUSB.print(F("Loading image '"));
	SerialUSB.print(filename);
	SerialUSB.println('\'');

	// Todo (BROKEN)
	// Open requested file on SD card
	if ((bmpFile = SD.open(filename))) {
		SerialUSB.println(F("File (maybe) not found"));
		// return;
	}

	// Parse BMP header
	SerialUSB.println(F("Start parse"));
	if (read16(bmpFile) == 0x4D42) { // BMP signature
		SerialUSB.print(F("File size: "));
		SerialUSB.println(read32(bmpFile));
		(void) read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		SerialUSB.print(F("Image Offset: "));
		SerialUSB.println(bmpImageoffset, DEC);
		// Read DIB header
		SerialUSB.print(F("Header size: "));
		SerialUSB.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
			SerialUSB.print(F("Bit Depth: "));
			SerialUSB.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
				SerialUSB.print(F("Image size: "));
				SerialUSB.print(bmpWidth);
				SerialUSB.print('x');
				SerialUSB.println(bmpHeight);

				// BMP rows are padded (if needed) to 4-byte boundary
				rowSize = (bmpWidth * 3 + 3) & ~3;

				// If bmpHeight is negative, image is in top-down order.
				// This is not canon but has been observed in the wild.
				if (bmpHeight < 0) {
					bmpHeight = -bmpHeight;
					flip = false;
				}

				// Crop area to be loaded
				x2 = x + bmpWidth - 1; // Lower-right corner
				y2 = y + bmpHeight - 1;
				if ((x2 >= 0) && (y2 >= 0)) { // On screen?
					w = bmpWidth; // Width/height of section to load/display
					h = bmpHeight;
					bx1 = by1 = 0; // UL coordinate in BMP file
					if (x < 0) { // Clip left
						bx1 = -x;
						x = 0;
						w = x2 + 1;
					}
					if (y < 0) { // Clip top
						by1 = -y;
						y = 0;
						h = y2 + 1;
					}
					if (x2 >= tftDisplay.width()) w = tftDisplay.width() - x; // Clip right
					if (y2 >= tftDisplay.height()) h = tftDisplay.height() - y; // Clip bottom

					// Set tftDisplay address window to clipped image bounds
					tftDisplay.startWrite(); // Requires start/end transaction now
					tftDisplay.setAddrWindow(x, y, w, h);

					for (row = 0; row < h; row++) { // For each scanline...

						// Seek to start of scan line.  It might seem labor-
						// intensive to be doing this on every line, but this
						// method covers a lot of gritty details like cropping
						// and scanline padding.  Also, the seek only takes
						// place if the file position actually needs to change
						// (avoids a lot of cluster math in SD library).
						if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
						pos = bmpImageoffset + (bmpHeight - 1 - (row + by1)) * rowSize;
						else
						// Bitmap is stored top-to-bottom
						pos = bmpImageoffset + (row + by1) * rowSize;
						pos += bx1 * 3; // Factor in starting column (bx1)
						if (bmpFile.position() != pos) { // Need seek?
							tftDisplay.endWrite(); // End tftDisplay transaction
							bmpFile.seek(pos);
							buffidx = sizeof(sdbuffer); // Force buffer reload
							tftDisplay.startWrite(); // Start new tftDisplay transaction
						}
						for (col = 0; col < w; col++) { // For each pixel...
							// Time to read more pixel data?
							if (buffidx >= sizeof(sdbuffer)) { // Indeed
								tftDisplay.endWrite(); // End tftDisplay transaction
								bmpFile.read(sdbuffer, sizeof(sdbuffer));
								buffidx = 0; // Set index to beginning
								tftDisplay.startWrite(); // Start new tftDisplay transaction
							}
							// Convert pixel from BMP to tftDisplay format, push to display
							b = sdbuffer[buffidx++];
							g = sdbuffer[buffidx++];
							r = sdbuffer[buffidx++];
							tftDisplay.writePixel(tftDisplay.color565(r, g, b));
						} // end pixel
					} // end scanline
					tftDisplay.endWrite(); // End last tftDisplay transaction
				} // end onscreen
				SerialUSB.print(F("Loaded in "));
				SerialUSB.print(millis() - startTime);
				SerialUSB.println(F(" ms"));
			} // end goodBmp
		}
	}

	bmpFile.close();
	if (!goodBmp)
	SerialUSB.println(F("BMP format not recognized or SD not mounted"));
	SerialUSB.println();
}

TS_Point getPoint() {
	TS_Point point = touchScreen.getPoint();
	// convert point to match display coordinate system
	point.x = map(point.x, 0, 240, 240, 0);
	point.y = map(point.y, 0, 320, 320, 0);
	return point;
}

// Function allows recycling
State addMove(uint8_t square, TS_Point marker, State player) {
	char bitmap;
	SerialUSB.print(F("addMove, player = "));
	SerialUSB.println(player);
	if (player == cross) {
		SerialUSB.println(F("this might be a cross"));
		if (square % 2) {
			bitmap = 'j';
			} else {
			bitmap = 'k';
		}
		player = nought;
		drawBitmap(bitmap, marker.x, marker.y);
		} else {
		SerialUSB.println(F("this might be a nought"));
		if (square % 2) {
			bitmap = 'h';
			} else {
			bitmap = 'i';
		}
		player = cross;
		drawBitmap(bitmap, marker.x - 2, marker.y);
	}
	return player;
}

void updateScore(int noughtsScore, int crossesScore) {
	String s = (String) noughtsScore;
	char newScore = s.charAt(0);
	drawBitmap(newScore, 95, 29);
	s = (String) crossesScore;
	newScore = s.charAt(0);
	drawBitmap(newScore, 132, 28);
}

State game(uint8_t noughtsScore, uint8_t crossesScore) {
	State player = nought;
	State boardState[9] = {empty, empty, empty, empty, empty, empty, empty, empty,
	empty};
	State winner = empty;
	uint8_t placedCounters = 0;
	uint8_t square = 255;
	TS_Point marker;

	// Draw grid
	drawBitmap('d', 0, 80);

	// Start match
	while (placedCounters < 9) {
		while (true) {
			if (!touchScreen.touched()) continue;
			TS_Point point = getPoint();
			if (verbose)
			SerialUSB.println(
			(String) F("screen pressed at: (") + (String) point.x + ","
			+ (String) point.y + (String) F(")"));
			if (point.y > 79 and point.y < 161) {
				if (point.x < 81) {
					square = 0;
					marker.x = 15;
					} else if (point.x < 161) {
					square = 1;
					marker.x = 95;
					} else if (point.x < 240) {
					square = 2;
					marker.x = 175;
				}
				marker.y = 95;

				} else if (point.y > 160 and point.y < 241) {
				if (point.x < 81) {
					square = 3;
					marker.x = 15;
					} else if (point.x < 161) {
					square = 4;
					marker.x = 95;
					} else if (point.x < 240) {
					square = 5;
					marker.x = 175;
				}
				marker.y = 175;

				} else if (point.y > 240) {
				if (point.x < 81) {
					square = 6;
					marker.x = 15;
					} else if (point.x < 161) {
					square = 7;
					marker.x = 95;
					} else if (point.x < 240) {
					square = 8;
					marker.x = 175;
				}
				marker.y = 255;
			}
			if (!(square == 255)) break;
		}

		// Check to see if selected square is occupied
		if (!(boardState[square] == empty)) continue;

		// Update board state
		boardState[square] = player;

		// Draw nought or cross on selected square and swap player
		player = addMove(square, marker, player);
		placedCounters++;

		// Check to see if someone has won
		for (uint8_t i = 0; i < 8; i++) {
			if (boardState[winIndexes[i][0]] == boardState[winIndexes[i][1]]
			and boardState[winIndexes[i][0]] == boardState[winIndexes[i][2]]
			and boardState[winIndexes[i][0]] != empty) {
				winner = boardState[winIndexes[i][0]];
			}
		}
		SerialUSB.print(F("winner = "));
		SerialUSB.println(winner);
		if (!(winner == empty)) break;
		delay(100);
	}
	return winner;
}

void playMatch(int maxGames) {
	uint8_t noughtsScore = 0;
	uint8_t crossesScore = 0;
	uint8_t gamesPlayed = 0;

	// Draw score bar for match
	drawBitmap('a', 0, 0);
	drawBitmap('c', 118, 32);
	updateScore(noughtsScore, crossesScore);
	drawBitmap('b', 81, 10);

	while (gamesPlayed < maxGames) {
		State winner = game(noughtsScore, crossesScore);
		gamesPlayed++;

		// Draw win banner
		char bitmap;
		switch (winner) {
			case cross:
			// Crosses wins
			bitmap = 'e';
			crossesScore++;
			break;
			case nought:
			// Noughts wins
			bitmap = 'g';
			noughtsScore++;
			break;
			case empty:
			// Game is a draw
			bitmap = 'f';
			break;
		}
		drawBitmap(bitmap, 0, 110);

		updateScore(noughtsScore, crossesScore);
		while (true)
		if (touchScreen.touched()) break;
	}
}

void startScreen() {
	uint8_t maxGames = 5;
	drawBitmap('l', 0, 0);

	while (true) {
		if (touchScreen.touched()) {
			TS_Point point = getPoint();
			if (point.x > 24 and point.x < 216 and point.y > 204 and point.y < 256) {
				// They pressed the start button
				break;
				} else {
				continue;
			}
			} else {
			continue;
		}
	}

	playMatch(maxGames);

}

void setup() {

	SerialUSB.begin(9600);

	tftDisplay.begin();

	if (!touchScreen.begin()) {
		SerialUSB.println(F("Couldn't start FT6206 touchscreen controller"));
		SerialUSB.println(F("Driver might not have been found"));
		while (true);
	}

	if (verbose) SerialUSB.println(F("Display and touchscreen started"));

	if (!SD.begin(SD_CS)) {
		SerialUSB.println(F("Failed to initialise SD card"));
	} else if (verbose) SerialUSB.println(F("SD card mounted"));

	// tests();

	startScreen();

}

void loop() {

	if (!touchScreen.touched()) return;

	//  TS_Point point = getPoint();
	//
	//  SerialUSB.println(
	//      "screen pressed at: (" + (String) point.x + "," + (String) point.y + ")");
	SerialUSB.println(F("loooop"));
	delay(100);

}