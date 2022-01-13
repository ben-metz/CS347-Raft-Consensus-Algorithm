// UDP COMMAND REVIEVER/DETAIL SENDER

#include <SoftwareSerial.h>

#include <esp_raft.h>

// WiFi creds
const char* ssid = "TALKTALK-68ED50";
const char* password = "Y8JDX87Y";

WiFiUDP Udp;
unsigned int localUdpPort = 12345;  // Port to listen on
char incomingPacket[255];  // Incoming packet buffer
char* replyPacket; // For reply

DETAILS* details;

// For UART
SoftwareSerial mySerial(13, 15);
SoftwareSerial detailsSerial(12, 14);

void setup()
{
  Serial.begin(115200);
  Serial.println();

  mySerial.begin(9600);
  detailsSerial.begin(115200);

  // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  details = (DETAILS*) malloc(sizeof(DETAILS));
  details -> id = -1;
  details -> state = -1;

  for (int i = 0; i < 5; i++){
    details -> values[i] = 0;
  }

  // Begin listening on UDP
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", 
    WiFi.localIP().toString().c_str(), localUdpPort);

  replyPacket = (char*) malloc(sizeof(char)*100);
}

void loop()
{ 
  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    // Receive UDP packets
    Serial.printf("\nReceived %d bytes from %s, port %d\n", packetSize, 
      Udp.remoteIP().toString().c_str(), Udp.remotePort());
    int len = Udp.read(incomingPacket, 255);
    if (len > 0)
    {
      incomingPacket[len] = 0;
    }
    
    Serial.printf("UDP packet contents: %s\n", incomingPacket);

    handleMessage(incomingPacket, replyPacket);

    sprintf(replyPacket, "%d %d %d %d %d", 
      details->values[0], details->values[1], details->values[2], details->values[3], details->values[4]);
    
    // send back a reply, to the IP address and port we got the packet from
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    
    Udp.write(replyPacket);
    mySerial.write(replyPacket);
    
    Udp.endPacket();
  }

  if (detailsSerial.available()){
    while(detailsSerial.available()){
      Serial.write(detailsSerial.read());
    }
    Serial.println();
  }
}

// Takes the message and applies change to list
void handleMessage(char* message, char* replyPacket){
  char *index, *new_value, *i;

  // Load info into strings
  index = strtok_r(message, " ", &i);
  new_value = strtok_r(NULL, " ", &i);

  // Note change to be applied
  Serial.print("\nSetting index ");
  Serial.printf(index);
  Serial.print(" to ");
  Serial.printf(new_value);
  Serial.println();

  // Convert the strings to integers for allocation
  int* int_index = (int*) malloc(sizeof(int));
  *int_index = 0;
  
  charArrToInt(index, int_index);
  charArrToInt(new_value, &(details->values[*int_index]));
  
  free(int_index);

  // List new values
  for (int i = 0; i < 5; i++){
    Serial.print(details->values[i]);
    Serial.print(" ");
  }
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
