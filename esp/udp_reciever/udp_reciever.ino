#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <espnow.h>

// WiFi creds
const char* ssid = "TALKTALK-68ED50";
const char* password = "Y8JDX87Y";

WiFiUDP Udp;
unsigned int localUdpPort = 12345;  // Port to listen on
char incomingPacket[255];  // Incoming packet buffer
char* replyPacket; // For reply

int values[] = {0, 0, 0, 0, 0}; // Value list to update

// For UART
SoftwareSerial mySerial(13, 15);

void setup()
{
  Serial.begin(115200);
  Serial.println();

  mySerial.begin(9600);

  // Connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

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

    mySerial.write(incomingPacket);

    handleMessage(incomingPacket, replyPacket);

    sprintf(replyPacket, "%d,%d,%d,%d,%d", 
      values[0], values[1], values[2], values[3], values[4]);
    
    // send back a reply, to the IP address and port we got the packet from
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(replyPacket);
    Udp.endPacket();
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
  charArrToInt(new_value, &values[*int_index]);
  
  free(int_index);

  // List new values
  for (int i = 0; i < 5; i++){
    Serial.print(values[i]);
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
