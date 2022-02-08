// SERVER

#include <esp_raft.h>
#include <SoftwareSerial.h>

char incomingPacket[255];  // Incoming packet buffer
char* replyPacket; // For reply

DETAILS* details;

char* uart_input_buffer;

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
  Serial.println("Recieved");
}

void sendStats(){
  esp_now_send(broadcastAddressSTAT, (uint8_t *) details, sizeof(DETAILS));
}

void setup()
{
  Serial.begin(115200);
  Serial.println();

  mySerial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  details = (DETAILS*) malloc(sizeof(DETAILS));
  details -> id = 0;
  details -> term = 1;
  details -> voted_for = -1;
  details -> state = 0;

  for (int i = 0; i < 5; i++){
    details -> values[i] = 0;
  }

  uart_input_buffer = (char*) malloc(sizeof(char) * 100);

  // Set ESP-NOW Role
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddressCL1, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressCL2, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_add_peer(broadcastAddressSTAT, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
  if (mySerial.available()){
    Serial.print("\nData Received over UART: ");

    int i = 0;
    while (mySerial.available() && i < 100){
      uart_input_buffer[i] = mySerial.read();
      Serial.write(uart_input_buffer[i]);
      i++;
    }
    uart_input_buffer[i] = 0;

    char *j;
    charArrToInt(strtok_r(uart_input_buffer, " ", &j), &(details -> values[0]));
    for (int k = 0; k < 4; k++){
      charArrToInt(strtok_r(NULL, " ", &j), &(details -> values[k + 1]));
    }
    Serial.println();
    Serial.println(details -> values[0]);
  }
  
  sendStats();
  delay(250);
}

// Converts passed char array to int and stores in place
void charArrToInt(char* chars, int* place){
  int i = 0;
  *place = 0;
  while(chars[i] != 0){
    *place*=10;
    *place += chars[i] - 48;
    i++;
  }
}
