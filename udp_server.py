# python3 tkinter application for communicating with ESP8266 network

from tkinter import *
import socket
import threading
import math

# Sends command when Send pressed
def send_command():
    global index, value
    index_val = index.get() 
    value_val = value.get()

    try:
        if ((int(index_val) >= 0) & (int(index_val) < 5)):
            string = str(int(index_val)) + ' ' + str(int(value_val))
            sock.sendto(bytes(string, 'utf-8'), (esp_ip, esp_port))
        else:
            print("Error:\tInvalid Input")
    except:
        print("Error:\tInvalid Input")

# Takes incoming packets, splits up and places in correct text box
def handle_packets(sock):
    while(True): # Wait for response (updated list)
        data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
        try:
            split_decoded = data.decode().split(':')
            text_boxes[int(data.decode()[0])].insert('1.0', "State: " + split_decoded[1] + "    Array: " + split_decoded[2] + '\n')
        except:
            print("Error:\tDecode Error")

def main():
    x = threading.Thread(target=handle_packets, args=(sock,), daemon=True)
    x.start()

    root.mainloop()
        
if __name__ == "__main__":
    # IP of ESP and port used
    esp_ip = "192.168.1.125"
    esp_port = 12345

    # Define UDP socket and bind to port
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    sock.bind(("0.0.0.0", esp_port))

    # Tkinter interface construction
    root = Tk()

    root.title('ESP Interface')

    index_label = Label(root, text="Index")
    index_label.grid(row=0, column=1)

    index = Entry(root)
    index.grid(row=1, column=1)

    value_label = Label(root, text="Value")
    value_label.grid(row=2, column=1)

    value = Entry(root)
    value.grid(row=3, column=1)

    b = Button(root,text='Send',command=send_command)
    b.grid(row=4, column=1)

    text_boxes = []

    for i in range(0, 6):
        label = Label(root, text="ESP "  + str(i))
        label.grid(row = 5 + 2 * math.floor(i/3), column = i - 3*math.floor(i/3))

        text_box = Text(
            root,
            height=16,
            width=64
        )

        text_box.grid(row = 6 + 2 * math.floor(i/3), column = i - 3*math.floor(i/3))

        text_boxes.append(text_box)

    main()