
#pragma once

/// \file
/// Compatibility helpers that let the gtkmm frontend build against both
/// gtkmm-3 and gtkmm-4 from a single source tree.
///
/// Everything lives in namespace `cdb` and is a thin inline wrapper, so that
/// when support for gtkmm-3 is eventually dropped these can be inlined back
/// into the call sites mechanically.
///
/// `USE_GTK4` is set as a compile definition by frontend/gtkmm/CMakeLists.txt;
/// it deliberately does *not* come from Config.hh, which is generated into the
/// source tree and shared between build directories.

#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/paned.h>
#include <gtkmm/window.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/stylecontext.h>
#include <gdkmm/display.h>
// display.h only forward-declares Monitor, and monitor_geometry() below calls
// through it.
#include <gdkmm/monitor.h>
#include <gdkmm/rectangle.h>
#ifdef USE_GTK4
// Likewise Gdk::Surface, which get_monitor_at_surface() takes.
#include <gdkmm/surface.h>
#endif
#include <gtkmm/textbuffer.h>
#include <gdkmm/pixbuf.h>
#ifdef USE_GTK4
#include <gdkmm/texture.h>
#endif
#ifdef USE_GTK4
#include <gtkmm/gestureclick.h>
#else
#include <gtkmm/gesturemultipress.h>
#endif
#include <gtkmm/widget.h>
#include <gtkmm/textview.h>
// Gdk::ModifierType moved between headers: gdkmm/types.h in gdkmm-3,
// gdkmm/enums.h in gdkmm-4.
#ifdef USE_GTK4
#include <gdkmm/enums.h>
#else
#include <gdkmm/types.h>
#endif
#include <giomm/application.h>
#include <pangomm/attributes.h>
#include <pangomm/tabarray.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>
#include <glibmm/value.h>

#include <type_traits>

namespace cdb {

	/// Unscoped gtkmm-3 enumerators became scoped enum classes in gtkmm-4, so
	/// the spellings differ even though the values do not. Declared once here
	/// and used unqualified at the call sites.

#ifdef USE_GTK4
	constexpr auto APP_HANDLES_OPEN         = Gio::Application::Flags::HANDLES_OPEN;
	constexpr auto APP_HANDLES_COMMAND_LINE = Gio::Application::Flags::HANDLES_COMMAND_LINE;
	constexpr auto APP_NON_UNIQUE           = Gio::Application::Flags::NON_UNIQUE;

	constexpr auto OPTION_TYPE_BOOL   = Gio::Application::OptionType::BOOL;
	constexpr auto OPTION_TYPE_INT    = Gio::Application::OptionType::INT;
	constexpr auto OPTION_TYPE_STRING = Gio::Application::OptionType::STRING;
#else
	constexpr auto APP_HANDLES_OPEN         = Gio::APPLICATION_HANDLES_OPEN;
	constexpr auto APP_HANDLES_COMMAND_LINE = Gio::APPLICATION_HANDLES_COMMAND_LINE;
	constexpr auto APP_NON_UNIQUE           = Gio::APPLICATION_NON_UNIQUE;

	constexpr auto OPTION_TYPE_BOOL   = Gio::Application::OptionType::OPTION_TYPE_BOOL;
	constexpr auto OPTION_TYPE_INT    = Gio::Application::OptionType::OPTION_TYPE_INT;
	constexpr auto OPTION_TYPE_STRING = Gio::Application::OptionType::OPTION_TYPE_STRING;
#endif

#ifdef USE_GTK4
	constexpr auto ORIENT_HORIZONTAL = Gtk::Orientation::HORIZONTAL;
	constexpr auto ORIENT_VERTICAL   = Gtk::Orientation::VERTICAL;

	constexpr auto ALIGN_FILL   = Gtk::Align::FILL;
	constexpr auto ALIGN_START  = Gtk::Align::START;
	constexpr auto ALIGN_END    = Gtk::Align::END;
	constexpr auto ALIGN_CENTER = Gtk::Align::CENTER;

	constexpr auto POLICY_ALWAYS    = Gtk::PolicyType::ALWAYS;
	constexpr auto POLICY_AUTOMATIC = Gtk::PolicyType::AUTOMATIC;
	constexpr auto POLICY_NEVER     = Gtk::PolicyType::NEVER;

	constexpr auto SIZE_REQUEST_HEIGHT_FOR_WIDTH = Gtk::SizeRequestMode::HEIGHT_FOR_WIDTH;
	constexpr auto SIZE_REQUEST_WIDTH_FOR_HEIGHT = Gtk::SizeRequestMode::WIDTH_FOR_HEIGHT;

	constexpr auto TEXT_WINDOW_TEXT       = Gtk::TextWindowType::TEXT;
	constexpr auto WRAP_WORD_CHAR         = Gtk::WrapMode::WORD_CHAR;
	constexpr auto WRAP_NONE              = Gtk::WrapMode::NONE;
	constexpr auto COLORSPACE_RGB         = Gdk::Colorspace::RGB;

	constexpr auto STATE_FLAG_NORMAL   = Gtk::StateFlags::NORMAL;
	constexpr auto STATE_FLAG_SELECTED = Gtk::StateFlags::SELECTED;

	constexpr auto RESPONSE_OK     = Gtk::ResponseType::OK;
	constexpr auto RESPONSE_CANCEL = Gtk::ResponseType::CANCEL;
	constexpr auto RESPONSE_CLOSE  = Gtk::ResponseType::CLOSE;
	constexpr auto RESPONSE_YES    = Gtk::ResponseType::YES;
	constexpr auto RESPONSE_APPLY  = Gtk::ResponseType::APPLY;

	constexpr auto MESSAGE_INFO     = Gtk::MessageType::INFO;
	constexpr auto MESSAGE_WARNING  = Gtk::MessageType::WARNING;
	constexpr auto MESSAGE_QUESTION = Gtk::MessageType::QUESTION;
	constexpr auto MESSAGE_ERROR    = Gtk::MessageType::ERROR;

	constexpr auto BUTTONS_NONE   = Gtk::ButtonsType::NONE;
	constexpr auto BUTTONS_OK     = Gtk::ButtonsType::OK;
	constexpr auto BUTTONS_YES_NO = Gtk::ButtonsType::YES_NO;
	// pangomm-2.48 goes with gtkmm-4, pangomm-1.4 with gtkmm-3.
	constexpr auto PANGO_TAB_LEFT         = Pango::TabAlign::LEFT;
	constexpr auto PANGO_UNDERLINE_ERROR  = Pango::Underline::ERROR;
#else
	constexpr auto ORIENT_HORIZONTAL = Gtk::ORIENTATION_HORIZONTAL;
	constexpr auto ORIENT_VERTICAL   = Gtk::ORIENTATION_VERTICAL;

	constexpr auto ALIGN_FILL   = Gtk::ALIGN_FILL;
	constexpr auto ALIGN_START  = Gtk::ALIGN_START;
	constexpr auto ALIGN_END    = Gtk::ALIGN_END;
	constexpr auto ALIGN_CENTER = Gtk::ALIGN_CENTER;

	constexpr auto POLICY_ALWAYS    = Gtk::POLICY_ALWAYS;
	constexpr auto POLICY_AUTOMATIC = Gtk::POLICY_AUTOMATIC;
	constexpr auto POLICY_NEVER     = Gtk::POLICY_NEVER;

	constexpr auto SIZE_REQUEST_HEIGHT_FOR_WIDTH = Gtk::SizeRequestMode::SIZE_REQUEST_HEIGHT_FOR_WIDTH;
	constexpr auto SIZE_REQUEST_WIDTH_FOR_HEIGHT = Gtk::SizeRequestMode::SIZE_REQUEST_WIDTH_FOR_HEIGHT;

	constexpr auto TEXT_WINDOW_TEXT       = Gtk::TEXT_WINDOW_TEXT;
	constexpr auto WRAP_WORD_CHAR         = Gtk::WRAP_WORD_CHAR;
	constexpr auto WRAP_NONE              = Gtk::WRAP_NONE;
	constexpr auto COLORSPACE_RGB         = Gdk::COLORSPACE_RGB;

	constexpr auto STATE_FLAG_NORMAL   = Gtk::STATE_FLAG_NORMAL;
	constexpr auto STATE_FLAG_SELECTED = Gtk::STATE_FLAG_SELECTED;

	constexpr auto RESPONSE_OK     = Gtk::RESPONSE_OK;
	constexpr auto RESPONSE_CANCEL = Gtk::RESPONSE_CANCEL;
	constexpr auto RESPONSE_CLOSE  = Gtk::RESPONSE_CLOSE;
	constexpr auto RESPONSE_YES    = Gtk::RESPONSE_YES;
	constexpr auto RESPONSE_APPLY  = Gtk::RESPONSE_APPLY;

	constexpr auto MESSAGE_INFO     = Gtk::MESSAGE_INFO;
	constexpr auto MESSAGE_WARNING  = Gtk::MESSAGE_WARNING;
	constexpr auto MESSAGE_QUESTION = Gtk::MESSAGE_QUESTION;
	constexpr auto MESSAGE_ERROR    = Gtk::MESSAGE_ERROR;

	constexpr auto BUTTONS_NONE   = Gtk::BUTTONS_NONE;
	constexpr auto BUTTONS_OK     = Gtk::BUTTONS_OK;
	constexpr auto BUTTONS_YES_NO = Gtk::BUTTONS_YES_NO;
	constexpr auto PANGO_TAB_LEFT         = Pango::TAB_LEFT;
	constexpr auto PANGO_UNDERLINE_ERROR  = Pango::Underline::UNDERLINE_ERROR;
#endif

	/// GTK4 replaced Gtk::TextBuffer::insert_pixbuf() with insert_paintable();
	/// a Gdk::Pixbuf has to be wrapped in a Gdk::Texture to become one.

	inline Gtk::TextBuffer::iterator insert_image(const Glib::RefPtr<Gtk::TextBuffer>& buffer,
																 const Gtk::TextBuffer::iterator& pos,
																 const Glib::RefPtr<Gdk::Pixbuf>& pixbuf)
		{
#ifdef USE_GTK4
		return buffer->insert_paintable(pos, Gdk::Texture::create_for_pixbuf(pixbuf));
#else
		return buffer->insert_pixbuf(pos, pixbuf);
#endif
		}

	/// GTK4 dissolved pack_start's three arguments into properties on the child:
	/// `expand` became set_h/vexpand, `fill` became the h/valign (FILL is the
	/// default, and no live call site used PACK_EXPAND_PADDING), and per-child
	/// `padding` became margins.
	///
	/// These helpers speak GTK4 vocabulary and let the gtkmm-3 branch reproduce
	/// it. That branch sets both the pack option *and* the expand property, so
	/// it does not depend on whether gtkmm-3's GtkBox folds
	/// gtk_widget_compute_expand() into its own allocation.
	///
	/// The gtkmm-3 branch also shows the child, normalising gtkmm-3's "widgets
	/// start hidden" to GTK4's "widgets start visible". Anything that should
	/// stay hidden therefore needs an explicit set_visible(false) -- which GTK4
	/// would require anyway.

	enum class Expand { no, yes };

	inline void box_append(Gtk::Box& box, Gtk::Widget& child,
	                       Expand expand = Expand::no, int padding = 0)
		{
		const bool vertical = (box.get_orientation() == ORIENT_VERTICAL);

		if(expand == Expand::yes) {
			if(vertical) child.set_vexpand(true);
			else         child.set_hexpand(true);
			}
		if(padding > 0) {
			if(vertical) {
				child.set_margin_top(padding);
				child.set_margin_bottom(padding);
				}
			else {
				child.set_margin_start(padding);
				child.set_margin_end(padding);
				}
			}
#ifdef USE_GTK4
		box.append(child);
#else
		box.pack_start(child,
		               expand==Expand::yes ? Gtk::PACK_EXPAND_WIDGET : Gtk::PACK_SHRINK,
		               0);
		child.show();
#endif
		}

	/// Gdk::ModifierType became a scoped enum in GTK4, so both the enumerator
	/// spellings and the way a mask is tested differ.

#ifdef USE_GTK4
	constexpr auto MOD_SHIFT   = Gdk::ModifierType::SHIFT_MASK;
	constexpr auto MOD_CONTROL = Gdk::ModifierType::CONTROL_MASK;
#else
	constexpr auto MOD_SHIFT   = Gdk::SHIFT_MASK;
	constexpr auto MOD_CONTROL = Gdk::CONTROL_MASK;
#endif

	inline bool has_modifier(Gdk::ModifierType state, Gdk::ModifierType modifier)
		{
		return (state & modifier) == modifier;
		}

	/// Gtk::GestureMultiPress was renamed Gtk::GestureClick in GTK4, and the way
	/// a controller is attached to a widget changed: gtkmm-3 passes the widget
	/// to create(), GTK4 uses Gtk::Widget::add_controller().

#ifdef USE_GTK4
	using ClickGesture = Gtk::GestureClick;
	constexpr auto SEQ_CLAIMED = Gtk::EventSequenceState::CLAIMED;
#else
	using ClickGesture = Gtk::GestureMultiPress;
	constexpr auto SEQ_CLAIMED = Gtk::EVENT_SEQUENCE_CLAIMED;
#endif

	inline Glib::RefPtr<ClickGesture> add_click_gesture(Gtk::Widget& widget)
		{
#ifdef USE_GTK4
		auto gesture = ClickGesture::create();
		widget.add_controller(gesture);
		return gesture;
#else
		return ClickGesture::create(widget);
#endif
		}

	/// Gdk::Screen is gone in GTK4; style providers attach to a Gdk::Display.

	inline void add_css_provider_for_default_display(const Glib::RefPtr<Gtk::CssProvider>& provider,
																	 guint priority)
		{
#ifdef USE_GTK4
		Gtk::StyleContext::add_provider_for_display(Gdk::Display::get_default(), provider, priority);
#else
		Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), provider, priority);
#endif
		}

	/// The geometry of the monitor the window is on. gtkmm-3 looks this up from
	/// the window's Gdk::Window, GTK4 from its Gdk::Surface.

	inline Gdk::Rectangle monitor_geometry(Gtk::Window& window)
		{
		auto display = Gdk::Display::get_default();
		if(!display)
			throw std::logic_error("cdb::monitor_geometry: cannot get default display.");

#ifdef USE_GTK4
		auto monitor = display->get_monitor_at_surface(window.get_surface());
#else
		auto monitor = display->get_monitor_at_window(window.get_window());
#endif
		if(!monitor)
			throw std::logic_error("cdb::monitor_geometry: cannot get monitor for window.");

		Gdk::Rectangle geometry;
		monitor->get_geometry(geometry);
		return geometry;
		}

	/// Gtk::Window::set_type_hint() is removed in GTK4; the window manager is
	/// told what a dialog is via its transient-for and modal state instead.

	inline void set_dialog_type_hint(Gtk::Window& window)
		{
#ifdef USE_GTK4
		(void)window;
#else
		window.set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
#endif
		}

	/// GTK4 replaced Gtk::Paned::pack1/pack2 with set_start_child/set_end_child
	/// plus separate resize/shrink setters.

	inline void paned_set_start(Gtk::Paned& paned, Gtk::Widget& child, bool resize, bool shrink)
		{
#ifdef USE_GTK4
		paned.set_start_child(child);
		paned.set_resize_start_child(resize);
		paned.set_shrink_start_child(shrink);
#else
		paned.pack1(child, resize, shrink);
#endif
		}

	inline void paned_set_end(Gtk::Paned& paned, Gtk::Widget& child, bool resize, bool shrink)
		{
#ifdef USE_GTK4
		paned.set_end_child(child);
		paned.set_resize_end_child(resize);
		paned.set_shrink_end_child(shrink);
#else
		paned.pack2(child, resize, shrink);
#endif
		}

	/// GTK4 removed show_all(). There, widgets are visible by default and
	/// cdb::add_child/box_append show each child as it is added, so making the
	/// widget itself visible is all that remains. On gtkmm-3 this stays a
	/// literal show_all(), so behaviour is unchanged.

	inline void show_all(Gtk::Widget& widget)
		{
#ifdef USE_GTK4
		widget.set_visible(true);
#else
		widget.show_all();
#endif
		}

	/// As above, but the gtkmm-3 form that shows the descendants *without*
	/// showing the widget itself. Under GTK4 there is nothing to do: the
	/// children were shown as they were added.

	template<typename Container>
	inline void show_all_children(Container& container)
		{
#ifdef USE_GTK4
		(void)container;
#else
		container.show_all_children();
#endif
		}

	/// Put `child` inside `parent`. gtkmm-3 has Gtk::Container::add() for every
	/// container; GTK4 removed it in favour of append() on Gtk::Box and
	/// set_child() on the single-child containers, so the right call depends on
	/// the concrete parent type.

	template<typename Parent>
	inline void add_child(Parent& parent, Gtk::Widget& child)
		{
#ifdef USE_GTK4
		if constexpr (std::is_base_of_v<Gtk::Box, Parent>)
			parent.append(child);
		else
			parent.set_child(child);
#else
		parent.add(child);
		child.show();
#endif
		}

	/// Show `dialog` modally, block until the user responds, and return the
	/// response id.
	///
	/// GTK4 removed Gtk::Dialog::run(). The replacement below spins a nested
	/// main loop, which is exactly what gtkmm-3's run() does internally, so the
	/// two branches behave the same. Doing it this way keeps the ~32 call sites
	/// (and, importantly, NotebookWindow::quit_safeguard() and its five callers)
	/// in their current synchronous shape.

	inline int dialog_run(Gtk::Dialog& dialog)
		{
#ifdef USE_GTK4
		int response = static_cast<int>(Gtk::ResponseType::NONE);
		auto loop = Glib::MainLoop::create(false);

		auto conn_response = dialog.signal_response().connect(
		   [&response, loop](int id) {
			if(loop->is_running()) {
				response = id;
				loop->quit();
				}
			});

		// A dialog dismissed without emitting ::response (closed by the window
		// manager, or hidden programmatically) would otherwise leave us stuck
		// in the nested loop.
		auto conn_hide = dialog.signal_hide().connect(
		   [loop]() {
			if(loop->is_running())
				loop->quit();
			});

		dialog.set_modal(true);
		dialog.set_visible(true);
		loop->run();

		conn_response.disconnect();
		conn_hide.disconnect();
		dialog.set_visible(false);

		return response;
#else
		return dialog.run();
#endif
		}

	/// Accessible roles we actually use. GTK4 removed ATK entirely; its
	/// replacement sets the role as a construct-time property, so there is no
	/// runtime equivalent and the GTK4 branch is a no-op. Kept as a helper
	/// rather than deleted so the intent stays visible and so it can be
	/// implemented properly (by choosing accessible-role at widget
	/// construction) later.

	enum class AccessibleRole { math, text, push_button };

	inline void set_accessible_role(Gtk::Widget& w, AccessibleRole role)
		{
#ifdef USE_GTK4
		(void)w;
		(void)role;
		// FIXME: GTK4 needs Gtk::Accessible::Role passed at construction time.
#else
		auto obj = w.get_accessible();
		if(!obj) return;
		switch(role) {
			case AccessibleRole::math:        obj->set_role(Atk::ROLE_MATH);        break;
			case AccessibleRole::text:        obj->set_role(Atk::ROLE_TEXT);        break;
			case AccessibleRole::push_button: obj->set_role(Atk::ROLE_PUSH_BUTTON); break;
			}
#endif
		}

	inline void set_accessible_name(Gtk::Widget& w, const Glib::ustring& name)
		{
#ifdef USE_GTK4
		Glib::Value<Glib::ustring> value;
		value.init(Glib::Value<Glib::ustring>::value_type());
		value.set(name);
		w.update_property(Gtk::Accessible::Property::LABEL, value);
#else
		auto obj = w.get_accessible();
		if(obj)
			obj->set_name(name);
#endif
		}

	}
