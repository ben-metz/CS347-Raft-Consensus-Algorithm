# python3 tkinter application for communicating with network

from tkinter import *
import tkinter.font as tkFont
import socket
import threading
import math
import time

from multilistbox import MultiListbox

names = ["Client", "Server 1", "Server 2", "Server 3", "Server 4", "Server 5"]

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
            decoded = data.decode()
            print(decoded)

            if ("Started" in decoded):
                con.configure(text='Connected', bg = "green")
                continue

            if ("Ended" in decoded):
                con.configure(text='Disconnected', bg = "red")
                continue

            split_decoded = decoded.split(':')
            text_boxes[int(data.decode()[0])].insert(0, 
                ('-',
                '-',
                '-',
                split_decoded[1],
                str(round(time.time(), 2))))

            con.configure(text='Connected', bg = "green")
        except:
            print("Error:\tDecode Error")

def rgb_hack(rgb):
    return '#%02x%02x%02x' % rgb

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
    textCol = rgb_hack((200, 200, 200))
    bgCol = rgb_hack((25, 25, 25))

    # Tkinter interface construction
    root = Tk()

    fontStyle = tkFont.Font(family="Piboto", size=20)
    titleFont = tkFont.Font(family="Piboto", size=32)

    root.configure(background=bgCol)

    root.title('ESP Interface')

    root_title = Label(root, text="Raft Consensus Algorithm Interface", font=titleFont)
    root_title.configure(foreground=textCol, background=bgCol)
    root_title.grid(row=0, column=2)

    # Input frame
    input_frame = Frame(root, bg=bgCol)
    input_frame.grid(row=1, column=1)

    input_title = Label(input_frame, text="Input", font=titleFont)
    input_title.configure(foreground=textCol, background=bgCol)
    input_title.grid(row=0, column=1)

    index_label = Label(input_frame, text="Index", font=fontStyle)
    index_label.configure(foreground=textCol, background=bgCol)
    index_label.grid(row=1, column=1)

    index = Entry(input_frame)
    index.grid(row=2, column=1)

    value_label = Label(input_frame, text="Value", font=fontStyle)
    value_label.configure(foreground=textCol, background=bgCol)
    value_label.grid(row=3, column=1)

    value = Entry(input_frame)
    value.grid(row=4, column=1)

    b = Button(input_frame,text='Send',command=send_command)
    b.grid(row=5, column=1)

    con = Button(input_frame,text='Disonnected', bg="red")
    con.grid(row=6, column=1)

    # # Config frame
    # config_frame = Frame(root, bg=bgCol)
    # config_frame.grid(row=1, column=5)

    # config_title = Label(config_frame, text="Config", font=titleFont)
    # config_title.configure(foreground=textCol, background=bgCol)
    # config_title.grid(row=6, column=1)

    # cfgs = []
    # cfg_buttons = []

    # for i in range(6):
    #     config_label = Label(config_frame, text="Config for " + names[i], font=fontStyle)
    #     config_label.configure(foreground=textCol, background=bgCol)
    #     config_label.grid(row=7 + 4*i, column=1)

    #     ip = Entry(config_frame)
    #     ip.grid(row=8 + 4*i, column=1)

    #     port = Entry(config_frame)
    #     port.grid(row=9 + 4*i, column=1)

    #     sub = Button(config_frame,text='Submit',command=send_command)
    #     sub.grid(row=10 + 4*i, column=1)

    #     cfgs.append([ip, port])

    # Border frames
    border1 = Frame(root, width = 40, height = 40, background=bgCol)
    border1.grid(row=0,column=0)

    border2 = Frame(root, width = 40, height = 40, background=bgCol)
    border2.grid(row=500,column=500)

    component_split = Frame(root, width = 1000, height = 40, background=bgCol)
    component_split.grid(row=500,column=2)

    # details frame
    details_frame = Frame(root, bg=bgCol)
    details_frame.grid(row=1, column=2)

    details_split = Frame(details_frame, width = 40, height = 40, background=bgCol)
    details_split.grid(row=500,column=2)

    text_boxes = []

    for i in range(0, 6):
        label = Label(details_frame, text=names[i], font=fontStyle)

        label.configure(foreground=textCol, background=bgCol)
        label.grid(row = 3 * math.floor(i/2), column = 3*(i - 2*math.floor(i/2)) + 1)

        text_box = MultiListbox(details_frame, (('State', 8), ('Term', 8), ('Voted For', 8), ('Array', 20), ('Time', 20)))
        text_box.grid(row = 1 + 3 * math.floor(i/2), column = 3*(i - 2*math.floor(i/2)) + 1)

        text_boxes.append(text_box)

    for i in range(100):
        text_boxes[0].insert(0, 
                    ('-',
                    '-',
                    '-',
                " split_decoded[1]",
                    str(round(time.time(), 2))))

    main()