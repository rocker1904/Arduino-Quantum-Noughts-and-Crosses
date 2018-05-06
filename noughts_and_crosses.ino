/***************************************************
  Touchscreen Noughts and Crosses Project

  Using Adafruit TFT Capacitive Touch Shield and Arduino Uno

  WIP:
  - Git
  - Game logic
    - win checking
    - 3D logic
  - Winning banner
  - Reset button
  - Settings
    - Game wins for match win
  - AI
  - Remove unused function
  - Consider better implmentations
  - 3D UI

  Written by Sam Ellis
 ****************************************************/


#include <SPI.h>    // Communication with display
#include <Adafruit_GFX.h>   // Graphics library
#include <Adafruit_ILI9341.h>   // tftDisplay display library
#include <SD.h> // Library for accessing SD cards, needed for loading bitmaps
#include <Wire.h>   // Needed by FT6206
#include <Adafruit_FT6206.h>    // Capacitive touchscreen library


// Debugging
bool verbose = false;


// Use hardware SPI (on 13, 12, 11) and below for CS/DC
#define tftDisplay_DC 9
#define tftDisplay_CS 10
Adafruit_ILI9341 tftDisplay = Adafruit_ILI9341(tftDisplay_CS, tftDisplay_DC);

// Touchscreen (uses I2C, on SCL/SDA)
Adafruit_FT6206 touchScreen = Adafruit_FT6206();

// SD card CS pin
#define SD_CS 4
// SD card Pixel buffer
#define BUFFPIXEL 40

// Keep track of the score
int noughtsScore = 0;
int crossesScore = 0;

// New type for keeping track of board state
enum State {empty, nought, cross};

void setup() {

  Serial.begin(9600);

  tftDisplay.begin();

  if (!touchScreen.begin()){
    Serial.println("Couldn't start FT6206 touchscreen controller");
    Serial.println("Driver might not have been found");
    while (1);
  }

  if (verbose == true) Serial.println("Display and touchscreen started");

  if (!SD.begin(SD_CS)) {
    Serial.println("Failed to initialise SD card");
  } else if (verbose == true) Serial.println("SD card mounted");

  // tests();


  startScreen();

}

void loop() {

  if (!touchScreen.touched()) return;

  TS_Point point = getPoint();

  Serial.println("screen pressed at: (" + (String)point.x + "," + (String)point.y + ")");
  delay(100);

}

void clearScreen() {
  char bitmap[] = "blank.bmp";
  bmpDraw(bitmap, 0, 0);
}

void startScreen() {

  char bitmap[] = "start.bmp";
  bmpDraw(bitmap, 0, 0);

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

  playGame();

}

void playGame() {
  noughtsScore = 0;
  crossesScore = 0;
  State player = cross;
  State boardState[9] = {empty, empty, empty, empty, empty, empty, empty, empty, empty};
  int placedCounters = 0;
  drawGameScreen();
  int square = -1;
  TS_Point marker;

  while (placedCounters < 9) {
    while(true) {
      if (!touchScreen.touched()) continue;
      TS_Point point = getPoint();
      if (verbose == true) Serial.println("screen pressed at: (" + (String)point.x + "," + (String)point.y + ")");
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
      if (!(square == -1)) break;
    }

    // Check to see if selected square is occupied
    if (!(boardState[square] == empty)) continue;

    // Update board state
    boardState[square] = player;

    // Draw nought or cross on selected square
    if (player == cross) {
      if (square % 2) {
        char bitmap[] = "X-b.bmp";
        bmpDraw(bitmap, marker.x, marker.y);
      } else {
        char bitmap[] = "X-w.bmp";
        bmpDraw(bitmap, marker.x, marker.y);
      }
      player = nought;
    } else {
      if (square % 2) {
        char bitmap[] = "O-b.bmp";
        bmpDraw(bitmap, marker.x -2, marker.y);
      } else {
        char bitmap[] = "O-w.bmp";
        bmpDraw(bitmap, marker.x -2, marker.y);
      }
      player = cross;
    }
    placedCounters += 1;
    delay(100);
  }
}

void drawGameScreen() {
  if (verbose == true) Serial.println("Drawing game");
  char bitmap[] = "main2.bmp";
  bmpDraw(bitmap, 0, 0);
  updateScore();
}

void updateScore() {
  char bitmap[] = "0.bmp";
  String s = (String)noughtsScore;
  char newScore = s.charAt(0);
  bitmap[0] = newScore;
  bmpDraw(bitmap, 95, 29);
  s = (String)crossesScore;
  newScore = s.charAt(0);
  bitmap[0] = newScore;
  bmpDraw(bitmap, 132, 29);
}

TS_Point getPoint() {
  TS_Point point = touchScreen.getPoint();
  // convert point to match display coordinate system
  point.x = map(point.x, 0, 240, 240, 0);
  point.y = map(point.y, 0, 320, 320, 0);
  return point;
}

// Unused
bool isIntInArray(int arr[], int n, int numToFind) {
  for(int i = 0; i < n; i++) {
    if (arr[i] == numToFind) return true;
  }
  return false;
}

void bmpDraw(char *filename, int16_t x, int16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col, x2, y2, bx1, by1;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tftDisplay.width()) || (y >= tftDisplay.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Todo (BROKEN)

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename))) {
    Serial.println("File (maybe) not found");
    // return;
  }


  // Parse BMP header
  Serial.println(F("Start parse"));
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        x2 = x + bmpWidth  - 1; // Lower-right corner
        y2 = y + bmpHeight - 1;
        if((x2 >= 0) && (y2 >= 0)) { // On screen?
          w = bmpWidth; // Width/height of section to load/display
          h = bmpHeight;
          bx1 = by1 = 0; // UL coordinate in BMP file
          if(x < 0) { // Clip left
            bx1 = -x;
            x   = 0;
            w   = x2 + 1;
          }
          if(y < 0) { // Clip top
            by1 = -y;
            y   = 0;
            h   = y2 + 1;
          }
          if(x2 >= tftDisplay.width())  w = tftDisplay.width()  - x; // Clip right
          if(y2 >= tftDisplay.height()) h = tftDisplay.height() - y; // Clip bottom

          // Set tftDisplay address window to clipped image bounds
          tftDisplay.startWrite(); // Requires start/end transaction now
          tftDisplay.setAddrWindow(x, y, w, h);

          for (row=0; row<h; row++) { // For each scanline...

            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
              pos = bmpImageoffset + (bmpHeight - 1 - (row + by1)) * rowSize;
            else     // Bitmap is stored top-to-bottom
              pos = bmpImageoffset + (row + by1) * rowSize;
            pos += bx1 * 3; // Factor in starting column (bx1)
            if(bmpFile.position() != pos) { // Need seek?
              tftDisplay.endWrite(); // End tftDisplay transaction
              bmpFile.seek(pos);
              buffidx = sizeof(sdbuffer); // Force buffer reload
              tftDisplay.startWrite(); // Start new tftDisplay transaction
            }
            for (col=0; col<w; col++) { // For each pixel...
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
              tftDisplay.writePixel(tftDisplay.color565(r,g,b));
            } // end pixel
          } // end scanline
          tftDisplay.endWrite(); // End last tftDisplay transaction
        } // end onscreen
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized or SD not mounted"));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

//void tests(){
//
//  clearScreen();
//
//  // Filled rectangle from (0,0)
//  tftDisplay.fillRect(0, 0, 150, 150, ILI9341_CYAN);
//  Serial.println("filled rectangle");
//  delay(3000);
//
//  clearScreen();
//
//  tftDisplay.println("Text with default formatting");
//  Serial.println("text");
//  delay(3000);
//
//  clearScreen();
//
//  // Print hight and width of the display to the display
//  tftDisplay.setCursor(0, 0);
//  tftDisplay.setTextColor(ILI9341_WHITE);
//  tftDisplay.setTextSize(2);
//  Serial.println("settings");
//  tftDisplay.println(tftDisplay.width());
//  tftDisplay.println(tftDisplay.height());
//  Serial.println("height and width");
//  Serial.println(tftDisplay.width());
//  Serial.println(tftDisplay.height());
//  delay(3000);
//}
