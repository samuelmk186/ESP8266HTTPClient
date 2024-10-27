#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> 
#include <SPI.h>
#include <MFRC522.h>

const char* ssid = "Captain Mk";
const char* password = "##samuelmk186##"; 

#define SS_PIN D8
#define RST_PIN D0

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
byte nuidPICC[4];

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init(); 
  Serial.println();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println();
  Serial.println(F("This code scans the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent())
    return;

  if (!rfid.PICC_ReadCardSerial())
    return;

  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3]) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    // Send the data to the server
    sendRFIDData(rfid.uid.uidByte, rfid.uid.size);

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }
  else {
    Serial.println(F("Card read previously."));
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void sendRFIDData(byte *uid, byte uidSize) {
  // Connect to WiFi
  Serial.println("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Create JSON payload
  StaticJsonDocument<200> payload;
  payload["fname"] = "John"; 
  payload["lname"] = "Doe";
  payload["uid"] = byteArrayToHexString(uid, uidSize); 

  // Serialize JSON to string
  String jsonString;
  serializeJson(payload, jsonString);

  // Make HTTP request to server
  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://192.168.1.117/LSSEMS/insert_data.php"); 
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonString);;

  if (httpResponseCode == HTTP_CODE_OK) {
    // Read server response
    String response = http.getString();
    Serial.println("Server response: " + response);
  } else {
    Serial.println("HTTP request failed");
  }

  http.end();

  // Disconnect WiFi
  WiFi.disconnect();
}

String byteArrayToHexString(byte *byteArray, byte arraySize) {
  String hexString = "";
  for (byte i = 0; i < arraySize; i++) {
    hexString += (byteArray[i] < 0x10 ? "0" : "");
    hexString += String(byteArray[i], HEX);
  }
  return hexString;
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
