from multilistbox import MultiListbox
import math
from tkinter import *
import json

# Class that displays details for each server
class Interface:
    def __init__(self, frame, textCol, bgCol, font, index, socket_details):
        self.index = index
        self.socket_details = socket_details

        self.last_message = ""

        label = Label(frame, text='Server ' + str(index),
                      font=font)

        label.configure(foreground=textCol, background=bgCol)
        label.grid(row=3 * math.floor(index / 2), column=3 * (index - 2
                   * math.floor(index / 2)) + 1)

        self.kill_button = Button(frame, text='Start', width=5, bg='red', fg='black', command=self.kill,
                                  borderwidth=0, highlightthickness=0)
        self.kill_button.grid(row=3 * math.floor(index / 2), column=3 * (index - 2
                                                                         * math.floor(index / 2)) + 1, sticky=E+S)

        self.text_box = MultiListbox(frame, (('State', 10), ('Term',
                                                             5), ('Vote', 5), ('Action', 25), ('Array', 15),
                                             ('Commit', 7), ('Time', 7)))
        self.text_box.grid(row=1 + 3 * math.floor(index / 2), column=3 * (index - 2
                                                                          * math.floor(index / 2)) + 1)

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

    # Sets the button to indicate connected
    def connected(self):
        self.kill_button["text"] = "Stop"
        self.kill_button["bg"] = "green"

    # Sets the buttons to indicate not connected
    def disconnected(self):
        self.kill_button["text"] = "Start"
        self.kill_button["bg"] = "red"

    # Inserts data into multitextbox
    def insert(self, data):
        # if (data[3] == self.last_message):
        #     #self.text_box.remove_last()
        #     self.text_box.set_list(6, data[6])
        # else:
        #     self.last_message = data[3]
        
        self.text_box.insert(0, data)
        #self.text_box.remove_last()

    def clear(self):
        self.text_box.clear()
        self.last_message = ""

    # Sends status change message to server running on C++ program
    def set_stopped(self, new_status):
        update_msg = {"message_type": "set_server_status", "data": {
            "server_id": self.index, "stopped": new_status}}

        print("Sending: ", update_msg, "to", self.index)

        # Send to Manager
        self.socket_details[0].sendto(bytes(json.dumps(update_msg), 'utf-8'),
                                      (self.socket_details[1], self.socket_details[2]))
