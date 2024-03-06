# Copyright (c) 2020 - 2021 Qualcomm Technologies International, Ltd.

import traceback

try:
    from tkinter import *
    import tkinter.filedialog as filedialog
    import tkinter.messagebox as messagebox
    import tkinter.simpledialog as simpledialog
except ImportError:
    from Tkinter import *
    import tkFileDialog as filedialog
    import tkMessageBox as messagebox
    import tkSimpleDialog as simpledialog


__MDE_THEME = {
    'pady': 5,
    'padx': 5,
    'background': 'white',
}

__selected_theme = __MDE_THEME


PACK_DEFAULTS = {
    'side': TOP,
    'anchor': W,
    'fill': BOTH,
    'expand': True
}


def set_theme(theme):
    global selected_theme
    selected_theme = theme


def __themed(cls):
    orig_init = cls.__init__

    def themed_init(self, *args, **kwargs):
        orig_init(self, *args, **kwargs)
        self.config(**__selected_theme)

    cls.__init__ = themed_init

    return cls


Root = __themed(Tk)()
Frame = __themed(Frame)
Label = __themed(Label)
Button = __themed(Button)
Checkbutton = __themed(Checkbutton)
Radiobutton = __themed(Radiobutton)
Text = __themed(Text)
tkFileDialog = __themed(filedialog)
tkMessageBox = __themed(messagebox)
tkSimpleDialog = __themed(simpledialog)


class ExceptionDialog(tkSimpleDialog.Dialog, object):
    def __init__(self, exception, title=None):
        formatted_excep = traceback.format_exception(*exception)
        self.exception = list()
        for each in formatted_excep:
            self.exception.extend(each.split('\n'))

        if title is None:
            title = "Unexpected Exception"

        super(ExceptionDialog, self).__init__(Root, title=title)

    def body(self, parent):
        self.config(background='white')

        width = 100
        height = len(self.exception)
        text = Text(self, height=height, width=width, borderwidth=0, relief=FLAT, padx=0)
        text.pack(side=LEFT, fill=BOTH, anchor=NW, expand=True)
        text.insert(END, '\n'.join(self.exception))
        text.config(state=DISABLED)

        # create a scrollbar widget and set its command to the text widget
        scrollbar = Scrollbar(self, background='white', orient='vertical', command=text.yview)
        scrollbar.pack(side=RIGHT, fill=Y)

        #  communicate back to the scrollbar
        text['yscrollcommand'] = scrollbar.set
        return text

    def buttonbox(self):
        pass


def show_error(self, exc_type, exc_value, exc_tb):
    try:
        exception = exc_value.exc_info
    except AttributeError:
        exception = (exc_type, exc_value, exc_tb)
    ExceptionDialog(exception)

Tk.report_callback_exception = show_error
