#include <SPI.h>
#include <TFT_eSPI.h>
#include "tools/MyAnim.h"
#if defined(ESP32)
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <OSCMessage.h>
#endif
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

const uint16_t* const kDemo2Frames[] = {
  gImage_A1,  gImage_A2,  gImage_A3,  gImage_A4,  gImage_A5,
  gImage_A6,  gImage_A7,  gImage_A8,  gImage_A9,  gImage_A10,
  gImage_A11, gImage_A12, gImage_A13, gImage_A14, gImage_A15,
  gImage_A16, gImage_A17, gImage_A18, gImage_A19, gImage_A20,
  gImage_A21, gImage_A22, gImage_A23, gImage_A24, gImage_A25
};
const uint8_t kDemo2FrameCount = sizeof(kDemo2Frames) / sizeof(kDemo2Frames[0]);

volatile bool g_trackEnabled = true;
volatile bool g_trackHasData = false;
volatile float g_trackNormX = 0.5f;
volatile float g_trackNormY = 0.5f;
volatile uint32_t g_lastTrackInputMs = 0;
volatile uint8_t g_trackBlendPct = 78;
const uint32_t kTrackHoldTimeoutMs = 1200;

volatile uint8_t g_requestedDemo2Loops = 0;

#if defined(ESP32)
const char* STA_SSID = "F7OWER";
const char* STA_PASSWORD = "12345678";
const char* NODE_ID = "eye_anime_1";
const char* NODE_TYPE = "eye_anime";
const int OSC_PORT = 8888;
const unsigned long WIFI_RETRY_INTERVAL_MS = 6000;

WiFiUDP udp;
bool mdnsStarted = false;
unsigned long lastWifiRetryMs = 0;
#endif

bool isOscTrackingActive() {
  if (!g_trackEnabled || !g_trackHasData) return false;
  return (millis() - g_lastTrackInputMs) <= kTrackHoldTimeoutMs;
}

void getOscTrackingNorm(float &nx, float &ny, uint8_t &blendPct) {
  nx = g_trackNormX;
  ny = g_trackNormY;
  blendPct = g_trackBlendPct;
}

#if defined(ESP32)
float clampf(float value, float lo, float hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

bool connectWifiWithAttempts(int attempts, bool verbose) {
  attempts = constrain(attempts, 1, 120);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(STA_SSID, STA_PASSWORD);
  if (verbose) Serial.print("[Net] Connecting");
  for (int i = 0; i < attempts; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      if (verbose) {
        Serial.print("\n[Net] Connected, IP: ");
        Serial.println(WiFi.localIP());
      }
      return true;
    }
    delay(500);
    if (verbose) Serial.print(".");
  }
  if (verbose) Serial.println("\n[Net] WiFi connect failed");
  return WiFi.status() == WL_CONNECTED;
}

void setupMDNS() {
  if (mdnsStarted || WiFi.status() != WL_CONNECTED) return;
  if (!MDNS.begin(NODE_ID)) {
    Serial.println("[Net] mDNS failed");
    return;
  }
  MDNS.addService("osc", "udp", OSC_PORT);
  MDNS.addServiceTxt("osc", "udp", "node_type", NODE_TYPE);
  MDNS.addServiceTxt("osc", "udp", "node_id", NODE_ID);
  MDNS.addService("datt_flower", "tcp", OSC_PORT);
  MDNS.addServiceTxt("datt_flower", "tcp", "node_type", NODE_TYPE);
  MDNS.addServiceTxt("datt_flower", "tcp", "node_id", NODE_ID);
  mdnsStarted = true;
  Serial.printf("[Net] mDNS ready: %s.local\n", NODE_ID);
}

void ensureWifiConnected() {
  if (WiFi.status() == WL_CONNECTED) return;
  unsigned long now = millis();
  if (now - lastWifiRetryMs < WIFI_RETRY_INTERVAL_MS) return;
  lastWifiRetryMs = now;
  connectWifiWithAttempts(8, false);
}

void ensureMDNS() {
  if (!mdnsStarted && WiFi.status() == WL_CONNECTED) setupMDNS();
}

void routeTrackAuto(OSCMessage &msg, int addrOffset) {
  if (msg.size() < 1) return;
  g_trackEnabled = msg.getInt(0) != 0;
}

void routeTrackMode(OSCMessage &msg, int addrOffset) {
  routeTrackAuto(msg, addrOffset);
}

void routeTrackNorm(OSCMessage &msg, int addrOffset) {
  if (msg.size() < 2) return;
  float nx = clampf(msg.getFloat(0), 0.0f, 1.0f);
  float ny = clampf(msg.getFloat(1), 0.0f, 1.0f);
  g_trackNormX = nx;
  g_trackNormY = ny;
  g_trackHasData = true;
  g_lastTrackInputMs = millis();
}

void routeTrackXY(OSCMessage &msg, int addrOffset) {
  if (msg.size() < 4) return;
  int x = msg.getInt(0);
  int y = msg.getInt(1);
  int frameW = max(msg.getInt(2), 1);
  int frameH = max(msg.getInt(3), 1);
  float nx = clampf((float)x / (float)frameW, 0.0f, 1.0f);
  float ny = clampf((float)y / (float)frameH, 0.0f, 1.0f);
  g_trackNormX = nx;
  g_trackNormY = ny;
  g_trackHasData = true;
  g_lastTrackInputMs = millis();
}

void routeTrackCenter(OSCMessage &msg, int addrOffset) {
  g_trackNormX = 0.5f;
  g_trackNormY = 0.5f;
  g_trackHasData = true;
  g_lastTrackInputMs = millis();
}

void routeEyeAnime(OSCMessage &msg, int addrOffset) {
  int loops = 1;
  if (msg.size() >= 2) loops = constrain(msg.getInt(1), 1, 20);
  else if (msg.size() >= 1) loops = constrain(msg.getInt(0), 1, 20);
  g_requestedDemo2Loops = (uint8_t)loops;
}

void routeInfoSelf(OSCMessage &msg, int addrOffset) {
  IPAddress ip = WiFi.localIP();
  OSCMessage reply("/info/self");
  reply.add((char*)NODE_ID);
  reply.add((char*)"esp32");
  reply.add((char*)NODE_TYPE);
  char ipbuf[20];
  snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  reply.add((char*)ipbuf);
  udp.beginPacket(udp.remoteIP(), udp.remotePort());
  reply.send(udp);
  udp.endPacket();
  reply.empty();
}

void processOSC() {
  int packetSize = udp.parsePacket();
  if (!packetSize) return;
  OSCMessage msg;
  while (packetSize--) msg.fill(udp.read());
  if (msg.hasError()) return;
  msg.route("/track/auto", routeTrackAuto);
  msg.route("/track/mode", routeTrackMode);
  msg.route("/track/norm", routeTrackNorm);
  msg.route("/track/xy", routeTrackXY);
  msg.route("/track/center", routeTrackCenter);
  msg.route("/eye/anime", routeEyeAnime);
  msg.route("/anime/play", routeEyeAnime);
  msg.route("/info/self", routeInfoSelf);
}
#endif

void serviceBackground() {
#if defined(ESP32)
  ensureWifiConnected();
  ensureMDNS();
  processOSC();
#endif
}

void delayWithService(uint32_t waitMs) {
  uint32_t start = millis();
  while ((millis() - start) < waitMs) {
    serviceBackground();
    delay(1);
  }
}

void Demo_1()
{
  updateEye();
}

void Demo_2()
{
  for (uint8_t idx = 0; idx < kDemo2FrameCount; idx++) {
    digitalWrite(device_A_CS, LOW);
    tft.pushImage(0, 0, 160, 160, kDemo2Frames[idx]);
    digitalWrite(device_A_CS, HIGH);
    delayWithService((uint32_t)frameTime);
  }
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

#if defined(ESP32)
  connectWifiWithAttempts(20, true);
  udp.begin(OSC_PORT);
  setupMDNS();
#endif

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
  for (int a = 1; a <= 2; a++) {
    if (a == 1) {
      tft.fillScreen(BLACK);
      uint32_t demo1Start = millis();
      while ((millis() - demo1Start) < 2000) {
        serviceBackground();
        Demo_1();
      }
    } else if (a == 2) {
      uint8_t loops = g_requestedDemo2Loops ? g_requestedDemo2Loops : 7;
      g_requestedDemo2Loops = 0;
      for (i = 0; i < loops; i++) {
        serviceBackground();
        Demo_2();
      }
    }
  }
  serviceBackground();
}
