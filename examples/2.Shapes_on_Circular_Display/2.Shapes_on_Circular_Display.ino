/**
 ****************************************************************************** 
 * @file     Shapes_on_Circular_Display.ino
 * @brief    Draw a square, triangle, and a circle centered on the display 
 *           with random fill colors for each shape.
 * @version  V1.0
 * @date     2024-12-04
 * @license  MIT
 ****************************************************************************** 
 * Experiment Objective: Display a square, triangle, and a circle centered 
 *                       on the display with random fill colors. Each shape 
 *                       is drawn sequentially every 3 seconds.
 *
 * Hardware Resources:
 * 1. Circular Display (160x160) --> GC9D01 driver or equivalent.
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

int centerX = 80;  // X coordinate of the center of the screen
int centerY = 80;  // Y coordinate of the center of the screen
int radius = 80;   // Radius for the circle (half of the display resolution)

void setup() {
  // Initialize TFT display
  tft.init();
  
  // Set the initial background to white
  tft.fillScreen(TFT_WHITE);
}

void drawSquare() {
  // Random color for the square
  uint16_t squareColor = tft.color565(random(0, 255), random(0, 255), random(0, 255));

  // Draw a square with random color
  int squareSize = radius * sqrt(2);  // Diagonal of the square equals the circle's diameter
  int squareTopLeftX = centerX - squareSize / 2;
  int squareTopLeftY = centerY - squareSize / 2;

  tft.fillRect(squareTopLeftX, squareTopLeftY, squareSize, squareSize, squareColor);
}

void drawTriangle() {
  // Random color for the triangle
  uint16_t triangleColor = tft.color565(random(0, 255), random(0, 255), random(0, 255));

  // Draw an equilateral triangle with random color
  int triangleHeight = radius * sqrt(3) / 2;  // Height of the equilateral triangle
  int triangleSide = 2 * radius * sin(PI / 3); // Side length of the triangle
  int triangleX[3], triangleY[3];

  // Calculate triangle vertices
  triangleX[0] = centerX;
  triangleY[0] = centerY - radius;
  triangleX[1] = centerX - triangleSide / 2;
  triangleY[1] = centerY + triangleHeight / 2;
  triangleX[2] = centerX + triangleSide / 2;
  triangleY[2] = centerY + triangleHeight / 2;

  tft.fillTriangle(triangleX[0], triangleY[0], triangleX[1], triangleY[1], triangleX[2], triangleY[2], triangleColor);
}

void drawCircleInCenter() {
  // Random color for the circle
  uint16_t circleColor = tft.color565(random(0, 255), random(0, 255), random(0, 255));

  // Draw a circle centered in the middle of the screen
  tft.fillCircle(centerX, centerY, 30, circleColor);
}

void loop() {
  // Step 1: Draw a random colored square
  drawSquare();
  delay(3000);  // Wait for 3 seconds

  // Step 2: Draw a random colored triangle
  drawTriangle();
  delay(3000);  // Wait for 3 seconds

  // Step 3: Draw a circle with the radius equal to half of the display resolution
  drawCircleInCenter();
  delay(3000);  // Wait for 3 seconds
}
