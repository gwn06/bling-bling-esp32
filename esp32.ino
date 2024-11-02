#include <FirebaseClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,16,2);

const char* ssid = "";
const char* password = "";

#define DATABASE_URL "URL"

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
NoAuth noAuth;

unsigned long duration, distance, prevDistance;
int ledPin = 2;
int buzzerPin = 5;
int triggerPin = 18;
int echoPin = 19;

int emptyTankDistance = 200 ;  // Distance when tank is empty
int fullTankDistance =  20 ;  // Distance when tank is full
int triggerLowPercentage =  0;  // alarm will start when water level drop below triggerPer
int triggerHighPercentage = 95;
int waterLevelPercentage = 0;

void printError(int code, const String &msg)
{
    Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}

void measureDistance() {
      // Set the trigger pin LOW for 2uS
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
  
    // Set the trigger pin HIGH for 20us to send pulse
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(20);
  
    // Return the trigger pin to LOW
    digitalWrite(triggerPin, LOW);
  
    // Measure the width of the incoming pulse
    duration = pulseIn(echoPin, HIGH);

    prevDistance = distance;
    distance = ((duration / 2) * 0.343) / 10;

    waterLevelPercentage = map((int)distance, emptyTankDistance, fullTankDistance, 0, 100);

    // waterLevelPercentage = (int)(((emptyTankDistance - distance) / (emptyTankDistance - fullTankDistance)) * 100);
    // Serial.println(distance);
    delay(200);
}

void displayData() {
  if (prevDistance == 0 || (distance == prevDistance)) return;
  // Serial.print("prev: ");
  // Serial.print(prevDistance);
  // Serial.print(" new: ");
  // Serial.print(distance);
  // Serial.println();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Distance: ");
  lcd.print(distance);
  lcd.print("cm");
  lcd.setCursor(0, 1);
  lcd.print("W. Level: ");
  lcd.print(waterLevelPercentage);
  lcd.print("%");
}

void initializeWifiConnection() {
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  // Serial.print("Connecting");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    // Serial.print(".");
    lcd.setCursor(10, 0);
    lcd.print(".");
  }

  // Serial.print("\nConnected to Wi-Fi network with IP Address: ");
  // Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connected");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(200);
}

void triggerWarningIndicator() {
  if (waterLevelPercentage >= triggerHighPercentage || waterLevelPercentage <= triggerLowPercentage) {
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(921600);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(2,0);
  lcd.print("Water Level");
  lcd.setCursor(3,1);
  lcd.print("Indicator");
  delay(500);
  initializeWifiConnection();

  Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
  ssl.setInsecure();

  // Initialize the authentication handler.
  initializeApp(client, app, getAuth(noAuth));

  // Binding the authentication handler with your Database class object.
  app.getApp<RealtimeDatabase>(Database);

  // Set your database URL
  Database.url(DATABASE_URL);

  // In sync functions, we have to set the operating result for the client that works with the function.
  client.setAsyncResult(result);
}

void loop() {
  
  measureDistance();
  while(distance == 0) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Measuring");
    lcd.setCursor(0,1);
    lcd.print("distance...");
    measureDistance();
  }

  displayData();
  triggerWarningIndicator();
  
  if (app.isInitialized() && app.ready() && distance != prevDistance && prevDistance != 0) {
      bool status = Database.set<int>(client, "/arduino/distance", distance);
  // if (status)
  //     Serial.println("ok");
  // else
  //     printError(client.lastError().code(), client.lastError().message());
  }
  delay(800);
}
