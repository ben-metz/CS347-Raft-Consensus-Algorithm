from tkinter import *
import tkinter.font as tkFont

# MultiListBox tkinker widget adapted from https://code.activestate.com/recipes/52266-multilistbox-tkinter-widget/
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

    # Converts RGB to tkinter colour format
    def rgb_hack(self, rgb):
        return '#%02x%02x%02x' % rgb

    # Scroll command
    def _scroll(self, *args):
        for l in self.lists:
            l.yview(*args)

    # Returns a list containing data in specified range
    def get(self, first, last=None):
        result = []
        for l in self.lists:
            result.append(l.get(first, last))
        if last:
            return map(*[None] + result)
        return result

    # Inserts entry into lists
    def insert(self, index, *elements):
        for e in elements:
            i = 0
            for l in self.lists:
                l.insert(index, e[i])
                i = i + 1

    # Clears all lists
    def clear(self):
        for l in self.lists:
            l.delete(0, END)
