#include "WiFiConfigManager.h"

WiFiConfigManager wifiManager;

// Define the pin for the trigger button (GPIO 0 is the BOOT button)
const int TRIGGER_PIN = 0; 
long buttonPressStartTime = 0;
bool buttonActive = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Main program running on Core 1.");
  Serial.println("Hold the BOOT button for 3 seconds to enter configuration mode.");

  // Set the button pin as an input with an internal pull-up resistor
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // Start the WiFi process in the background on Core 0
  wifiManager.beginAndRunInBackground(0);
}

void loop() {
  // --- Configuration Portal Trigger Logic ---
  // Check if the button is pressed (INPUT_PULLUP means LOW when pressed)
  if (digitalRead(TRIGGER_PIN) == LOW) {
    if (!buttonActive) {
      buttonActive = true;
      buttonPressStartTime = millis();
    }
  } else {
    buttonActive = false;
  }
  
  // If the button is active and held for more than 3 seconds
  if (buttonActive && (millis() - buttonPressStartTime > 3000)) {
    // Call the function to start the blocking configuration portal
    wifiManager.startConfigPortalBlocking();
  }


  // --- Your Main Code Continues to Run Normally Here ---
  Serial.print("Main loop running, time: ");
  Serial.println(millis());
  delay(2000);
}
