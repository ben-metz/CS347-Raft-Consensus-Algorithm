// STATE COLLECTOR

#include <esp_raft.h>
#include <SoftwareSerial.h>

char incomingPacket[255];  // Incoming packet buffer
char* replyPacket; // For reply

DETAILS* incomingDetails;

SoftwareSerial detailsSerial(12, 14);

// Callback when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(incomingDetails, incomingData, sizeof(DETAILS));

  Serial.println("\nReceived");
  Serial.print("ID: ");
  Serial.println(incomingDetails -> id);
  Serial.print("STATE: ");
  Serial.println(incomingDetails -> state);

  detailsSerial.print(incomingDetails -> id);
  detailsSerial.print(':');
  detailsSerial.print(incomingDetails -> state);
  detailsSerial.print(':');

  for (int i = 0; i < 5; i++){
    detailsSerial.print(incomingDetails -> values[i]);
    detailsSerial.print(' ');
    Serial.print(incomingDetails -> values[i]);
    Serial.print(" ");
  }
  Serial.println();
}

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  detailsSerial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  incomingDetails = (DETAILS*) malloc(sizeof(DETAILS));

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddressSER, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressCL1, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressCL2, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{

}
