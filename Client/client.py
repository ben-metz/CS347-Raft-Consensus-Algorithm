# Multiprocess client application for raft consensus algorithm

from tkinter import *
import tkinter.font as tkFont
import math
from server import Server
from threading import *
import signal
import time
from multilist_box import MultiListbox

servers = [] # List of servers

server_count = 6

# Sends command when Send pressed
def send_command():
    print("Sending Command...") # Will do something when data library implemented

# Function performed by the data collector thread
def collector_thread_function():
    try:
        while True:
            time.sleep(0.1)
            for i in range(0, len(servers)):
                details = servers[i].getDetails()
                text_boxes[int(details[0])].insert(0, (details[1], details[2], 
                    details[3], "SAMPLE DATA", str(round(time.time(), 2))))
    except(KeyboardInterrupt):
        print("Keyboard exit")

def main():
    root.mainloop()

if __name__=='__main__':
    # Initialise the servers
    for i in range(0, server_count):
        server = Server(str(i))
        servers.append(server)

    # Initialise neighbours
    for i in range(0, server_count):
        for j in range(0, server_count):
            if (i != j):
                servers[i].addNeighbour(servers[j])

    # Start the servers
    for i in range(0, server_count):
        servers[i].start()

    # Initialise the master thread
    master_thread = Thread(target=collector_thread_function, args=( ))
    master_thread.daemon = True # Thread ends when main process ends
    master_thread.start()

    # Interface colours
    textCol = "black"
    bgCol ="light grey"

    # Tkinter interface construction
    root = Tk()

    fontStyle = tkFont.Font(family="Piboto", size=20)

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

    for i in range(0, server_count):
        label = Label(root, text="SERVER "  + str(i+1), font=fontStyle)

        label.configure(foreground=textCol, background=bgCol)
        label.grid(row = 5 + 2 * math.floor(i/3), column = 2*(i - 3*math.floor(i/3)) + 1)

        text_box = MultiListbox(root, (('State', 8), ('Term', 8), ('Voted For', 8), ('Array', 20), ('Time', 20)))

        text_box.grid(row = 6 + 2 * math.floor(i/3), column = 2*(i - 3*math.floor(i/3)) + 1)

        text_boxes.append(text_box)

    main()
        
