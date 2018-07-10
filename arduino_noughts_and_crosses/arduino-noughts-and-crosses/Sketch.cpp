
/***************************************************
 Touchscreen Noughts and Crosses Project

Using Adafruit TFT Capacitive Touch Shield and Arduino M0 Pro

Written by Sam Ellis
****************************************************/

#include <Arduino.h> // Required by Atmel Studio
#include <SPI.h>    // Communication with display
#include <Adafruit_GFX.h>   // Graphics library
#include <Adafruit_ILI9341.h>   // tftDisplay display library
#include <SD.h> // Library for accessing SD cards, needed for loading bitmaps
#include <Wire.h>   // Needed by FT6206
#include <Adafruit_FT6206.h>    // Capacitive touchscreen library

// New type for keeping track of game state (only used in classic N&C)
enum State {
	empty, nought, cross
};

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
#define BUFFPIXEL 85

// Array of arrays of three indexes in a row on a 3x3 grid
uint8_t winIndexes[][3] = {{0, 4, 8}, {2, 4, 6}, {0, 1, 2}, {3, 4, 5},
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

	Serial.println();
	Serial.print(F("Loading image '"));
	Serial.print(filename);
	Serial.println('\'');

	// Todo (BROKEN)
	// Open requested file on SD card
	if ((bmpFile = SD.open(filename))) {
		Serial.println(F("File (maybe) not found"));
		// return;
	}

	// Parse BMP header
	Serial.println(F("Start parse"));
	if (read16(bmpFile) == 0x4D42) { // BMP signature
		Serial.print(F("File size: "));
		Serial.println(read32(bmpFile));
		(void) read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print(F("Image Offset: "));
		Serial.println(bmpImageoffset, DEC);
		// Read DIB header
		Serial.print(F("Header size: "));
		Serial.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
			Serial.print(F("Bit Depth: "));
			Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
				Serial.print(F("Image size: "));
				Serial.print(bmpWidth);
				Serial.print('x');
				Serial.println(bmpHeight);

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
				Serial.print(F("Loaded in "));
				Serial.print(millis() - startTime);
				Serial.println(F(" ms"));
			} // end goodBmp
		}
	}

	bmpFile.close();
	if (!goodBmp)
	Serial.println(F("BMP format not recognized or SD not mounted"));
	Serial.println();
}

void drawBitmap(char* filename, int16_t x, int16_t y) {

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

	if ((x >= tftDisplay.width()) || (y >= tftDisplay.height())) return;

	Serial.println();
	Serial.print(F("Loading image '"));
	Serial.print(filename);
	Serial.println('\'');

	// Todo (BROKEN)
	// Open requested file on SD card
	if ((bmpFile = SD.open(filename))) {
		Serial.println(F("File (maybe) not found"));
		// return;
	}

	// Parse BMP header
	Serial.println(F("Start parse"));
	if (read16(bmpFile) == 0x4D42) { // BMP signature
		Serial.print(F("File size: "));
		Serial.println(read32(bmpFile));
		(void) read32(bmpFile); // Read & ignore creator bytes
		bmpImageoffset = read32(bmpFile); // Start of image data
		Serial.print(F("Image Offset: "));
		Serial.println(bmpImageoffset, DEC);
		// Read DIB header
		Serial.print(F("Header size: "));
		Serial.println(read32(bmpFile));
		bmpWidth = read32(bmpFile);
		bmpHeight = read32(bmpFile);
		if (read16(bmpFile) == 1) { // # planes -- must be '1'
			bmpDepth = read16(bmpFile); // bits per pixel
			Serial.print(F("Bit Depth: "));
			Serial.println(bmpDepth);
			if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

				goodBmp = true; // Supported BMP format -- proceed!
				Serial.print(F("Image size: "));
				Serial.print(bmpWidth);
				Serial.print('x');
				Serial.println(bmpHeight);

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
				Serial.print(F("Loaded in "));
				Serial.print(millis() - startTime);
				Serial.println(F(" ms"));
			} // end goodBmp
		}
	}

	bmpFile.close();
	if (!goodBmp)
	Serial.println(F("BMP format not recognized or SD not mounted"));
	Serial.println();
}

TS_Point getPoint() {
	TS_Point point = touchScreen.getPoint();
	 // Map point so that coordinate system starts from top left
	point.x = map(point.x, 0, 240, 240, 0);
	point.y = map(point.y, 0, 320, 320, 0);
	return point;
}

State addMove(uint8_t square, TS_Point marker, State player) {
	char bitmap;
	Serial.print(F("addMove, player = "));
	Serial.println(player);
	if (player == cross) {
		Serial.println(F("this might be a cross"));
		if (square % 2) {
			bitmap = 'j';
			} else {
			bitmap = 'k';
		}
		drawBitmap(bitmap, marker.x, marker.y);
		// Change players
		player = nought;
		} else {
		Serial.println(F("this might be a nought"));
		if (square % 2) {
			bitmap = 'h';
			} else {
			bitmap = 'i';
		}
		drawBitmap(bitmap, marker.x, marker.y);
		// Change players
		player = cross;
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

uint8_t getSquare(TS_Point point) {
	uint8_t square = 255;
	if (point.y > 79 && point.y < 161) {
		if (point.x < 81) {
			square = 0;
			} else if (point.x < 161) {
			square = 1;
			} else if (point.x < 240) {
			square = 2;
		}

		} else if (point.y > 160 && point.y < 241) {
		if (point.x < 81) {
			square = 3;
			} else if (point.x < 161) {
			square = 4;
			} else if (point.x < 240) {
			square = 5;
		}

		} else if (point.y > 240) {
		if (point.x < 81) {
			square = 6;
			} else if (point.x < 161) {
			square = 7;
			} else if (point.x < 240) {
			square = 8;
		}
	}
	return square;
}

TS_Point getCounterPosition(uint8_t square) {
	TS_Point counterPos;
	if (square == 0 || square == 3 || square == 6) {
		counterPos.x = 0;
	} else if (square == 1 || square == 4 || square == 7) {
		counterPos.x = 80;
	} else {
		counterPos.x = 160;
	}
	
	if (square == 0 || square == 1 || square == 2) {
		counterPos.y = 80;
	} else if (square == 3 || square == 4 || square == 5) {
		counterPos.y = 160;
	} else {
		counterPos.y = 240;
	}
	return counterPos;
}

TS_Point getSmallCounterPosition(uint8_t square, uint8_t turn) {
	TS_Point counterPos;
	
	counterPos = getCounterPosition(square);
	
	if (turn == 1 || turn == 4 || turn == 7) {
		counterPos.x += 1;
		} else if (turn == 2 || turn == 5 || turn == 8) {
		counterPos.x += 27;
		} else {
		counterPos.x += 53;
	}
		
	if (turn == 1 || turn == 2 || turn == 3) {
		counterPos.y += 1;
		} else if (turn == 4 || turn == 5 || turn == 6) {
		counterPos.y += 27;
		} else {
		counterPos.y += 53;
	}
	
	return counterPos;
}

void drawSmallCounter(uint8_t square, uint8_t turn) {
	TS_Point counterPos;
	
	counterPos = getSmallCounterPosition(square, turn);
	
	char bg;
	if (square % 2) {
		bg = 'b';
	} else {
		bg = 'w';
	}
	
	char bitmap[8] = {bg, 'c', ((String) turn).charAt(0), '.', 'b', 'm', 'p'};
	drawBitmap(bitmap, counterPos.x, counterPos.y);
}

void findCircle(uint8_t boardState[9][11], uint8_t x, uint8_t y, uint8_t origX, uint8_t subscripts[9]) {
	// For each of the possible small counters in this square
	for (uint8_t i = 0; i < 9; i++) {
		// Find any counters that exist and aren't the current counter
		if (boardState[x][i + 2] != 0 && i + 2 != y) {
			// Find the pair counter of the one found
			// By going through all the squares
			for (uint8_t j = 0; j < 9; j++) {
				// And finding a counter of the same subscript that isn't the current one
				if (boardState[j][i + 2] != 0 && j != x) {
					// Check if this is the original square
					if (j == origX) {
						subscripts[0] = y - 1;
						return;
					}
					// If not repeat for the next counter
					findCircle(boardState, j, i + 2, origX, subscripts);
					// When the function returns, check if it found the original square
					if (subscripts[0] != 255) {
						// If it did add this point on the end
						for (uint8_t k = 0; k < 9; k++) {
							if (subscripts[k] == 255) {
								subscripts[k] = y - 1;
								return;
							}
						}
					}
				}
			}
		}
	}
}

void drawClassicalCounters(uint8_t boardState[9][11], uint8_t x, uint8_t y, uint8_t origX, uint8_t origY) {
	// Draw classic counter
	char bg;
	if (x % 2) {
		bg = 'b';
	} else {
		bg = 'w';
	}
	uint8_t player;
	if (y % 2) {
		player = 2;
	} else {
		player = 1;
	}
	char bitmap[] = {bg , 'b', ((String) (y - 1)).charAt(0), '.', 'b', 'm', 'p'};
	TS_Point counterPos = getCounterPosition(x);
	drawBitmap(bitmap, counterPos.x, counterPos.y);
	boardState[x][0] = player;
	boardState[x][1] = y;
	
	for (uint8_t i = 0; i < 9; i++) {
		if (boardState[x][i + 2] != 0 && i + 2 != y) {
			// Runs when another counter is found in the same square
			for (uint8_t j = 0; j < 9; j++) {
				if (boardState[j][i + 2] != 0 && j != x) {
					// Runs when pair is found at [j][i + 2]
					if (j != origX && i + 2 != origY) {
						// Runs if not the original counter
						drawClassicalCounters(boardState, j, i + 2, origX, origY);
					}
				}
			}
		}
	}
}

uint8_t checkForQuantumWinner(uint8_t boardState[9][11]) {
	// winningRows[x][0] = winner
	// winningRows[x][1] = subscript total
	uint8_t winningRows[3][2] = {{0, 0}, {0, 0}, {0, 0}};
	uint8_t numOfWinningRows = 0;
	uint8_t winner = 0;
	// Check to see if someone has won
	for (uint8_t i = 0; i < 8; i++) {
		if (boardState[winIndexes[i][0]][0] == boardState[winIndexes[i][1]][0]
		&& boardState[winIndexes[i][0]][0] == boardState[winIndexes[i][2]][0]
		&& boardState[winIndexes[i][0]][0] != 0) {
			// Someone has won
				
			// Add the player that won to winningRows
			winningRows[numOfWinningRows][0] = boardState[winIndexes[i][0]][0];
				
			// Total the subscripts and store in winningRows
			uint8_t highestSubscript = 0;
			for (uint8_t j = 0; j < 3; j++) {
					if (highestSubscript < boardState[winIndexes[i][j]][1]) {
						highestSubscript = boardState[winIndexes[i][j]][1];
					}
			}
			winningRows[numOfWinningRows][1] = highestSubscript;
			numOfWinningRows++;
		}
	}
		
	// Find the winner if one exists
	uint8_t lowestMaxSubscript = 255;
	for (uint8_t i = 0; i < 3; i++) {
		if (winningRows[i][1] < lowestMaxSubscript && winningRows[i][1] != 0) {
			winner = winningRows[i][0];
			lowestMaxSubscript = winningRows[i][1];
		}
	}
	return winner;
}

State game(uint8_t noughtsScore, uint8_t crossesScore) {
	State player = cross;
	State boardState[9] = {empty, empty, empty, empty, empty, empty, empty, empty,
	empty};
	State winner = empty;
	uint8_t placedCounters = 0;
	uint8_t square = 255;
	TS_Point newCounterPos;

	// Draw grid
	drawBitmap('d', 0, 80);

	// Start game
	while (placedCounters < 9) {
		while (true) {
			if (!touchScreen.touched()) continue;
			TS_Point pointTouched = getPoint();
			if (verbose)
			Serial.println(
			(String) F("screen pressed at: (") + (String) pointTouched.x + ","
			+ (String) pointTouched.y + (String) F(")"));
			square = getSquare(pointTouched);
			newCounterPos = getCounterPosition(square);
			if (!(square == 255)) break;
		}

		// Check to see if selected square is occupied
		if (boardState[square] != empty) continue;

		// Update boardState
		boardState[square] = player;

		// Draw nought or cross on selected square and swap player
		player = addMove(square, newCounterPos, player);
		placedCounters++;

		// Check to see if someone has won
		for (uint8_t i = 0; i < 8; i++) {
			if (boardState[winIndexes[i][0]] == boardState[winIndexes[i][1]]
			&& boardState[winIndexes[i][0]] == boardState[winIndexes[i][2]]
			&& boardState[winIndexes[i][0]] != empty) {
				winner = boardState[winIndexes[i][0]];
			}
		}
		Serial.print(F("winner = "));
		Serial.println(winner);
		if (!(winner == empty)) break;
		delay(100);
	}
	return winner;
}

uint8_t quantumGame(uint8_t noughtsScore, uint8_t crossesScore) {
	// 0 = none, 1 = cross, 2 = nought
	uint8_t player = 1;
	uint8_t turn = 1;
	uint8_t winner = 0;
	uint8_t boardState[9][11] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
	uint8_t bigCounters = 0;
	TS_Point newCounterPos;

	// Draw grid
	drawBitmap('d', 0, 80);

	// Game logic
	while (bigCounters < 9) {
		uint8_t countersThisTurn = 0;
		uint8_t square = 255;
		uint8_t recentSquares[] = {255, 255};
		
		// On final turn check if only one square is left
		if (turn == 9) {
			uint8_t classicalCounters = 0;
			for (uint8_t i = 0; i < 9; i++) {
				if (boardState[i][0] != 0) {
					classicalCounters++;
				}
			}
			if (classicalCounters == 8) {
				while (true) {
					if (!touchScreen.touched()) continue;
					TS_Point point = getPoint();
					square = getSquare(point);
					if (square == 255) continue;
					if (boardState[square][0] == 0) break;
				}
				boardState[square][turn + 1] = player;
				drawSmallCounter(square, turn);
				drawClassicalCounters(boardState, square, turn + 1, square, turn + 1);
				return checkForQuantumWinner(boardState);
			}
		}
		
		while (countersThisTurn < 2) {
			while (true) {
				if (!touchScreen.touched()) continue;
				TS_Point pointTouched = getPoint();
				if (verbose)
				Serial.println(
				(String) F("screen pressed at: (") + (String) pointTouched.x + ","
				+ (String) pointTouched.y + (String) F(")"));
				square = getSquare(pointTouched);
				if (!(square == 255)) break;
			}

			// Check to see if selected square is occupied
			if (boardState[square][0] != 0) continue;
			if (boardState[square][turn + 1] != 0) continue;

			// Update board state
			boardState[square][turn + 1] = player;
			if (recentSquares[0] == 255) {
				recentSquares[0] = square;
			} else {
				recentSquares[1] = square;
			}
		

			// Draw small nought or cross on selected square
			drawSmallCounter(square, turn);
			
			countersThisTurn++;
		}
		
		// Check for measurement
		uint8_t circle[9] = {255, 255, 255, 255, 255, 255, 255, 255, 255};
		findCircle(boardState, square, turn + 1, square, circle);
		if (circle[0] != 255) {
			// Circle found
			
			// Ask user how to resolve
			// Underline/Draw box around recent counters
			for (uint8_t i = 0; i < 9; i++) {
				if (boardState[i][turn + 1] != 0) {
					TS_Point point = getSmallCounterPosition(i, turn);
					//tftDisplay.drawRect(point.x, point.y, 25, 25, 0x07FF);
					tftDisplay.drawLine(point.x + 2, point.y + 26, point.x + 24, point.y + 26, 0xF800);
					tftDisplay.drawLine(point.x + 2, point.y + 25, point.x + 24, point.y + 25, 0xF800);
				}
			}
			
			// Get the two boxes parameters
			TS_Point square0 = getCounterPosition(recentSquares[0]);
			TS_Point square1 = getCounterPosition(recentSquares[1]);
			
			
			// Wait for the user to click one of the two boxes
			uint8_t tappedSquare = 255;
			while (true) {
				if (!touchScreen.touched()) continue;
				TS_Point point = getPoint();
				if (point.x > square0.x && point.x < square0.x + 80 && point.y > square0.y && point.y < square0.y + 80) {
					tappedSquare = recentSquares[0];
					break;
				} else if (point.x > square1.x && point.x < square1.x + 80 && point.y > square1.y && point.y < square1.y + 80) {
					tappedSquare = recentSquares[1];
					break;
				}
			}


			// Turn to classical counters
			if (tappedSquare != 255) {
				drawClassicalCounters(boardState, tappedSquare, turn + 1, tappedSquare, turn + 1);
			} else {
				Serial.println(F("Error"));
			}
		}

		winner = checkForQuantumWinner(boardState);
		
		Serial.print(F("winner = "));
		Serial.println(winner);
		if (winner != 0) break;
		
		turn++;
		(player == 1) ? player = 2 : player = 1;
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

		// Select win banner and add to winners score
		char bitmap;
		switch (winner) {
			case cross:
			// Crosses wins
			bitmap = 'e';
			crossesScore++;
			gamesPlayed++;
			break;
			case nought:
			// Noughts wins
			bitmap = 'g';
			noughtsScore++;
			gamesPlayed++;
			break;
			case empty:
			// Game is a draw
			bitmap = 'f';
			break;
		}
		
		updateScore(noughtsScore, crossesScore);
		
		// If there are still games to be played draw win banner,
		// otherwise draw end game screen
		uint8_t gamesToWin = (maxGames / 2) + 1;
		if (noughtsScore < gamesToWin && crossesScore < gamesToWin) {
			drawBitmap(bitmap, 0, 110);
			while (true) if (touchScreen.touched()) break;
			} else {
			if (crossesScore > noughtsScore) {
				// Draw game-over, crosses wins
				drawBitmap('m', 0, 0);
				// Then wait button press to return to main menu
				while (true) {
					if (touchScreen.touched()) {
						TS_Point point = getPoint();
						if (point.x > 22 && point.x < 216 && point.y > 185 && point.y < 265) {
							// They pressed the start button
							goto exit;
						}
						} else {
						continue;
					}
				}
				} else {
				// Draw game-over, noughts wins
				drawBitmap('n', 0, 0);
				// Then wait button press to return to main menu
				while (true) {
					if (touchScreen.touched()) {
						TS_Point point = getPoint();
						if (point.x > 22 && point.x < 216 && point.y > 185 && point.y < 265) {
							// They pressed the start button
							goto exit;
							} else {
							continue;
						}
					}
				}
			}
		}
	}
	exit:
	Serial.println("Broke while loop");
}

void playQuantumMatch(int maxGames) {
	uint8_t noughtsScore = 0;
	uint8_t crossesScore = 0;
	uint8_t gamesPlayed = 0;

	// Draw score bar for match
	drawBitmap('a', 0, 0);
	drawBitmap('c', 118, 32);
	updateScore(noughtsScore, crossesScore);
	drawBitmap('b', 81, 10);

	while (gamesPlayed < maxGames) {
		uint8_t winner = quantumGame(noughtsScore, crossesScore);

		// Select win banner and add to winners score
		char bitmap;
		switch (winner) {
			case 1:
			// Crosses wins
			bitmap = 'e';
			crossesScore++;
			gamesPlayed++;
			break;
			case 2:
			// Noughts wins
			bitmap = 'g';
			noughtsScore++;
			gamesPlayed++;
			break;
			case 0:
			// Game is a draw
			bitmap = 'f';
			break;
		}
		
		updateScore(noughtsScore, crossesScore);
		
		// If there are still games to be played draw win banner,
		// otherwise draw end game screen
		uint8_t gamesToWin = (maxGames / 2) + 1;
		if (noughtsScore < gamesToWin && crossesScore < gamesToWin) {
			drawBitmap(bitmap, 0, 110);
			while (true) if (touchScreen.touched()) break;
			} else {
			if (crossesScore > noughtsScore) {
				// Draw game-over, crosses wins
				drawBitmap('m', 0, 0);
				// Then wait button press to return to main menu
				while (true) {
					if (touchScreen.touched()) {
						TS_Point point = getPoint();
						if (point.x > 22 && point.x < 216 && point.y > 185 && point.y < 265) {
							// They pressed the start button
							goto exit;
						}
						} else {
						continue;
					}
				}
				} else {
				// Draw game-over, noughts wins
				drawBitmap('n', 0, 0);
				// Then wait button press to return to main menu
				while (true) {
					if (touchScreen.touched()) {
						TS_Point point = getPoint();
						if (point.x > 22 && point.x < 216 && point.y > 185 && point.y < 265) {
							// They pressed the start button
							goto exit;
							} else {
							continue;
						}
					}
				}
			}
		}
	}
	exit:
	Serial.println("Broke while loop");
}

void setup() {

	Serial.begin(9600);

	tftDisplay.begin();

	if (!touchScreen.begin()) {
		Serial.println(F("Couldn't start FT6206 touchscreen controller"));
		Serial.println(F("Driver might not have been found"));
		while (true);
	}

	if (verbose) Serial.println(F("Display and touchscreen started"));

	if (!SD.begin(SD_CS)) {
		Serial.println(F("Failed to initialise SD card"));
	} else if (verbose) Serial.println(F("SD card mounted"));
}

void loop() {
	uint8_t maxGames = 1;
	drawBitmap('l', 0, 0);
	String s = (String) maxGames;
	char maxGamesChar = s.charAt(0);
	drawBitmap(maxGamesChar, 173, 115);
	boolean quantumMatch;

	while (true) {
		if (touchScreen.touched()) {
			TS_Point point = getPoint();
			if (point.x > 20 && point.x < 220 && point.y > 240 && point.y < 315) {
				// They pressed the quantum button
				quantumMatch = true;
				break;
			} else if (point.x > 20 && point.x < 220 && point.y > 170 && point.y < 241) {
				// They pressed the classic button
				quantumMatch = false;
				break;
			} else if (point.x > 105 && point.x < 181 && point.y > 90 && point.y < 171) {
				// They pressed the left "best of:" button
				if (maxGames != 1) {
					maxGames -= 2;
					// Update score
					String s = (String) maxGames;
					char maxGamesChar = s.charAt(0);
					drawBitmap(maxGamesChar, 173, 115);
				}
				continue;
			} else if (point.x > 180 && point.x < 241 && point.y > 90 && point.y < 171) {
				// They pressed the left "best of:" button
				if (maxGames != 9) {
					maxGames += 2;
					// Update score
					String s = (String) maxGames;
					char maxGamesChar = s.charAt(0);
					drawBitmap(maxGamesChar, 173, 115);
				}
				continue;
			} else {
				continue;
			}
		} else {
			continue;
		}
	}
	
	if (quantumMatch) {
		playQuantumMatch(maxGames);
		} else {
		playMatch(maxGames);
	}
}