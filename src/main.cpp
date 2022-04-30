#include <Arduino.h>
#include <FirebaseESP32.h>
#include <WiFi.h>

#include "addons/RTDBHelper.h"
#include "addons/TokenHelper.h"
//#include "firebase/app.h"

// every device must have a unique serial number
// devices sold in pairs will have sn = "xxxA"and "xxxB"
#define serial_number_definition "1"
#define device_definition "B"

// TODO: allow user to input wifi credentials without reflash
#define ssid "howdy"
#define password "6462211294"

// Firebase Database Credentials
// Should never change
#define database "https://esp32-3005c-default-rtdb.firebaseio.com/"
#define firebase_key "AIzaSyBWaT7LSAQqzAwA1l1C26uBlMtM_BF4RNA"

#define update_interval 10000
#define updatep_interval 5000

#define pressed 1
#define unpressed 0

#define led_pin 17
#define button_pin 27

// Firebase R-T Database Object, Authentication Object, configuration Object
FirebaseData reader;
FirebaseData writer;
FirebaseAuth auth;
FirebaseConfig config;

// Firebase
String uid = "";
String serial_number = serial_number_definition;
String device = device_definition;
String write_path = "/" + serial_number + "/" + device;
String read_path;

bool authenticated = false;

// Data
unsigned long ms = 0;
unsigned long prev_ms = 0;
unsigned long update = 0;
int data = 0;
int state = 0;
bool pressing = false;

// pressing
unsigned long msp = 0;
unsigned long prev_msp = 0;
unsigned long updatep = 0;
/*
Connects to wifi, called in setup
*/
void init_wifi() {
  // Establish connection
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  // Success
  Serial.println();
  Serial.print("Success! IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

/*
Connects to Firebase Real-Time Database
*/
void firebase_init() {
  // define key, database, and connection
  config.api_key = firebase_key;
  config.database_url = database;
  Firebase.reconnectWiFi(true);

  // sign in anonymously
  if (Firebase.signUp(&config, &auth, "", "")) {
    // Authenticated Successfully
    authenticated = true;
    uid = auth.token.uid.c_str();

    // Assign the callback function for the long running token generation task,
    // see addons/TokenHelper.h
    config.token_status_callback = tokenStatusCallback;
    // Start Firebase!
    Firebase.begin(&config, &auth);
  } else {
    // Authentication Failed
    authenticated = false;
  }
}

/*
Every update_interval ms, read in the LED state of the PARTNER ESP32 from the
database
*/
int read_database() {
  prev_ms = ms;
  ms = millis();
  update = update + (ms - prev_ms);

  if (update > update_interval && Firebase.ready()) {
    // read
    update = 0;

    Firebase.getInt(reader, read_path.c_str());

    state = reader.intData();
  }
  return state;
}

void listen() {
  Firebase.readStream(reader);
  if (reader.streamAvailable()) {
    state = reader.intData();
  }
}

void write_database() {
  prev_msp = msp;
  msp = millis();
  updatep = updatep + (msp - prev_msp);

  if (pressing) {
    updatep = 0;
    if (data == unpressed) {
      Firebase.set(writer, write_path, pressed);
      data = pressed;
      Serial.println("writing press!");
    }
  } else if (!pressing && data == pressed && updatep > updatep_interval) {
    Firebase.set(writer, write_path, unpressed);
    data = unpressed;
    Serial.println("writing unpress!");
  }

  if (updatep > updatep_interval) {
  }
}

/*
Updates the state of the LED based on the value of state
Updates data based on the value of the button/touch sensor
*/
void update_state(int state) {
  pressing = digitalRead(button_pin);
  digitalWrite(led_pin, state);
}

void setup() {
  pinMode(led_pin, OUTPUT);
  pinMode(button_pin, INPUT);

  Serial.begin(9600);

  init_wifi();
  while (!authenticated) {
    firebase_init();
  }

  if (device == "A") {
    read_path = "/" + serial_number + "/" + "B";
  } else if (device == "B") {
    read_path = "/" + serial_number + "/" + "A";
  }

  Firebase.getInt(writer, write_path);
  data = reader.intData();

  Firebase.beginStream(reader, read_path);
}

void loop() {
  // state = read_database();
  listen();

  update_state(state);

  write_database();
}