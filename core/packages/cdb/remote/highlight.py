from gi.repository import Gio, GLib
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk
import dbus
from dbus.mainloop.glib import DBusGMainLoop
import pyatspi
import cairo
import sys
from threading import Thread
import time
import math
import textwrap

class WidgetHighlighter:
    def __init__(self, window_name):
        self.window = None
        self.window_name = window_name
        self.current_highlight = None
        self.subtitle_window = None
        self.subtitle_x = 0
        self.subtitle_y = 0
        self.subtitle_bottom = 20
        self.subtitle_large = False
        self.label = None
        self.loop = None
        self.target_window = self.find_window(self.window_name)

    def create_highlight_window(self, x, y, width, height):
        # from gi.repository import Gtk, Gdk
        
        if self.current_highlight:
            self.remove_highlight()
            
        win = Gtk.Window(type=Gtk.WindowType.POPUP)
        win.set_app_paintable(True)
        win.set_visual(win.get_screen().get_rgba_visual())
        print(x,y,width,height)
        win.resize(width + 4, height + 4)
        win.move(x - 2, y - 2)
        
        win.connect('draw', self.draw_highlight)
        win.show_all()
        self.current_highlight = win
        
    def draw_highlight(self, widget, cr):
        cr.set_operator(cairo.OPERATOR_SOURCE)
        cr.set_source_rgba(1.0, 0.6, 0.0, 0.3)  # Semi-transparent orange
        cr.paint()
        cr.set_source_rgba(1.0, 0.6, 0.0, 0.8)  # Solid orange border
        cr.set_line_width(2)
        cr.rectangle(1, 1, widget.get_allocated_width() - 2,
                    widget.get_allocated_height() - 2)
        cr.stroke()
        return False

    def remove_highlight(self):
        if self.current_highlight:
            self.current_highlight.destroy()
            self.current_highlight = None

    def find_button_by_label(self, app_name, button_label):
        desktop = pyatspi.Registry.getDesktop(0)
        for app in desktop:
            if app and app_name.lower() in app.name.lower():
                for window in app:
                    button = self._find_button_recursive(window, button_label)
                    if button:
                        return button
        return None
    
    def _find_button_recursive(self, obj, label):
        if not obj.getRole() == pyatspi.ROLE_MENU_ITEM:
            if label.lower() in obj.name.lower():
                return obj
                
        for child in obj:
            result = self._find_button_recursive(child, label)
            if result:
                return result
        return None
    
    def highlight_button(self, button):
        if not button:
            return
            
        component = button.queryComponent()
        print(component)
        x, y = component.getPosition(pyatspi.DESKTOP_COORDS)
        w, h = component.getSize()
        # print(x,y,w,h)
        
        # Create highlight overlay
        self.create_highlight_window(x, y, w, h)
        
        # Start GTK main loop in a separate thread
        if not self.loop:
            self.loop = GLib.MainLoop()
            thread = Thread(target=self._run_loop)
            thread.daemon = True
            thread.start()

    def _run_loop(self):
        self.loop.run()

    def find_window(self, name):
        desktop = pyatspi.Registry.getDesktop(0)
        self.target_window = None
        for app in desktop:
            if app:
                for window in app:
                    if self.window_name.lower() in window.name.lower():
                        return window

        raise Exception("Window "+self.window_name+" not found")
        
    def subtitle(self, text, large=False):
        """Display a subtitle at the bottom of the specified window.
        If window_name is None, places it at bottom of screen."""
        if self.subtitle_window:
            self.remove_subtitle()
        self.subtitle_large = large
            
        # Create subtitle window
        win = Gtk.Window(type=Gtk.WindowType.POPUP)
        self.subtitle_window = win
        win.set_app_paintable(True)
        win.set_visual(win.get_screen().get_rgba_visual())
        win.set_type_hint(Gdk.WindowTypeHint.POPUP_MENU)
        win.set_name("subtitle-window")  # Give the window a name for CSS
        
        # Create label with text
        self.label = Gtk.Label()
        self.label.set_text("\n".join(textwrap.wrap(text)))
        self.label.set_name("subtitle")
        
        # Style the label
        css_provider = Gtk.CssProvider()
        if large:
            css_provider.load_from_data(b"""
                #subtitle-window {
                    border-radius: 5px;
                }
                #subtitle {
                    padding: 10px 40px;
                    font-size: 90px;
                    color: black;
                }
            """)
        else:
            css_provider.load_from_data(b"""
                #subtitle-window {
                    border-radius: 5px;
                }
                #subtitle {
                    padding: 10px 20px;
                    font-size: 45px;
                    color: black;
                }
            """)

        context = self.label.get_style_context()
        context.add_provider(css_provider, 
                           Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION)

        win.add(self.label)
        win.connect('draw', self.draw_subtitle)
        win.show_all()
        self.startloop()

    def startloop(self):
        # Start GTK main loop if not already running
        if not self.loop:
            self.loop = GLib.MainLoop()
            thread = Thread(target=self._run_loop)
            thread.daemon = True
            thread.start()

    def draw_subtitle(self, widget, cr):
        # Size and position the window
        monitor = Gdk.Display.get_default().get_primary_monitor()
        geometry = monitor.get_geometry()

        # Get natural size of label
        label_width, label_height = self.label.get_preferred_size()[1].width, \
            self.label.get_preferred_size()[1].height
        # print("LABEL:", label_width, label_height)
        
        component = self.target_window.queryComponent()
        wx, wy = component.getPosition(pyatspi.WINDOW_COORDS)
        ww, wh = component.getSize()
        self.subtitle_x = wx + (ww - label_width) // 2 
        self.subtitle_y = wy + wh - label_height - self.subtitle_bottom
        # print(wx, wy, ww, wh, label_width, label_height, self.subtitle_x, self.subtitle_y)
        
        self.subtitle_window.move(self.subtitle_x, self.subtitle_y)
        self.draw_subtitle_background(widget, cr) #connect('draw', self.draw_subtitle_background)
        
    
    def draw_subtitle_background(self, widget, cr):
        cr.set_operator(cairo.OPERATOR_SOURCE)
        gl=0.9
        cr.set_source_rgba(gl, gl, gl, 0.6)  # 0=transparent, 1=opaque
        
        # Get the window dimensions
        width = widget.get_allocated_width()
        height = widget.get_allocated_height()
        
        # Set the radius for rounded corners
        radius = 12
        
        # Create rounded rectangle path
        cr.new_path()
        
        # Top left corner
        cr.arc(radius, radius, radius, 180 * (math.pi/180), 270 * (math.pi/180))
        # Top right corner
        cr.arc(width - radius, radius, radius, -90 * (math.pi/180), 0)
        # Bottom right corner
        cr.arc(width - radius, height - radius, radius, 0, 90 * (math.pi/180))
        # Bottom left corner
        cr.arc(radius, height - radius, radius, 90 * (math.pi/180), 180 * (math.pi/180))
        cr.close_path()
        cr.fill()

        if self.subtitle_large:
            radius = 18
            # Marker left #209020;
            cr.set_source_rgba(0.125, 0.565, 0.125, 1)  # 0=transparent, 1=opaque
            cr.new_path()
            # Top left corner
            cr.arc(radius, radius, radius, 180 * (math.pi/180), 270 * (math.pi/180))
            # Bottom left corner
            cr.arc(radius, height - radius, radius, 90 * (math.pi/180), 180 * (math.pi/180))
            cr.close_path()
            cr.fill()
        
        return False
    
    def remove_subtitle(self):
        """Remove the subtitle window if it exists"""
        if self.subtitle_window:
            self.subtitle_window.destroy()
            self.subtitle_window = None

    def remove_all(self):
        """Remove both highlight and subtitle windows"""
        self.remove_highlight()
        self.remove_subtitle()

highlighter = None

def init_highlight(title="Cadabra"):
    global highlighter
    if highlighter == None:
        highlighter = WidgetHighlighter(title)
    

def mark(label=""):
    global highlighter

    if label=="":
        highlighter.remove_highlight()
    else:
        obj = highlighter.find_button_by_label("cadabra2-gtk", label)
        if obj:
            print("FOUND", obj)
            highlighter.highlight_button(obj)

def subtitle(txt="", large=False, bottom=20):
    global highlighter

    highlighter.subtitle_bottom = bottom
    if txt=="":
        highlighter.remove_subtitle()
    else:
        highlighter.subtitle(txt, large)

DBusGMainLoop(set_as_default=True)
        
if __name__ == "__main__":
    main()
