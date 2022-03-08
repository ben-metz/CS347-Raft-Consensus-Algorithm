from tkinter import *
import tkinter.font as tkFont

class MultiListbox(Frame):
    def __init__(self, master, lists):
        Frame.__init__(self, master)
        self.lists = []
        for (l, w) in lists:
            frame = Frame(self)
            frame.pack(side=LEFT, expand=YES, fill=BOTH)
            Label(
                frame,
                text=l,
                borderwidth=0,
                relief=FLAT,
                fg='white',
                bg=self.rgb_hack((50, 50, 75)),
                anchor='w',
                ).pack(fill=X)
            lb = Listbox(
                frame,
                width=w,
                height=10,
                borderwidth=0,
                selectborderwidth=0,
                relief=FLAT,
                exportselection=FALSE,
                bg=self.rgb_hack((25, 25, 40)),
                fg='#DDDDFF',
                highlightthickness=0,
                )
            lb.pack(expand=YES, fill=BOTH)
            self.lists.append(lb)
            lb.bind('<B1-Motion>', lambda e, s=self: s._select(e.y))
            lb.bind('<Button-1>', lambda e, s=self: s._select(e.y))
            lb.bind('<Leave>', lambda e: 'break')
            lb.bind('<B2-Motion>', lambda e, s=self: s._b2motion(e.x,
                    e.y))
            lb.bind('<Button-2>', lambda e, s=self: s._button2(e.x,
                    e.y))
        
        frame = Frame(self)
        frame.pack(side=LEFT, fill=Y)
        Label(frame, borderwidth=0, relief=FLAT, bg=self.rgb_hack((50,
              50, 75))).pack(fill=X)
        sb = Scrollbar(
            frame,
            orient=VERTICAL,
            command=self._scroll,
            highlightthickness=0,
            borderwidth=0,
            bg=self.rgb_hack((50, 50, 75)),
            troughcolor=self.rgb_hack((25, 25, 40)),
            )
        sb.pack(expand=YES, fill=Y)
        self.lists[0]['yscrollcommand'] = sb.set

    def rgb_hack(self, rgb):
        return '#%02x%02x%02x' % rgb

    def _select(self, y):
        row = self.lists[0].nearest(y)
        self.selection_clear(0, END)
        self.selection_set(row)
        return 'break'

    def _button2(self, x, y):
        for l in self.lists:
            l.scan_mark(x, y)
        return 'break'

    def _b2motion(self, x, y):
        for l in self.lists:
            l.scan_dragto(x, y)
        return 'break'

    def _scroll(self, *args):
        for l in self.lists:
            l.yview(*args)

    def curselection(self):
        return self.lists[0].curselection()

    def delete(self, first, last=None):
        for l in self.lists:
            l.delete(first, last)

    def get(self, first, last=None):
        result = []
        for l in self.lists:
            result.append(l.get(first, last))
        if last:
            return map(*[None] + result)
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
        return self.lists[0].size()

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
