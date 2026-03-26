#include <SPI.h>
#include <TFT_eSPI.h>
#include "EYEA.h"
TFT_eSPI tft;           // A single instance is used for 1 or 2 displays

#define device_A_CS  5

// some extra colors
   #define BLACK        0x0000
   #define BLUE         0x001F
   #define RED          0xF800
   #define GREEN        0x07E0
   #define CYAN         0x07FF
   #define MAGENTA      0xF81F
   #define YELLOW       0xFFE0
   #define WHITE        0xFFFF
   #define ORANGE       0xFBE0
   #define GREY         0x84B5
   #define BORDEAUX     0xA000
   #define DINOGREEN    0x2C86
   #define WHITE        0xFFFF

   int frameTime = 70;
   int j;
// A pixel buffer is used during eye rendering
#define BUFFER_SIZE 1024 // 128 to 1024 seems optimum

#ifdef USE_DMA
  #define BUFFERS 2      // 2 toggle buffers with DMA
#else
  #define BUFFERS 1      // 1 buffer for no DMA
#endif

uint16_t pbuffer[BUFFERS][BUFFER_SIZE]; // Pixel rendering buffer
bool     dmaBuf   = 0;                  // DMA buffer selection

// This struct is populated in config.h
typedef struct {        // Struct is defined before including config.h --
  int8_t  select;       // pin numbers for each eye's screen select line
  int8_t  wink;         // and wink button (or -1 if none) specified there,
  uint8_t rotation;     // also display rotation and the x offset
  int16_t xposition;    // position of eye on the screen
} eyeInfo_t;

#include "config.h"     // ****** CONFIGURATION IS DONE IN HERE ******

extern void user_setup(void); // Functions in the user*.cpp files
extern void user_loop(void);

#define SCREEN_X_START 0
#define SCREEN_X_END   SCREEN_WIDTH   // Badly named, actually the "eye" width!
#define SCREEN_Y_START 0
#define SCREEN_Y_END   SCREEN_HEIGHT  // Actually "eye" height

// A simple state machine is used to control eye blinks/winks:
#define NOBLINK 0       // Not currently engaged in a blink
#define ENBLINK 1       // Eyelid is currently closing
#define DEBLINK 2       // Eyelid is currently opening
typedef struct {
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duration of blink state (micros)
  uint32_t startTime;   // Time (micros) of last state change
} eyeBlink;

struct {                // One-per-eye structure
  int16_t   tft_cs;     // Chip select pin for each display
  eyeBlink  blink;      // Current blink/wink state
  int16_t   xposition;  // x position of eye image
} eye[NUM_EYES];

uint32_t startTime;  // For FPS indicator

void Demo_1()
{
  updateEye();
}

void Demo_2()
{
// ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160,160,gImage_A1);
   digitalWrite (device_A_CS, HIGH); 
  
   delay (frameTime);
// ============================================================================

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A2);
   digitalWrite (device_A_CS, HIGH); 

   delay (frameTime);


// ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A3);
   digitalWrite (device_A_CS, HIGH); 
   
   delay (frameTime);

// ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A4);
   digitalWrite (device_A_CS, HIGH); 

   delay (frameTime);

// ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A5);
   digitalWrite (device_A_CS, HIGH); 

   delay (frameTime);
   

// ============================================================================ 
// // ============================================================================ 
   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A6);
   digitalWrite (device_A_CS, HIGH); 
   
   delay (frameTime);


// // ============================================================================ 

  digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A7);
   digitalWrite (device_A_CS, HIGH); 
  
   
   delay (frameTime);


// // ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A8);
   digitalWrite (device_A_CS, HIGH);  
   delay (frameTime);


// // ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A9);
   digitalWrite (device_A_CS, HIGH); 
   
   
   delay (frameTime);


// // ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A10);
   digitalWrite (device_A_CS, HIGH); 
   
   delay (frameTime);
// // ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A11);
   digitalWrite (device_A_CS, HIGH);  
   delay (frameTime);
// // ============================================================================ 

   digitalWrite (device_A_CS, LOW);   
   tft.pushImage (0, 0,160, 160,gImage_A12);
   digitalWrite (device_A_CS, HIGH); 
   delay (frameTime);
// // ============================================================================ 
}

// INITIALIZATION -- runs once at startup ----------------------------------
void setup(void) {
  Serial.begin(115200);
  tft.writecommand(TFT_MADCTL);
  delay(10);
  tft.writedata(TFT_MAD_MX | TFT_MAD_MV );
  //while (!Serial);
  Serial.println("Starting");

#if defined(DISPLAY_BACKLIGHT) && (DISPLAY_BACKLIGHT >= 0)
  // Enable backlight pin, initially off
  Serial.println("Backlight turned off");
  pinMode(DISPLAY_BACKLIGHT, OUTPUT);
  digitalWrite(DISPLAY_BACKLIGHT, LOW);
#endif

  // User call for additional features
  user_setup();

  // Initialise the eye(s), this will set all chip selects low for the tft.init()
  initEyes();

  // Initialise TFT
  Serial.println("Initialising displays");
  tft.init();
  // tft.setRotation(1);
  
  // tft.invertDisplay(0);
#ifdef USE_DMA
  tft.initDMA();
#endif

  // Raise chip select(s) so that displays can be individually configured
  digitalWrite(eye[0].tft_cs, HIGH);
  if (NUM_EYES > 1) digitalWrite(eye[1].tft_cs, HIGH);

  for (uint8_t e = 0; e < NUM_EYES; e++) {
    digitalWrite(eye[e].tft_cs, LOW);
    tft.setRotation(eyeInfo[e].rotation);
    tft.fillScreen(TFT_BLACK);
    digitalWrite(eye[e].tft_cs, HIGH);
  }

#if defined(DISPLAY_BACKLIGHT) && (DISPLAY_BACKLIGHT >= 0)
  Serial.println("Backlight now on!");
  analogWrite(DISPLAY_BACKLIGHT, BACKLIGHT_MAX);
#endif

  startTime = millis(); // For frame-rate calculation
}

// MAIN LOOP -- runs continuously after setup() ----------------------------
char i=0;
void loop() {
  // Demo_1();
  for(int a=1;a<=2;a++){
    if(a==1){                                    
    tft.fillScreen (BLACK);
    Demo_1();
    delay(2000);
    }

    else if(a==2){
      for(i=0;i<7;i++){
        Demo_2();
      }
    }
  }
  
}
