# python3 tkinter application for communicating with ESP8266 network

from tkinter import *
import tkinter.font as tkFont
import socket
import threading
import math
import time

class MultiListbox(Frame):
    def __init__(self, master, lists):
        Frame.__init__(self, master)
        self.lists = []
        for l,w in lists:
            frame = Frame(self); frame.pack(side=LEFT, expand=YES, fill=BOTH)
            Label(frame, text=l, borderwidth=1, relief=RAISED).pack(fill=X)
            lb = Listbox(frame, width=w, height=16, borderwidth=0, selectborderwidth=0,
                         relief=FLAT, exportselection=FALSE)
            lb.pack(expand=YES, fill=BOTH)
            self.lists.append(lb)
            lb.bind('<B1-Motion>', lambda e, s=self: s._select(e.y))
            lb.bind('<Button-1>', lambda e, s=self: s._select(e.y))
            lb.bind('<Leave>', lambda e: 'break')
            lb.bind('<B2-Motion>', lambda e, s=self: s._b2motion(e.x, e.y))
            lb.bind('<Button-2>', lambda e, s=self: s._button2(e.x, e.y))
        frame = Frame(self); frame.pack(side=LEFT, fill=Y)
        Label(frame, borderwidth=1, relief=RAISED).pack(fill=X)
        sb = Scrollbar(frame, orient=VERTICAL, command=self._scroll)
        sb.pack(expand=YES, fill=Y)
        self.lists[0]['yscrollcommand']=sb.set

    def _select(self, y):
        row = self.lists[0].nearest(y)
        self.selection_clear(0, END)
        self.selection_set(row)
        return 'break'

    def _button2(self, x, y):
        for l in self.lists: l.scan_mark(x, y)
        return 'break'

    def _b2motion(self, x, y):
        for l in self.lists: l.scan_dragto(x, y)
        return 'break'

    def _scroll(self, *args):
        for l in self.lists:
            l.yview(*args)

    def curselection(self):
        return self.lists[0].curselection(  )

    def delete(self, first, last=None):
        for l in self.lists:
            l.delete(first, last)

    def get(self, first, last=None):
        result = []
        for l in self.lists:
            result.append(l.get(first,last))
        if last: return map(*[None] + result)
        return result

    def index(self, index):
        self.lists[0].index(index)

    def insert(self, index, *elements):
        for e in elements:
            i = 0
            for l in self.lists:
                l.insert(index, e[i])
                i = i + 1

    def size(self):
        return self.lists[0].size(  )

    def see(self, index):
        for l in self.lists:
            l.see(index)

    def selection_anchor(self, index):
        for l in self.lists:
            l.selection_anchor(index)

    def selection_clear(self, first, last=None):
        for l in self.lists:
            l.selection_clear(first, last)

    def selection_includes(self, index):
        return self.lists[0].selection_includes(index)

    def selection_set(self, first, last=None):
        for l in self.lists:
            l.selection_set(first, last)
    

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
            text_boxes[int(data.decode()[0])].insert(0, (split_decoded[1],split_decoded[2],split_decoded[3],split_decoded[4],str(round(time.time(), 2))))
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