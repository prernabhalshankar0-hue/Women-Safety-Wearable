/*
 * Project: Women Safety Device with GPS & GSM Alerts
 * Author: Prerna & Team
 * Board: ESP32 Dev Module
 */

#include <HardwareSerial.h>
#include <TinyGPS++.h>

// --- Hardware Pin Definitions ---
const int SOS_BUTTON_PIN = 32;
const int BUZZER_PIN     = 13;
const int TORCH_PIN      = 12;
const int PULSE_PIN      = 34;

// Serial Communication Pins
const int GSM_RX = 16;
const int GSM_TX = 17;
const int GPS_RX = 4;
const int GPS_TX = 2;

// --- Configurable Settings ---
const String EMERGENCY_NUMBER = "+919876543210"; // Ithe tumcha primary phone number taka

// --- Global Objects ---
HardwareSerial gsmSerial(2); // UART2
HardwareSerial gpsSerial(1); // UART1
TinyGPSPlus gps;

// Tracking Variables
unsigned long lastButtonPressTime = 0;
const unsigned long debounceDelay = 300; // Debounce window (ms)

void setup() {
  Serial.begin(115200);
  
  // Configure Input/Output Pins
  pinMode(SOS_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TORCH_PIN, OUTPUT);

  // Default Pin States
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(TORCH_PIN, LOW);

  // Initialize Hardware Serials
  gsmSerial.begin(9600, SERIAL_8N1, GSM_RX, GSM_TX);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  Serial.println("[SYSTEM] Initializing Women Safety Device...");
  delay(2000);

  initGSMModule();
  Serial.println("[SYSTEM] Device Ready.");
}

void loop() {
  // Read GPS stream continuously to keep coordinates updated
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // Check SOS Button status with debounce check
  if (digitalRead(SOS_BUTTON_PIN) == LOW) {
    if (millis() - lastButtonPressTime > debounceDelay) {
      lastButtonPressTime = millis();
      Serial.println("[ALERT] SOS Triggered!");
      handleEmergencySequence();
    }
  }
}

// --- Core Emergency Routine ---
void handleEmergencySequence() {
  // 1. Audio and Visual indication
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(TORCH_PIN, HIGH);

  // 2. Fetch Location Data
  String googleMapsUrl = getGpsLocationUrl();
  String alertMessage = "EMERGENCY ALERT! I am in danger. My live location: " + googleMapsUrl;

  // 3. Dispatch SMS
  Serial.println("[GSM] Sending SOS SMS...");
  sendSms(EMERGENCY_NUMBER, alertMessage);

  // 4. Trigger Direct Call
  delay(2000);
  Serial.println("[GSM] Initiating Emergency Call...");
  placeCall(EMERGENCY_NUMBER);

  // Reset alert indicators after dispatch
  delay(4000);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(TORCH_PIN, LOW);
}

// --- Helper Functions ---
String getGpsLocationUrl() {
  if (gps.location.isValid()) {
    return "https://maps.google.com/?q=" + 
           String(gps.location.lat(), 6) + "," + 
           String(gps.location.lng(), 6);
  } else {
    return "https://maps.google.com/?q=19.8762,75.3433"; // Fallback location if GPS fix fails
  }
}

void initGSMModule() {
  gsmSerial.println("AT");
  delay(1000);
  gsmSerial.println("AT+CMGF=1"); // Set SMS to Text Mode
  delay(1000);
}

void sendSms(String number, String msg) {
  gsmSerial.println("AT+CMGF=1");
  delay(500);
  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(number);
  gsmSerial.println("\"");
  delay(500);
  gsmSerial.print(msg);
  delay(500);
  gsmSerial.write(26); // End-of-message character (CTRL+Z)
  delay(4000);
}

void placeCall(String number) {
  gsmSerial.print("ATD");
  gsmSerial.print(number);
  gsmSerial.println(";");
}
