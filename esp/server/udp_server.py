# Simple python3 server for communicating with ESP8266 chip

import socket

def main():
    # IP of ESP and port used
    esp_ip = "192.168.1.124"
    esp_port = 12345

    # Define UDP socket and bind to port
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    sock.bind(("0.0.0.0", esp_port))

    while(True): # Send messages
        message = input("Enter message:")
        sock.sendto(bytes(message, 'utf-8'), (esp_ip, esp_port))

        while(True): # Wait for response (updated list)
            data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
            print(data.decode())
            break
    
if __name__ == "__main__":
    main()