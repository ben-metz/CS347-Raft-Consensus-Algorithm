# python3 tkinter application for communicating with network

from tkinter import *
from interface import Interface
from state_injector import Injector

import tkinter.font as tkFont
import socket
import threading
import time
import json
import math
import subprocess
import signal
import atexit

def exit_handler():
    global proc, started

    if (started):
        proc.send_signal(signal.SIGINT)

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

# Clears the server interface logs
def clear_interfaces():
    for interface in interfaces:
        interface.clear()

# Blocks messages
def block_messages():
    global blocked, connected

    if (connected and not blocked):
        blocked = True
        con.configure(text='Disconnected', bg='red')
    elif (connected and blocked):
        blocked = False
        con.configure(text='Connected', bg='green')
    elif (not connected):
        blocked = True
        con.configure(text='Disconnected', bg='red')

# Terminates the current raft subprocess and spawns another
def restart_raft():
    global proc, started

    if (started):
        proc.terminate()
    else:
        started = True
        restart.configure(text='Restart', bg='green')

    proc = subprocess.Popen(["../Raft_Implementation/manager"])

    #clear_interfaces()

def send_all_tests():
    for i in range(0, len(injectors)):
        injectors[i].send_test()

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
                global connected, states, blocked
                if connected == False:
                    connected = True
                    blocked = False

                    start_time = round(time.time(), 2)

                    for i in range(5):
                        interfaces[i].connected()

                    con.configure(text='Connected', bg='green')

                # If details update, display in respective server
                data = decoded['data']

                if (not blocked):
                    interfaces[int(data['id'])].insert((states[data['state']],
                                                            data['term'], data['vote'], data['action'],
                                                            data['database'], data['lastCommited'], str(round(time.time() - start_time, 2))))
            elif (decoded['message_type'] == "connection_status"):
                # If connection status, update connected status
                if decoded['data'] == 'started':
                    start_time = round(time.time(), 2)

                    connected = True
                    blocked = False

                    con.configure(text='Connected', bg='green')

                    for i in range(5):
                        interfaces[i].connected()

                    continue

                if decoded['data'] == 'ended':
                    con.configure(text='Disconnected', bg='red')
                    
                    for i in range(5):
                        interfaces[i].disconnected()

                    connected = False
                    blocked = True

                    continue
        except:
            print('Error:\tDecode Error')

def switch_mode():
    global mode, mode_switch
    if (mode == 0):
        view_update_frame.pack_forget()
        testing_frame.pack(expand=1)
        mode_switch.configure(text='Switch to Overview')
        mode = 1
    else:
        view_update_frame.pack(expand=1)
        testing_frame.pack_forget()
        mode_switch.configure(text='Switch to Testing')
        mode = 0

def main():
    # Initialise thread for receiving packets
    receiver = threading.Thread(target=handle_packets,
                                args=(client_receive_socket, ),
                                daemon=True)
    receiver.start()

    # Begin UI loop
    root.mainloop()

    #proc = subprocess.Popen(["../Raft_Implementation/manager"])


if __name__ == '__main__':

    atexit.register(exit_handler)

    mode = 1

    # IP of ESP and port used
    client_ip = '127.0.0.1'
    client_receive_port = 12345
    client_send_port = 12346

    connected = False
    blocked = False

    started = False

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
    root.geometry("1280x720")

    fontStyle = tkFont.Font(family='Piboto', size=16)
    titleFont = tkFont.Font(family='Piboto', size=32)

    root_frame = Frame(root, background=bgCol)
    root_frame.pack(fill=BOTH, expand=1)

    root_title = Label(root_frame,
                       text='Raft Consensus Algorithm Interface',
                       font=titleFont)
    root_title.configure(foreground=textCol, background=bgCol)
    root_title.pack()

    mode_switch = Button(root_frame, text='Switch to Testing', bg='orange',command=switch_mode,
                 borderwidth=0, highlightthickness=0)
    mode_switch.pack()

    # Root frame
    view_update_frame = Frame(root_frame, bg=bgCol)
    view_update_frame.pack(expand=1)

    # Root frame
    testing_frame = Frame(root_frame, bg=bgCol)
    testing_frame.pack(expand=1)

    root.configure(background=bgCol)

    root.title('ESP Interface')

    # Button frame
    button_frame = Frame(view_update_frame, bg=bgCol)
    button_frame.pack(expand=1)

    con = Button(button_frame, text='Disconnected', bg='red', command=block_messages,
                 borderwidth=0, highlightthickness=0, width=15)
    con.pack(side=LEFT, padx='25', pady='5')

    restart = Button(button_frame, text='Start', bg='red',command=restart_raft,
                 borderwidth=0, highlightthickness=0,width=15)
    restart.pack(side=LEFT, padx='25')

    clear = Button(button_frame, text='Clear', bg='red',command=clear_interfaces,
                 borderwidth=0, highlightthickness=0, width=15)
    clear.pack(side=LEFT, padx='25')

    # Details frame
    details_frame = Frame(view_update_frame, bg=bgCol)
    details_frame.pack(expand=1)

    # Input frame
    input_frame = Frame(details_frame, bg=bgCol, pady='10')
    input_frame.pack(expand=1)

    # Inputs
    server_label = Label(input_frame, text='Server ID', font=fontStyle)
    server_label.configure(foreground=textCol, background=bgCol)
    server_label.grid(row=1, column=0)

    server = Entry(input_frame, borderwidth=0)
    server.grid(row=2, column=0, padx=10)

    index_label = Label(input_frame, text='Index', font=fontStyle)
    index_label.configure(foreground=textCol, background=bgCol)
    index_label.grid(row=1, column=1)

    index = Entry(input_frame, borderwidth=0)
    index.grid(row=2, column=1, padx=10)

    value_label = Label(input_frame, text='Value', font=fontStyle)
    value_label.configure(foreground=textCol, background=bgCol)
    value_label.grid(row=1, column=2)

    value = Entry(input_frame, borderwidth=0)
    value.grid(row=2, column=2, padx=10)

    send = Button(input_frame, text='Send', command=send_command,
               borderwidth=0, highlightthickness=0)
    send.grid(row=2, column=3)

    interface_frames = []
    testing_frames = []

    for i in range(0, 3):
        new_frame = Frame(details_frame, bg=bgCol)
        interface_frames.append(new_frame)
        new_frame.pack(expand=1)

        new_test_frame = Frame(testing_frame, bg=bgCol)
        testing_frames.append(new_test_frame)
        new_test_frame.pack(expand=1)

    interfaces = []
    injectors = []

    send_all = Button(testing_frame, text='Send All', bg='red', command=send_all_tests,
               borderwidth=0, highlightthickness=0, width=50)
    send_all.pack()

    # Add the server displays
    for i in range(0, 5):
        interfaces.append(Interface(interface_frames[math.floor(i/2)], textCol, bgCol, fontStyle, i, [client_send_socket, client_ip, client_send_port]))

        injectors.append(Injector(testing_frames[math.floor(i/2)], textCol, bgCol, fontStyle, i, [client_send_socket, client_ip, client_send_port]))

    switch_mode()

    # Begin raft c++ implementation
    print("Building C++ Raft Implementation...")
    proc = subprocess.Popen(["make"], stdout=subprocess.PIPE, cwd="../Raft_Implementation/")
    proc.wait()
    print("Completed")

    main()
