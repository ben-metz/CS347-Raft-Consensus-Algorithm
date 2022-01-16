#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <espnow.h>

#define VALUES 5

// For ESP NOW
uint8_t broadcastAddressSER[] = {0xA8, 0x48, 0xFA, 0xC1, 0xD8, 0x78};
uint8_t broadcastAddressCL1[] = {0xC4, 0x5B, 0xBE, 0x63, 0x61, 0xC2};
uint8_t broadcastAddressCL2[] = {0x98, 0xCD, 0xAC, 0x28, 0x45, 0x0A};
uint8_t broadcastAddressSTAT[] = {0xC4, 0x5B, 0xBE, 0x62, 0xC3, 0x73};

typedef struct struct_message {
    int id;
    int state;
    int term;
    int voted_for;
    int values[VALUES];
} DETAILS;
