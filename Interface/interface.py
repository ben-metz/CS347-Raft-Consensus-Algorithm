from multilistbox import MultiListbox
import math
from tkinter import *
import json

# Class that displays details for each server
class Interface:
    # Initialise tkinter components of interface
    def __init__(self, frame, textCol, bgCol, font, index, socket_details):
        self.index = index
        self.socket_details = socket_details

        self.interface_frame =  Frame(frame, bg=bgCol)
        
        self.interface_frame.pack(side=LEFT)

        self.show_repeated_messages = False

        self.last_message = ""

        self.label_text = StringVar()
        self.label_text.set('Server %d (Current Time: %.2f)' % (index, 0.0))
        self.label = Label(self.interface_frame, textvariable=self.label_text,
                      font=font)

        self.label.configure(foreground=textCol, background=bgCol)
        self.label.grid(row=0, column=0)

        self.kill_button = Button(self.interface_frame, text='Start', width=5, 
            bg='red', fg='black', command=self.kill,
            borderwidth=0, highlightthickness=0)

        self.kill_button.grid(row=0, column=0, sticky=E+S, padx='25')

        self.repeat_button = Button(self.interface_frame, text='Show', 
            width=10, bg='orange', fg='black', command=self.toggle_repeated_messages,
            borderwidth=0, highlightthickness=0)
        
        self.update_repeat_text()
        self.repeat_button.grid(row=0, column=0, sticky=W+S, padx='25')

        # Multilist box that stores the entries
        self.text_box = MultiListbox(self.interface_frame, (('State', 10), ('Term',
            5), ('Vote', 5), ('Action', 25), ('Array', 15), ('Commit', 7), ('Time', 7)))
        self.text_box.grid(row=1, column=0, padx='25', pady='10')

    # Disables comms of server if running, else enables comms
    def kill(self):
        if self.kill_button["text"] == "Start":
            self.kill_button["text"] = "Stop"
            self.kill_button["bg"] = "green"

            self.set_stopped(0)
        else:
            self.kill_button["text"] = "Start"
            self.kill_button["bg"] = "red"

            self.set_stopped(1)

    def update_repeat_text(self):
        self.repeat_button['text'] ='Hide Repeated' if self.show_repeated_messages else 'Show Repeated'

    def toggle_repeated_messages(self):
        self.show_repeated_messages = not self.show_repeated_messages
        self.update_repeat_text()

    # Updates the time/title of the server
    def update_time(self, time: str):
        self.label_text.set('Server %d (Current Time: %s)' % (self.index, time))

    # Sets the button to indicate connected
    def connected(self):
        self.kill_button["text"] = "Stop"
        self.kill_button["bg"] = "green"

    # Sets the buttons to indicate not connected
    def disconnected(self):
        self.kill_button["text"] = "Start"
        self.kill_button["bg"] = "red"

    # Compares data for removing duplicate entries
    def _compare_data(self, data, other):
        for (index, (it, other_it)) in enumerate(zip(data, other)):
            if index == 6:
                return True
            if (it != other_it):
                return False

    # Inserts data into multitextbox
    def insert(self, data):
        last_data = self.text_box.get(0)
        self.update_time(data[6])
        if self._compare_data(data, last_data) and not self.show_repeated_messages:
            return
        
        self.text_box.insert(0, data)

    # Clears entries
    def clear(self):
        self.text_box.clear()
        self.last_message = ""

    # Sends status change message to server running on C++ program
    def set_stopped(self, new_status):
        update_msg = {"message_type": "set_server_status", "data": {
            "server_id": self.index, "stopped": new_status}}

        # Send to Manager
        self.socket_details[0].sendto(bytes(json.dumps(update_msg), 'utf-8'),
                                      (self.socket_details[1], self.socket_details[2]))
