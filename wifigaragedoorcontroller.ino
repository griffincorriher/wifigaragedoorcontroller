#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif

#define SERIAL_BAUDRATE     9600
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASSWORD"

#define DOOR1MAGNET1         13  // d7
#define DOOR1CONTROL         12  // d6
#define DOOR1MAGNET2         14  // d5

#define DOOR2CONTROL         5   // d1
#define WIFI_LED             4   // d2
#define DOOR2MAGNET1         2   // d4
#define DOOR2MAGNET2         0   // d3

// Initial door status value
int door1magnet1status = 1;
int door1magnet2status = 1;
int door2magnet1status = 1;
int door2magnet2status = 1;

// Last door status value
unsigned long lastDoor1Magnet1Change = 0;
unsigned long lastDoor1Magnet2Change = 0;
unsigned long lastDoor2Magnet1Change = 0;
unsigned long lastDoor2Magnet2Change = 0;

// Garage door intervals
static unsigned long lastDoorStateCheck = 0;
const int numButtons = 2;  // Number of garage door buttons
const long debounceDelay = 10;  // Debounce delay for button toggle
const long doorStateInterval = 1000;  // Interval for checking door status
const long magnetDebounceDelay = 500;   // Debounce delay for magnetic read switch

// Wifi intervals and led state
bool ledState = LOW;                      // Tracks the current state of the LED
unsigned long wifi_led_previousMillis = 0; // Tracks the last time the LED was updated
const long notconnected_led_interval = 20; // Interval for NOT CONNECTED state
const long connected_led_on_interval = 100; // ON time when CONNECTED
const long connected_led_off_interval = 10000; // OFF time when CONNECTED

char command[30] = "default";

// Array to store the last press time for each button
unsigned long lastPressTime[numButtons] = {0, 0};  // One entry for each button

// Maybe switch to this to be more dynamic?
//std::vector<unsigned long> lastPressTime(numButtons, 0); // Initializes all to 0


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
  
  if (currentMillis - lastDoorStateCheck >= doorStateInterval) {
    lastDoorStateCheck = currentMillis;
    get_door_states();
  }
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
    if (strcmp(command, "wifioff") == 0) {
      Serial.println("Turning off wifi");
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      
    } else if (strcmp(command, "wifion") == 0) {
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


void get_door_states() {
  unsigned long currentMillis = millis();
  
  int currentDoor1Magnet1Status = digitalRead(DOOR1MAGNET1);
  int currentDoor1Magnet2Status = digitalRead(DOOR1MAGNET2);
  int currentDoor2Magnet1Status = digitalRead(DOOR2MAGNET1);
  int currentDoor2Magnet2Status = digitalRead(DOOR2MAGNET2);

  // Check and print changes for door1magnet1
  if (currentDoor1Magnet1Status != door1magnet1status && currentMillis - lastDoor1Magnet1Change > magnetDebounceDelay) {
    door1magnet1status = currentDoor1Magnet1Status;
    lastDoor1Magnet1Change = currentMillis; // Update the debounce timer
    Serial.print("Door1 Magnet1 changed to: ");
    Serial.println(door1magnet1status);
  }

  // Check and print changes for door1magnet2
  if (currentDoor1Magnet2Status != door1magnet2status && currentMillis - lastDoor1Magnet2Change > magnetDebounceDelay) {
    door1magnet2status = currentDoor1Magnet2Status;
    lastDoor1Magnet2Change = currentMillis; // Update the debounce timer
    Serial.print("Door1 Magnet2 changed to: ");
    Serial.println(door1magnet2status);
  }

  // Check and print changes for door2magnet1
  if (currentDoor2Magnet1Status != door2magnet1status && currentMillis - lastDoor2Magnet1Change > magnetDebounceDelay) {
    door2magnet1status = currentDoor2Magnet1Status;
    lastDoor2Magnet1Change = currentMillis; // Update the debounce timer
    Serial.print("Door2 Magnet1 changed to: ");
    Serial.println(door2magnet1status);
  }

  // Check and print changes for door2magnet2
  if (currentDoor2Magnet2Status != door2magnet2status && currentMillis - lastDoor2Magnet2Change > magnetDebounceDelay) {
    door2magnet2status = currentDoor2Magnet2Status;
    lastDoor2Magnet2Change = currentMillis; // Update the debounce timer
    Serial.print("Door2 Magnet2 changed to: ");
    Serial.println(door2magnet2status);
  }
}
