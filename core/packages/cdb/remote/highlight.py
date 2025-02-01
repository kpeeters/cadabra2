from gi.repository import Gio, GLib
import gi
gi.require_version('Gtk', '3.0')
import dbus
from dbus.mainloop.glib import DBusGMainLoop
import pyatspi
import cairo
import sys
from threading import Thread
import time

class WidgetHighlighter:
    def __init__(self):
        self.window = None
        self.current_highlight = None
        self.loop = None
        
    def create_highlight_window(self, x, y, width, height):
        from gi.repository import Gtk, Gdk
        
        if self.current_highlight:
            self.remove_highlight()
            
        win = Gtk.Window(type=Gtk.WindowType.POPUP)
        win.set_app_paintable(True)
        win.set_visual(win.get_screen().get_rgba_visual())
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
        x, y = component.getPosition(pyatspi.DESKTOP_COORDS)
        w, h = component.getSize()
        
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

highlighter = WidgetHighlighter()

def mark(label=""):
    if label=="":
        highlighter.remove_highlight()
    else:
        obj = highlighter.find_button_by_label("cadabra2-gtk", label)
        if obj:
            highlighter.highlight_button(obj)
        
def main():
    DBusGMainLoop(set_as_default=True)
    highlighter = WidgetHighlighter()
    
    app_name = "cadabra2-gtk"
    button_label = "Run"
    
    # Find and highlight the button
    button = highlighter.find_button_by_label(app_name, button_label)
    if button:
        highlighter.highlight_button(button)
        print(f"Button found and highlighted")
        
        # Example: Do something else while highlighted
        time.sleep(2)
        print("Doing something else...")
        time.sleep(2)
        
        # Remove highlight when done
        highlighter.remove_highlight()
        print("Highlight removed")
    else:
        print(f"Button '{button_label}' not found in {app_name}")

DBusGMainLoop(set_as_default=True)
        
if __name__ == "__main__":
    main()
