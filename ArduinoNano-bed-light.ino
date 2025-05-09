#include <CapacitiveSensor.h>

// Debug mode (set to 1 to enable serial debugging, 0 to disable)
#define DEBUG_MODE 0

#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_BEGIN() Serial.begin(115200)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_BEGIN()
#endif

// Pin definitions for Arduino Nano
#define SEND_PIN 6        // Capacitive sensor send pin
#define RECEIVE_PIN 8     // Capacitive sensor receive pin
#define MOSFET_PIN 3      // PWM output pin for MOSFET

// Constants
#define TOUCH_THRESHOLD 300     // Adjust this value based on your setup
#define RELEASE_THRESHOLD 150   // Threshold to detect release
#define FADE_SPEED 5           // Lower number = slower fade
#define DEBOUNCE_DELAY 500     // Delay to prevent multiple triggers
#define MAX_BRIGHTNESS 255     // Maximum LED brightness
#define AUTO_OFF_TIME 600000   // Auto off after 10 minutes (600000ms)

// Create capacitive sensor object
CapacitiveSensor touchSensor = CapacitiveSensor(SEND_PIN, RECEIVE_PIN);

// Variables
bool isOn = false;
bool wasTouched = false;      // Track if sensor was touched
int currentBrightness = 0;
int targetBrightness = 0;
unsigned long lastTouchTime = 0;
unsigned long turnOnTime = 0;    // When the light was turned on
bool firstTime = true;       // Flag for first touch after startup

// Helper function to handle millis() overflow
unsigned long getElapsedTime(unsigned long startTime) {
  unsigned long currentTime = millis();
  // Handle overflow
  if (currentTime < startTime) {
    return (0xFFFFFFFF - startTime) + currentTime;
  }
  return currentTime - startTime;
}

void setup() {
  pinMode(MOSFET_PIN, OUTPUT);
  
  DEBUG_BEGIN();
  DEBUG_PRINTLN(F("Starting up..."));
  
  // Initialize the capacitive sensor
  touchSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);
  
  DEBUG_PRINTLN(F("Setup complete"));
}

void loop() {
  long value = touchSensor.capacitiveSensor(30);
  
  // Check auto-off timer using overflow-safe comparison
  if (isOn && (getElapsedTime(turnOnTime) >= AUTO_OFF_TIME)) {
    isOn = false;
    wasTouched = false;
    targetBrightness = 0;
    DEBUG_PRINTLN(F("Auto-off triggered"));
  }
  
  // Debug output
  DEBUG_PRINT(F("Raw Sensor: "));
  DEBUG_PRINT(value);
  DEBUG_PRINT(F(" State: "));
  DEBUG_PRINT(isOn ? "ON" : "OFF");
  DEBUG_PRINT(F(" Touched: "));
  DEBUG_PRINT(wasTouched ? "YES" : "NO");
  DEBUG_PRINT(F(" Brightness: "));
  DEBUG_PRINTLN(currentBrightness);
  
  // Check for touch and release
  if (value > TOUCH_THRESHOLD) {
    if (!wasTouched) {  // Only toggle state on initial touch
      if (firstTime) {
        lastTouchTime = millis() - DEBOUNCE_DELAY - 1;  // Make first touch work immediately
        firstTime = false;
      }
      
      if (getElapsedTime(lastTouchTime) > DEBOUNCE_DELAY) {
        wasTouched = true;
        // Toggle state immediately on touch
        isOn = !isOn;
        targetBrightness = isOn ? MAX_BRIGHTNESS : 0;
        if (isOn) {
          turnOnTime = millis();
        }
        DEBUG_PRINTLN(F("Touch detected - toggling state"));
      }
    }
    lastTouchTime = millis();  // Update time continuously while touched
  }
  else if (value < RELEASE_THRESHOLD && wasTouched) {
    wasTouched = false;
    DEBUG_PRINTLN(F("Release detected"));
  }

  // Fade handling
  if (currentBrightness < targetBrightness) {
    currentBrightness = min(currentBrightness + FADE_SPEED, targetBrightness);
  } else if (currentBrightness > targetBrightness) {
    currentBrightness = max(currentBrightness - FADE_SPEED, targetBrightness);
  }

  // Output to MOSFET
  analogWrite(MOSFET_PIN, currentBrightness);

  delay(4);
}
