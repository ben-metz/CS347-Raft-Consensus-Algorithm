from tkinter import *
import json


class Injector:
    def __init__(self, frame, textCol, bgCol, font, index, socket_details):
        self.frame = frame

        self.index = index

        self.interface_frame = Frame(frame, bg=bgCol)
        self.interface_frame.pack(side=LEFT, padx=50)

        self.input_frame = Frame(self.interface_frame, bg=bgCol)

        self.candidate = "Follower"

        self.label_text = StringVar()
        self.label_text.set('Server %d' % (index))
        self.label = Label(self.interface_frame, textvariable=self.label_text,
                           font=font)

        self.label.configure(foreground=textCol, background=bgCol)

        self.socket_details = socket_details

        self.new_state = StringVar(self.input_frame)
        self.new_state.set("Follower")  # default value

        state_selector_label = Label(
            self.input_frame, text='New State', font=font)
        state_selector_label.configure(foreground=textCol, background=bgCol)
        state_selector_label.grid(row=0, column=0)

        state_selector = OptionMenu(
            self.input_frame, self.new_state, "Leader", "Candidate", "Follower")
        state_selector.config(width=10, borderwidth=0, highlightthickness=0)
        state_selector.grid(row=1, column=0, padx=10)

        timeout_label = Label(self.input_frame, text='Timeout', font=font)
        timeout_label.configure(foreground=textCol, background=bgCol)
        timeout_label.grid(row=0, column=1)

        self.timeout = Entry(self.input_frame, borderwidth=0)
        self.timeout.grid(row=1, column=1, padx=10)

        term_label = Label(self.input_frame, text='Term', font=font)
        term_label.configure(foreground=textCol, background=bgCol)
        term_label.grid(row=0, column=2)

        self.term = Entry(self.input_frame, borderwidth=0)
        self.term.grid(row=1, column=2, padx=10)

        send = Button(self.interface_frame, text='Send', bg='green', command=self.send_test,
                      borderwidth=0, highlightthickness=0, width=15)

        self.label.pack(side=TOP)
        self.input_frame.pack(side=TOP)
        send.pack(side=BOTTOM, pady=15)

    # Sends the entered test state to the associated raft node
    def send_test(self):
        try:
            update_msg = {"message_type": "state_injection", "data": {
                "server_id": self.index, "new_state": self.new_state.get(),
                "election_timeout": int(self.timeout.get()),
                "term": int(self.term.get())}}

            print("Sending: ", update_msg, "to", self.index)

            # Send to Manager
            self.socket_details[0].sendto(bytes(json.dumps(update_msg), 'utf-8'),
                                          (self.socket_details[1], self.socket_details[2]))
        except:
            print("Missing/Invalid Entry...")
