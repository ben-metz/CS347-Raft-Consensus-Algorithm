# python3 tkinter application for communicating with network

from tkinter import *
import tkinter.font as tkFont
import socket
import threading
import math
import time

from multilistbox import MultiListbox

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
        print(data.decode())
        # try:
        #     split_decoded = data.decode().split(':')
        #     text_boxes[int(data.decode()[0])].insert(0, 
        #         (split_decoded[1],
        #         split_decoded[2],
        #         split_decoded[3],
        #         split_decoded[4],
        #         str(round(time.time(), 2))))
        # except:
        #     print("Error:\tDecode Error")

def main():
    # Initialise thread for receiving packets
    receiver = threading.Thread(target=handle_packets, args=(sock,), daemon=True)
    receiver.start()

    # Begin UI loop
    root.mainloop()
        
if __name__ == "__main__":
    # IP of ESP and port used
    esp_ip = "127.0.0.1"
    esp_port = 12345

    # Define UDP socket and bind to port
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # UDP
    sock.bind((esp_ip, esp_port))

    # Interface colours
    textCol = "black"
    bgCol ="light grey"

    # Tkinter interface construction
    root = Tk()

    fontStyle = tkFont.Font(family="Piboto", size=20)

    print(tkFont.families())

    root.configure(background=bgCol)

    root.title('ESP Interface')

    index_label = Label(root, text="Index", font=fontStyle)
    index_label.configure(foreground=textCol, background=bgCol)
    index_label.grid(row=0, column=3)

    index = Entry(root)
    index.grid(row=1, column=3)

    value_label = Label(root, text="Value", font=fontStyle)
    value_label.configure(foreground=textCol, background=bgCol)
    value_label.grid(row=2, column=3)

    value = Entry(root)
    value.grid(row=3, column=3)

    b = Button(root,text='Send',command=send_command)
    b.grid(row=4, column=3)

    text_boxes = []

    for i in [0, 2, 4, 6]:
        root.grid_columnconfigure(i, minsize=25)

    for i in range(0, 6):
        if (i == 0):
            label = Label(root, text="SERVER", font=fontStyle)
        else:
            label = Label(root, text="ESP "  + str(i), font=fontStyle)

        label.configure(foreground=textCol, background=bgCol)
        label.grid(row = 5 + 2 * math.floor(i/3), column = 2*(i - 3*math.floor(i/3)) + 1)

        text_box = MultiListbox(root, (('State', 8), ('Term', 8), ('Voted For', 8), ('Array', 20), ('Time', 20)))

        text_box.grid(row = 6 + 2 * math.floor(i/3), column = 2*(i - 3*math.floor(i/3)) + 1)

        text_boxes.append(text_box)

    main()