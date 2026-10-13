from gi.repository import Gio, GLib
import gi
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk, Gdk
import dbus
from dbus.mainloop.glib import DBusGMainLoop
import pyatspi
import cairo
from Xlib import display as xdisplay, X
import sys
import os
import faulthandler
import threading
from threading import Thread
import time
import math
import textwrap

# Print the Python stack of all threads if we ever segfault.
faulthandler.enable()

# Distance in pixels between the bottom of the subtitle and the bottom
# of the notebook window; large enough to clear the status bar.
subtitle_bottom = 100

# Print the numbers which go into positioning a highlight.
debug = os.environ.get("CADABRA_HIGHLIGHT_DEBUG", "") not in ("", "0")

# All GTK, AT-SPI and X work happens on a single thread running the
# GLib main loop; none of those libraries are thread-safe. The public
# functions below hand their work to that thread with _run_on_loop.
_loop = None
_loop_thread = None

def _start_loop():
    global _loop, _loop_thread
    if _loop_thread is None:
        _loop = GLib.MainLoop()
        _loop_thread = Thread(target=_loop.run, daemon=True)
        _loop_thread.start()

def _run_on_loop(fn, *args):
    """Run fn(*args) on the GLib loop thread, wait for it to finish,
    and return its result (or re-raise its exception)."""
    _start_loop()
    if threading.current_thread() is _loop_thread:
        return fn(*args)
    done = threading.Event()
    result = {}
    def task():
        try:
            result["value"] = fn(*args)
        except BaseException as ex:
            result["error"] = ex
        done.set()
        return False # run once
    GLib.idle_add(task)
    done.wait()
    if "error" in result:
        raise result["error"]
    return result.get("value")

class WidgetHighlighter:
    def __init__(self, window_name):
        self.window = None
        self.window_name = window_name
        self.current_highlight = None
        self.subtitle_window = None
        self.subtitle_x = 0
        self.subtitle_y = 0
        self.subtitle_target = (0, 0, 0, 0)
        self.subtitle_bottom = subtitle_bottom
        self.subtitle_large = False
        self.label = None
        self.xdisplay = None
        self.xwindow = None
        self.target_window = self.find_window(self.window_name)

    def create_highlight_window(self, x, y, width, height):
        # from gi.repository import Gtk, Gdk
        
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
        """Find a widget by its accessible name. An exact (case-insensitive)
        match wins; otherwise the first widget whose name contains the
        label is returned."""
        label = button_label.lower()
        partial = []
        desktop = pyatspi.Registry.getDesktop(0)
        for app in desktop:
            if app and app_name.lower() in (app.name or "").lower():
                for window in app:
                    button = self._find_button_recursive(window, label, partial)
                    if button:
                        return button
        return partial[0] if partial else None
    
    def _find_button_recursive(self, obj, label, partial):
        if not obj.getRole() == pyatspi.ROLE_MENU_ITEM:
            name = (obj.name or "").lower()
            if name == label:
                return obj
            if label in name:
                partial.append(obj)
                
        for child in obj:
            result = self._find_button_recursive(child, label, partial)
            if result:
                return result
        return None

    def window_origin(self):
        """Screen position of the target window's content. GTK4 does not
        report screen coordinates over AT-SPI (it returns 0,0), only
        coordinates relative to the window, so get the window position
        from the X server instead."""
        x, y, w, h = self.window_rect()
        return x, y

    def window_rect(self):
        """Screen position and size (x, y, w, h) of the target window's
        content, excluding any client-side shadow."""
        if self.xdisplay is None:
            self.xdisplay = xdisplay.Display()
        d = self.xdisplay
        root = d.screen().root
        if self.xwindow is not None:
            try:
                self.xwindow.get_geometry()
            except Exception:
                self.xwindow = None
        if self.xwindow is None:
            self.xwindow = self._find_xwindow(root, self.window_name.lower())
            if self.xwindow is None:
                raise Exception("X window "+self.window_name+" not found")
        pos = root.translate_coords(self.xwindow, 0, 0)
        geom = self.xwindow.get_geometry()
        x, y, w, h = pos.x, pos.y, geom.width, geom.height
        # GTK windows with client-side decorations include their shadow
        # in the X window; widget coordinates start inside it.
        extents = self.xwindow.get_full_property(d.intern_atom("_GTK_FRAME_EXTENTS"),
                                                 X.AnyPropertyType)
        if extents and len(extents.value) == 4:
            left, right, top, bottom = extents.value
            x += left
            y += top
            w -= left + right
            h -= top + bottom
        return x, y, w, h

    def _find_xwindow(self, win, name):
        d = self.xdisplay
        try:
            prop = win.get_full_property(d.intern_atom("_NET_WM_NAME"),
                                         d.intern_atom("UTF8_STRING"))
            title = prop.value if prop else win.get_wm_name()
            if isinstance(title, bytes):
                title = title.decode("utf-8", "replace")
            if title and name in title.lower() \
               and win.get_attributes().map_state == X.IsViewable:
                return win
            children = win.query_tree().children
        except Exception:
            return None # window went away while we were looking
        for child in children:
            found = self._find_xwindow(child, name)
            if found is not None:
                return found
        return None
    
    def highlight_button(self, button):
        if not button:
            return
            
        component = button.queryComponent()
        bx, by = component.getPosition(pyatspi.WINDOW_COORDS)
        wx, wy = self.window_origin()
        x, y = wx + bx, wy + by
        w, h = component.getSize()
        if debug:
            self.print_debug(button, bx, by, w, h, x, y)
        
        # Create highlight overlay
        self.create_highlight_window(x, y, w, h)

    def print_debug(self, button, bx, by, w, h, x, y):
        print(f"highlight '{button.name}': atspi window-coords pos=({bx},{by}) size=({w},{h})")
        try:
            top = self.target_window.queryComponent()
            print(f"  atspi toplevel '{self.target_window.name}': "
                  f"window-coords extents={tuple(top.getExtents(pyatspi.WINDOW_COORDS))} "
                  f"size={tuple(top.getSize())}")
        except Exception as ex:
            print(f"  atspi toplevel: {ex}")
        d = self.xdisplay
        root = d.screen().root
        pos = root.translate_coords(self.xwindow, 0, 0)
        geom = self.xwindow.get_geometry()
        ext = self.xwindow.get_full_property(d.intern_atom("_GTK_FRAME_EXTENTS"),
                                             X.AnyPropertyType)
        print(f"  X window 0x{self.xwindow.id:x}: root origin=({pos.x},{pos.y}) "
              f"size=({geom.width},{geom.height}) border={geom.border_width}")
        print(f"  _GTK_FRAME_EXTENTS (l,r,t,b)={tuple(ext.value) if ext else None}")
        print(f"  screen box=({x},{y},{w},{h})")

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
        # Look up where the target window is once; the subtitle is then
        # (re)positioned whenever GTK settles its size.
        self.subtitle_target = self.window_rect()
        self.subtitle_x = self.subtitle_y = None # new window: always move
        win.connect('size-allocate', self.position_subtitle)
        win.connect('draw', self.draw_subtitle_background)
        win.show_all()

    def position_subtitle(self, widget, allocation):
        """Centre the subtitle window near the bottom of the target
        window, using the subtitle's actual allocated size."""
        wx, wy, ww, wh = self.subtitle_target
        x = wx + (ww - allocation.width) // 2
        y = wy + wh - allocation.height - self.subtitle_bottom
        if debug:
            print(f"subtitle: target window=({wx},{wy},{ww},{wh}) "
                  f"subtitle size=({allocation.width},{allocation.height}) pos=({x},{y})")
        if (x, y) != (self.subtitle_x, self.subtitle_y):
            self.subtitle_x, self.subtitle_y = x, y
            widget.move(x, y)
        
    
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
        highlighter = _run_on_loop(WidgetHighlighter, title)
    

def _mark(label):
    if label=="":
        highlighter.remove_highlight()
    else:
        obj = highlighter.find_button_by_label("cadabra2-gtk", label)
        if obj:
            highlighter.highlight_button(obj)
        else:
            print(f"mark: no widget named '{label}' found")

def mark(label=""):
    _run_on_loop(_mark, label)

def _subtitle(txt, large, bottom):
    highlighter.subtitle_bottom = subtitle_bottom if bottom is None else bottom
    if txt=="":
        highlighter.remove_subtitle()
    else:
        highlighter.subtitle(txt, large)

def subtitle(txt="", large=False, bottom=None):
    """Show a subtitle, or remove it if `txt` is empty. `bottom` is the
    distance from the bottom of the window; None means the module-wide
    default `subtitle_bottom`."""
    _run_on_loop(_subtitle, txt, large, bottom)

DBusGMainLoop(set_as_default=True)
        
if __name__ == "__main__":
    main()
