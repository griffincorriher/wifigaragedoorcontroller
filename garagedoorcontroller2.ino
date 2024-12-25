#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif

#define SERIAL_BAUDRATE     9600
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASSWORD"

#define DOOR1CONTROL         13  // d7
#define DOOR1MAGNET1         12  // d6
#define DOOR1MAGNET2         14  // d5

#define DOOR2CONTROL             5   // d1
#define WIFI_LED         4   // d2
#define DOOR2MAGNET1         0   // d3
#define DOOR2MAGNET2         2   // d4



unsigned long wifi_led_previousMillis = 0; // Tracks the last time the LED was updated
unsigned long led_interval = 200;         // Initial interval
bool ledState = LOW;                      // Tracks the current state of the LED
const long notconnected_led_interval = 20; // Interval for NOT CONNECTED state
const long connected_led_on_interval = 100; // ON time when CONNECTED
const long connected_led_off_interval = 10000; // OFF time when CONNECTED
char command[30] = "default";

const int numButtons = 2;       
const long debounceDelay = 10;

// Array to store the last press time for each button
unsigned long lastPressTime[numButtons] = {0, 0};  // One entry for each button

void setup() {
  
    Serial.begin(SERIAL_BAUDRATE);
    
    pinMode(DOOR1CONTROL, OUTPUT);
    pinMode(DOOR1MAGNET1, INPUT);
    pinMode(DOOR1MAGNET2, INPUT);
    
    pinMode(WIFI_LED, OUTPUT);
    pinMode(DOOR2CONTROL, OUTPUT);
    pinMode(DOOR2MAGNET1, INPUT);
    pinMode(DOOR2MAGNET2, INPUT);

    wifiSetup();
}

void loop() {
  flash_wifi_led();
  take_serial_commands();  
}


void toggle_button(int buttonPin, int buttonIndex) {
    unsigned long currentTime = millis(); // Get the current time

    // Debounce logic: Ensure enough time has passed since the last press
    if (currentTime - lastPressTime[buttonIndex] >= debounceDelay) {
        // Simulate a momentary button press
        digitalWrite(buttonPin, HIGH);
        delay(5000); // Button press duration
        digitalWrite(buttonPin, LOW);

        // Update the last press time for this button
        lastPressTime[buttonIndex] = currentTime;
    }
}

// Set up wifi
void wifiSetup() {

    // Set WIFI module to STA mode
    WiFi.mode(WIFI_STA);

    // Connect
    Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Wait
    while (WiFi.status() != WL_CONNECTED) {
        flash_wifi_led();
        Serial.print(".");
        yield();
    }
    Serial.println();

    // Connected!
    Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());

    // Maintain connection after wifi off/on
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    digitalWrite(WIFI_LED, LOW);
}

// Flash wifi led based on connection status
void flash_wifi_led() {
  unsigned long currentMillis = millis(); // Get the current time

  if (WiFi.status() == WL_CONNECTED) {
    // Handle flashing for CONNECTED status
    if (ledState == HIGH && (currentMillis - wifi_led_previousMillis >= connected_led_on_interval)) {
      ledState = LOW; // Turn off the LED
      wifi_led_previousMillis = currentMillis; // Update the timestamp
      digitalWrite(WIFI_LED, ledState); // Update the LED state
    } else if (ledState == LOW && (currentMillis - wifi_led_previousMillis >= connected_led_off_interval)) {
      ledState = HIGH; // Turn on the LED
      wifi_led_previousMillis = currentMillis; // Update the timestamp
      digitalWrite(WIFI_LED, ledState); // Update the LED state
    }
  } else {
    // Handle flashing for NOT CONNECTED status
    if (currentMillis - wifi_led_previousMillis >= notconnected_led_interval) {
      ledState = !ledState; // Toggle the LED state
      wifi_led_previousMillis = currentMillis; // Update the timestamp
      digitalWrite(WIFI_LED, ledState); // Update the LED state
    }
  }
  yield();
}


// Send wifi and door commands through serial for testing
void take_serial_commands() {
  if (Serial.available() > 0) {
    Serial.println("Serial available");

    // Read the incoming data into the buffer
    size_t len = Serial.readBytesUntil('\n', command, sizeof(command) - 1);
    command[len] = '\0'; // Ensure null-termination

    // Debugging: Print the raw command received
    Serial.print("Raw command: '");
    Serial.print(command);
    Serial.println("'");

    // Trim trailing whitespace
    trim(command);

    // Debugging: Print the trimmed command
    Serial.print("Trimmed command: '");
    Serial.print(command);
    Serial.println("'");

    // Check if the command matches "off"
    if (strcmp(command, "off") == 0) {
      Serial.println("Turning off wifi");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
    } else if (strcmp(command, "on") == 0) {
      Serial.println("Turning on wifi");
      wifiSetup();
    } else if (strcmp(command, "toggledoor1") == 0) {
      Serial.println("Toggling door 1");
      toggle_button(DOOR1CONTROL, 0);
    } else if (strcmp(command, "toggledoor2") == 0) {
      Serial.println("Toggling door 2");
      toggle_button(DOOR2CONTROL, 1);
    } else {
      Serial.println("Unknown command");
    }
  }
}

// Function to trim trailing whitespace
void trim(char* str) {
  size_t len = strlen(str);
  while (len > 0 && isspace(str[len - 1])) {
    str[len - 1] = '\0';
    len--;
  }
}
