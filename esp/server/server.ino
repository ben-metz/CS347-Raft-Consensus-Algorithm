#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <espnow.h>
#include <SoftwareSerial.h>

char incomingPacket[255];  // Incoming packet buffer
char* replyPacket; // For reply

int values[] = {0, 0, 0, 0, 0}; // Value list to update

// For ESP NOW
uint8_t broadcastAddressSER[] = {0xA8, 0x48, 0xFA, 0xC1, 0xD8, 0x78};
uint8_t broadcastAddressCL1[] = {0xC4, 0x5B, 0xBE, 0x63, 0x61, 0xC2};
uint8_t broadcastAddressCL2[] = {0x98, 0xCD, 0xAC, 0x28, 0x45, 0x0A};

// For UART
SoftwareSerial mySerial(13, 15);

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("\nLast Packet Send Status: ");
  if (sendStatus == 0){
    Serial.print("Delivery success");
  }
  else{
    Serial.print("Delivery fail");
  }
}

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  if (len == 2){
    Serial.println("Recieved from SLAVE 1");
  } else if (len == 3){
    Serial.println("Recieved from SLAVE 2");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  mySerial.begin(9600);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddressCL1, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressCL2, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  if (mySerial.available())
    Serial.print("\nData Received over UART: ");
  while (mySerial.available())
    Serial.write(mySerial.read());
  
  char* hello = "hello";
  delay(1000);
  esp_now_send(broadcastAddressCL1, (uint8_t *) hello, sizeof(char) * 1);
  esp_now_send(broadcastAddressCL2, (uint8_t *) hello, sizeof(char) * 1);
}
