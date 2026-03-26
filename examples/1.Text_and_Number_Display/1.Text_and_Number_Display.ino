/**
 ****************************************************************************** 
 * @file     Text_and_Number_Display.ino
 * @brief    Set up a TFT display to show a cycling two-digit number.
 *           The program initializes the TFT screen, displays a color transition,
 *           and then continuously displays a number that increments from 00 to 99.
 * @version  V1.0
 * @date     2024-12-04
 * @license  MIT
 ****************************************************************************** 
 * Experiment Objective: Display a continuously incrementing two-digit number 
 *                       (00-99) on a TFT screen with a background color transition.
 *                       Demonstrates basic TFT display functionality and number formatting.
 *
 * Hardware Resources:
 * 1. TFT Display --> Connected via SPI (pins defined in User_Setup.h).
 *
 ****************************************************************************** 
 * Development Platform: ESP32-C3/S3 Development Board
 * Support Forum: service.waveshare.com
 * Company Website: www.waveshare.com
 *
 ****************************************************************************** 
 */

#include <SPI.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

void setup() {
  // Initialize TFT display
  tft.init();

  // Display color transitions
  tft.fillScreen(TFT_BLUE);
  delay(1000);
  tft.fillScreen(TFT_RED);
  delay(1000);
  tft.fillScreen(TFT_GREEN);
  delay(1000);

  // Set background color and font color
  tft.fillScreen(0x04FF);  // RGB565 color
  tft.setTextColor(TFT_WHITE, 0x04FF);  // Set text color to white with background

  // Display welcome message
  tft.drawString("Hello, Waveshare!", 30, 40, 2);
}

int number = 0;

void loop() {
  // Format the number as a two-digit string
  String displayNumber = String(number);
  if (displayNumber.length() == 1) {
    displayNumber = "0" + displayNumber;  // Add leading zero if needed
  }

  // Display the number on the screen
  tft.drawString(displayNumber, 55, 80, 6);

  // Increment the number and check if it exceeds 99
  number++;
  if (number > 99) {
    number = 0;  // Reset to 0 if the number exceeds 99
  }

  // Delay for 1 second
  delay(1000);
}
