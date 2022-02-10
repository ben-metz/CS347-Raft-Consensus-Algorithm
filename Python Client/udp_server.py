# python3 tkinter application for communicating with network

from tkinter import *
from interface import Interface

import tkinter.font as tkFont
import socket
import threading
import time
import json

# Sends command when send button pressed
def send_command():
    # Get the values from the send interface
    global index, value
    index_val = index.get()
    value_val = value.get()
    server_val = server.get()

    try:
        # Make sure valid input with conditionals
        if ((abs(int(value_val)) < 100) & (int(index_val) >= 0) & (int(index_val) < 5) &
                (int(server_val) >= 0) & (int(server_val) < 5) &
                (len(index_val) > 0) & (len(value_val) > 0) & (len(server_val))):

            # Construct JSON update message
            update_msg = {"message_type": "data_update", "data": {"server_id": int(
                server_val), "index": int(index_val), "value": int(value_val)}}

            # Send to Manager
            client_send_socket.sendto(bytes(json.dumps(update_msg), 'utf-8'),
                                      (client_ip, client_send_port))
        else:
            print('Error:\tInvalid Input')
    except:
        print('Error:\tInvalid Input')


# Takes incoming packets, splits up and places in correct text box
def handle_packets(client_receive_socket):
    while True:  # Wait for response (updated list)
        (data, addr) = client_receive_socket.recvfrom(
            1024)  # buffer size is 1024 bytes

        try:
            # Decode and load into json
            decoded = json.loads(data.decode())

            global start_time

            if (decoded['message_type'] == "details_update"):
                global connected, states
                if connected == False:
                    connected = True

                    start_time = round(time.time(), 2)

                    for i in range(5):
                        interfaces[i].connected()

                    con.configure(text='Connected', bg='green')

                # If details update, display in respective server
                data = decoded['data']

                interfaces[int(data['id'])].insert((states[data['state']],
                                                        data['term'], data['vote'], data['action'],
                                                        data['database'], str(round(time.time() - start_time, 2))))
            elif (decoded['message_type'] == "connection_status"):
                # If connection status, update connected status
                if decoded['data'] == 'started':
                    start_time = round(time.time(), 2)

                    connected = True

                    con.configure(text='Connected', bg='green')

                    for i in range(5):
                        interfaces[i].connected()

                    continue

                if decoded['data'] == 'ended':
                    con.configure(text='Disconnected', bg='red')
                    
                    for i in range(5):
                        interfaces[i].disconnected()

                    connected = False

                    continue
        except:
            print('Error:\tDecode Error')


def main():
    # Initialise thread for receiving packets
    receiver = threading.Thread(target=handle_packets,
                                args=(client_receive_socket, ),
                                daemon=True)
    receiver.start()

    # Begin UI loop
    root.mainloop()


if __name__ == '__main__':
    # IP of ESP and port used
    client_ip = '127.0.0.1'
    client_receive_port = 12345
    client_send_port = 12346

    connected = False

    start_time = 0

    states = {0:"Leader", 1:"Candidate", 2:"Follower"}

    # Define UDP socket and bind to port
    client_receive_socket = socket.socket(socket.AF_INET,
                                          socket.SOCK_DGRAM)  # UDP
    client_receive_socket.bind((client_ip, client_receive_port))

    client_send_socket = socket.socket(socket.AF_INET,
                                       socket.SOCK_DGRAM)  # UDP

    # Interface colours
    textCol = '#DDDDFF'
    bgCol = '#190C1C'

    # Tkinter interface construction
    root = Tk()
    root.geometry('1280x720')

    # Root frame
    root_frame = Frame(root, bg=bgCol)
    root_frame.place(relx=.5, rely=.5, anchor='center')

    fontStyle = tkFont.Font(family='Piboto', size=16)
    titleFont = tkFont.Font(family='Piboto', size=32)

    root.configure(background=bgCol)

    root.title('ESP Interface')

    root_title = Label(root_frame,
                       text='Raft Consensus Algorithm Interface',
                       font=titleFont)
    root_title.configure(foreground=textCol, background=bgCol)
    root_title.grid(row=0, column=3)

    con = Button(root_frame, text='Disonnected', bg='red',
                 borderwidth=0, highlightthickness=0)
    con.grid(row=0, column=3, sticky=W)

    # Details frame
    details_frame = Frame(root_frame, bg=bgCol)
    details_frame.grid(row=1, column=3)

    details_split = Frame(details_frame, width=40, height=0,
                          background=bgCol)
    details_split.grid(row=500, column=2)

    interfaces = []

    # Input frame
    input_frame = Frame(details_frame, bg=bgCol)
    input_frame.grid(row=7, column=4)

    # Inputs
    server_label = Label(input_frame, text='Server ID', font=fontStyle)
    server_label.configure(foreground=textCol, background=bgCol)
    server_label.grid(row=1, column=0)

    server = Entry(input_frame, borderwidth=0)
    server.grid(row=2, column=0)

    index_label = Label(input_frame, text='Index', font=fontStyle)
    index_label.configure(foreground=textCol, background=bgCol)
    index_label.grid(row=1, column=2)

    index = Entry(input_frame, borderwidth=0)
    index.grid(row=2, column=2)

    value_label = Label(input_frame, text='Value', font=fontStyle)
    value_label.configure(foreground=textCol, background=bgCol)
    value_label.grid(row=3, column=0)

    value = Entry(input_frame, borderwidth=0)
    value.grid(row=4, column=0)

    button_sep = Frame(input_frame, width=40, height=10,
                       background=bgCol)
    button_sep.grid(row=0, column=1)

    b = Button(input_frame, text='Send', command=send_command,
               borderwidth=0, highlightthickness=0)
    b.grid(row=4, column=2)

    # Border frames
    component_split = Frame(root_frame, width=25, height=40,
                            background=bgCol)
    component_split.grid(row=500, column=2)

    # Add the server displays
    for i in range(0, 5):
        interfaces.append(Interface(details_frame, textCol, bgCol, fontStyle, i, [client_send_socket, client_ip, client_send_port]))

    main()
