
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
#include <gtkmm/aboutdialog.h>
#include <gtkmm/label.h>
#include <gtkmm/filechooser.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
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
#include <giomm/menu.h>
#include <giomm/inputstream.h>
#ifdef USE_GTK4
#include <gdkmm/clipboard.h>
#include <gdkmm/contentprovider.h>
#include <gdkmm/contentformats.h>
#else
#include <gtkmm/clipboard.h>
#include <gtkmm/selectiondata.h>
#include <gtkmm/targetentry.h>
#endif
#include <giomm/simpleactiongroup.h>
#ifdef USE_GTK4
#include <gtkmm/popovermenu.h>
#else
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#endif
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
#include <memory>
#include <functional>
#include <string>
#include <vector>

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

	constexpr auto FILE_CHOOSER_ACTION_OPEN = Gtk::FileChooser::Action::OPEN;
	constexpr auto FILE_CHOOSER_ACTION_SAVE = Gtk::FileChooser::Action::SAVE;
	constexpr auto LICENSE_GPL_3_0          = Gtk::License::GPL_3_0;
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

	constexpr auto FILE_CHOOSER_ACTION_OPEN = Gtk::FILE_CHOOSER_ACTION_OPEN;
	constexpr auto FILE_CHOOSER_ACTION_SAVE = Gtk::FILE_CHOOSER_ACTION_SAVE;
	constexpr auto LICENSE_GPL_3_0          = Gtk::License::LICENSE_GPL_3_0;
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

	/// Clipboard.
	///
	/// This is the one area with no common subset: gtkmm-3 has Gtk::Clipboard
	/// with X11 target atoms and *synchronous* reads, GTK4 has Gdk::Clipboard
	/// with mime types, Gdk::ContentProvider and asynchronous reads only. Both
	/// are hidden behind the two calls below.
	///
	/// Cadabra puts two things on the clipboard: the rendered text, and (when
	/// available) the cell's input form under a private "cadabra" name, so that
	/// pasting between Cadabra instances round-trips the input rather than the
	/// LaTeX. The same string is used as the gtkmm-3 target atom and the GTK4
	/// mime type, which keeps a GTK3 and a GTK4 build interoperable.

	inline const char *CADABRA_CLIPBOARD_TARGET = "cadabra";

	namespace clipboard_detail {

		inline std::string& text_payload()    { static std::string s; return s; }
		inline std::string& cadabra_payload() { static std::string s; return s; }

#ifndef USE_GTK4
		inline void on_get(Gtk::SelectionData& selection_data, guint)
			{
			if(selection_data.get_target() == CADABRA_CLIPBOARD_TARGET)
				selection_data.set(CADABRA_CLIPBOARD_TARGET, cadabra_payload());
			else
				selection_data.set_text(text_payload());
			}

		inline void on_clear() {}
#endif
		}

	/// Put `text` on the clipboard, plus `cadabra` under the private target if
	/// it is non-empty.

	inline void clipboard_set(Gtk::Widget& widget, const std::string& text, const std::string& cadabra)
		{
		clipboard_detail::text_payload()    = text;
		clipboard_detail::cadabra_payload() = cadabra;

#ifdef USE_GTK4
		std::vector<Glib::RefPtr<Gdk::ContentProvider>> providers;
		if(!cadabra.empty())
			providers.push_back(
			   Gdk::ContentProvider::create(CADABRA_CLIPBOARD_TARGET,
													  Glib::Bytes::create(cadabra.data(), cadabra.size())));

		Glib::Value<Glib::ustring> value;
		value.init(Glib::Value<Glib::ustring>::value_type());
		value.set(text);
		providers.push_back(Gdk::ContentProvider::create(value));

		widget.get_clipboard()->set_content(Gdk::ContentProvider::create(providers));
#else
		(void)widget;
		std::vector<Gtk::TargetEntry> targets;
		if(!cadabra.empty())
			targets.push_back(Gtk::TargetEntry(CADABRA_CLIPBOARD_TARGET));
		targets.push_back(Gtk::TargetEntry("UTF8_STRING"));
		targets.push_back(Gtk::TargetEntry("TEXT"));

		Gtk::Clipboard::get(GDK_SELECTION_CLIPBOARD)->set(
		   targets,
		   sigc::ptr_fun(&clipboard_detail::on_get),
		   sigc::ptr_fun(&clipboard_detail::on_clear));
#endif
		}

	/// Read the clipboard and hand the result to `on_received`. Prefers the
	/// private Cadabra target, falling back to plain text.
	///
	/// Always asynchronous. gtkmm-3 could read synchronously, but GTK4 cannot,
	/// so both paths defer; callers must not assume the callback has run by the
	/// time this returns.

	inline void clipboard_read(Gtk::Widget& widget, bool primary,
										const std::function<void(const std::string&)>& on_received)
		{
#ifdef USE_GTK4
		auto clipboard = primary ? widget.get_primary_clipboard() : widget.get_clipboard();

		if(clipboard->get_formats()->contain_mime_type(CADABRA_CLIPBOARD_TARGET)) {
			clipboard->read_async(
			   std::vector<Glib::ustring>{ CADABRA_CLIPBOARD_TARGET }, Glib::PRIORITY_DEFAULT,
			   [clipboard, on_received](const Glib::RefPtr<Gio::AsyncResult>& result) {
				try {
					Glib::ustring mime_type;
					auto stream = clipboard->read_finish(result, mime_type);
					if(!stream) return;

					std::string content;
					char buffer[4096];
					gssize n;
					while((n = stream->read(buffer, sizeof(buffer))) > 0)
						content.append(buffer, n);
					on_received(content);
					}
				catch(const Glib::Error&) { /* nothing on the clipboard for us */ }
				});
			return;
			}

		clipboard->read_text_async(
		   [clipboard, on_received](const Glib::RefPtr<Gio::AsyncResult>& result) {
			try {
				on_received(clipboard->read_text_finish(result));
				}
			catch(const Glib::Error&) { }
			});
#else
		(void)widget;
		auto clipboard = Gtk::Clipboard::get(primary ? GDK_SELECTION_PRIMARY : GDK_SELECTION_CLIPBOARD);
		clipboard->request_targets(
		   [clipboard, on_received](const std::vector<Glib::ustring>& targets) {
			std::string target;
			for(const auto& t : targets) {
				if(t == CADABRA_CLIPBOARD_TARGET) { target = t; break; }
				if(target.empty() && (t == "UTF8_STRING" || t == "TEXT" || t == "STRING"))
					target = t;
				}
			if(target.empty()) return;

			clipboard->request_contents(target,
			   [on_received](const Gtk::SelectionData& data) {
				on_received(data.get_data_as_string());
				});
			});
#endif
		}

	/// A right-click context menu.
	///
	/// gtkmm-3 builds one from Gtk::Menu + Gtk::MenuItem and pops it up at the
	/// pointer. Both of those classes are gone in GTK4, where a menu is a
	/// Gio::Menu *model* whose items name actions, shown through a
	/// Gtk::PopoverMenu parented to the widget. This wrapper hides the
	/// difference so the call sites read the same on both.

	class ContextMenu {
		public:
			ContextMenu() = default;
			ContextMenu(const ContextMenu&) = delete;
			ContextMenu& operator=(const ContextMenu&) = delete;

			~ContextMenu()
				{
#ifdef USE_GTK4
				// A Gtk::Popover attached with set_parent() must be unparented
				// before it goes away.
				if(popover) {
					popover->unparent();
					delete popover;
					}
#endif
				}

			/// Call once, before add_item(). `action_prefix` only has to be
			/// unique within `widget`.
			void attach_to(Gtk::Widget& widget, const Glib::ustring& action_prefix)
				{
#ifdef USE_GTK4
				prefix  = action_prefix;
				model   = Gio::Menu::create();
				actions = Gio::SimpleActionGroup::create();
				widget.insert_action_group(prefix, actions);

				popover = new Gtk::PopoverMenu(model);
				popover->set_parent(widget);
				popover->set_has_arrow(false);
#else
				(void)widget;
				(void)action_prefix;
#endif
				}

			void add_item(const Glib::ustring& label, const sigc::slot<void()>& callback)
				{
#ifdef USE_GTK4
				const Glib::ustring name = "item" + Glib::ustring::format(next_id++);
				actions->add_action(name, callback);
				model->append(label, prefix + "." + name);
#else
				auto* item = Gtk::make_managed<Gtk::MenuItem>(label);
				item->signal_activate().connect(callback);
				menu.append(*item);
				item->show();
#endif
				}

			/// `x`, `y` are relative to the widget passed to attach_to().
			void popup_at(double x, double y)
				{
#ifdef USE_GTK4
				popover->set_pointing_to(Gdk::Rectangle(static_cast<int>(x), static_cast<int>(y), 1, 1));
				popover->popup();
#else
				(void)x;
				(void)y;
				// nullptr means 'the event currently being processed'.
				menu.popup_at_pointer(nullptr);
#endif
				}

		private:
#ifdef USE_GTK4
			Glib::ustring                        prefix;
			Glib::RefPtr<Gio::Menu>              model;
			Glib::RefPtr<Gio::SimpleActionGroup> actions;
			Gtk::PopoverMenu                    *popover = nullptr;
			int                                  next_id = 0;
#else
			Gtk::Menu menu;
#endif
		};

	/// glibmm-2.68 made Glib::RefPtr an alias for std::shared_ptr, which dropped
	/// the static RefPtr<T>::cast_dynamic().

	template<typename T, typename U>
	inline Glib::RefPtr<T> refptr_dynamic_cast(const Glib::RefPtr<U>& ptr)
		{
#ifdef USE_GTK4
		return std::dynamic_pointer_cast<T>(ptr);
#else
		return Glib::RefPtr<T>::cast_dynamic(ptr);
#endif
		}

	/// Gtk::Window::set_position() is removed in GTK4, where placement is left
	/// to the compositor. (Gtk::Paned::set_position() is unrelated and fine.)

	inline void window_centre_on_parent(Gtk::Window& window)
		{
#ifdef USE_GTK4
		(void)window;
#else
		window.set_position(Gtk::WIN_POS_CENTER);
#endif
		}

	/// Gtk::Label::set_line_wrap() is set_wrap() in GTK4.

	inline void label_set_wrap(Gtk::Label& label, bool wrap = true)
		{
#ifdef USE_GTK4
		label.set_wrap(wrap);
#else
		label.set_line_wrap(wrap);
#endif
		}

	/// GTK4 has no single "margin" property, only the four sides. Both toolkits
	/// have the individual setters, so this needs no conditional.

	inline void set_margin_all(Gtk::Widget& widget, int margin)
		{
		widget.set_margin_start(margin);
		widget.set_margin_end(margin);
		widget.set_margin_top(margin);
		widget.set_margin_bottom(margin);
		}

	/// GTK4 added a baseline argument to Gtk::Widget::size_allocate().

	inline void widget_size_allocate(Gtk::Widget& widget, const Gtk::Allocation& allocation)
		{
#ifdef USE_GTK4
		widget.size_allocate(allocation, -1);
#else
		widget.size_allocate(allocation);
#endif
		}

	/// Position of a point in `src`'s coordinate space, expressed in `dest`'s.
	/// Returns false if the two have no common ancestor, in which case nothing
	/// is stored.
	///
	/// Needed because get_allocation() does not mean the same thing in the two
	/// toolkits: gtkmm-3 reports a widget's position within the GdkWindow it
	/// draws on -- which, inside a Gtk::ScrolledWindow, is the scrolled bin
	/// window, so allocations come out directly in document coordinates. GTK4
	/// has no such windows and get_allocation() is relative to the immediate
	/// parent, so anything comparing a cell's position against a scroll
	/// adjustment has to translate explicitly. The signature also differs:
	/// gtkmm-3 takes and returns ints, GTK4 doubles.

	inline bool translate_widget_coords(Gtk::Widget& src, Gtk::Widget& dest,
													double src_x, double src_y,
													double& dest_x, double& dest_y)
		{
#ifdef USE_GTK4
		return src.translate_coordinates(dest, src_x, src_y, dest_x, dest_y);
#else
		int dx = 0, dy = 0;
		if(!src.translate_coordinates(dest, static_cast<int>(src_x), static_cast<int>(src_y), dx, dy))
			return false;
		dest_x = dx;
		dest_y = dy;
		return true;
#endif
		}

	/// Gtk::AboutDialog derives from Gtk::Dialog on gtkmm-3 but from Gtk::Window
	/// on GTK4, so it has no ::response signal there and cannot go through
	/// dialog_run(). On GTK4 it is simply presented and dismissed by the user.

	inline int dialog_run(Gtk::Dialog& dialog);   // defined further down

	inline void about_dialog_run(Gtk::AboutDialog& about)
		{
#ifdef USE_GTK4
		about.set_modal(true);
		about.set_visible(true);
#else
		dialog_run(about);
#endif
		}

	/// GTK4's Gtk::AboutDialog::set_logo() takes a Gdk::Paintable rather than a
	/// Gdk::Pixbuf.

	inline void about_dialog_set_logo(Gtk::AboutDialog& about, const Glib::RefPtr<Gdk::Pixbuf>& logo)
		{
#ifdef USE_GTK4
		about.set_logo(Gdk::Texture::create_for_pixbuf(logo));
#else
		about.set_logo(logo);
#endif
		}

	/// Wrap a freshly-constructed Glib::Object subclass in a RefPtr.
	///
	/// glibmm-2.68 made Glib::RefPtr an alias for std::shared_ptr, so
	/// `Glib::RefPtr<T>(new T())` is no longer merely old-fashioned but wrong:
	/// the object is already reference-counted by GObject, and the shared_ptr
	/// would add a second, independent count and double-free. glibmm-2.68
	/// provides make_refptr_for_instance() for this; glibmm-2.4 does not have
	/// it, but there the plain constructor is correct.

	template<typename T>
	inline Glib::RefPtr<T> refptr_for_instance(T* object)
		{
#ifdef USE_GTK4
		return Glib::make_refptr_for_instance<T>(object);
#else
		return Glib::RefPtr<T>(object);
#endif
		}

	/// Gtk::CssProvider::load_from_data() returns bool on gtkmm-3 but void on
	/// GTK4, where a parse failure is reported through ::parsing-error instead.

	inline bool css_load_from_data(const Glib::RefPtr<Gtk::CssProvider>& provider,
											 const std::string& data)
		{
#ifdef USE_GTK4
		provider->load_from_data(data);
		return true;
#else
		return provider->load_from_data(data);
#endif
		}

	/// gtkmm-3's Gtk::Container API for inspecting and reordering children is
	/// gone in GTK4: there is no get_children(), and reorder_child(child, n)
	/// became reorder_child_after(child, sibling).

	inline unsigned int box_child_count(Gtk::Box& box)
		{
#ifdef USE_GTK4
		unsigned int n = 0;
		for(auto *child = box.get_first_child(); child; child = child->get_next_sibling())
			++n;
		return n;
#else
		return box.get_children().size();
#endif
		}

	/// Move `child` to absolute position `index` among `box`'s children.

	inline void box_reorder_child(Gtk::Box& box, Gtk::Widget& child, unsigned int index)
		{
#ifdef USE_GTK4
		if(index==0) {
			box.reorder_child_at_start(child);
			return;
			}
		// Find the sibling that should end up just before `child`, skipping
		// `child` itself so the index refers to the list without it.
		Gtk::Widget *sibling = nullptr;
		unsigned int seen = 0;
		for(auto *c = box.get_first_child(); c; c = c->get_next_sibling()) {
			if(c == &child) continue;
			sibling = c;
			if(++seen == index) break;
			}
		if(sibling)
			box.reorder_child_after(child, *sibling);
#else
		box.reorder_child(child, index);
#endif
		}

	/// Detaching a child. gtkmm-3 has Gtk::Container::remove(); GTK4 dropped it
	/// for the single-child containers, where the child is a property instead.

	inline void scrolled_window_unset_child(Gtk::ScrolledWindow& sw)
		{
#ifdef USE_GTK4
		sw.unset_child();
#else
		sw.remove();
#endif
		}

	inline void paned_unset_end(Gtk::Paned& paned)
		{
#ifdef USE_GTK4
		paned.property_end_child() = nullptr;
#else
		if(auto *child = paned.get_child2())
			paned.remove(*child);
#endif
		}

	/// Gtk::Widget::show_now() is removed in GTK4; there is no way (nor need)
	/// to force a synchronous map.

	inline void show_now(Gtk::Widget& widget)
		{
#ifdef USE_GTK4
		widget.set_visible(true);
#else
		widget.show_now();
#endif
		}

	/// GTK4 removed Gtk::FileChooser::set_do_overwrite_confirmation(): a save
	/// dialog there always confirms an overwrite.

	inline void file_chooser_confirm_overwrite(Gtk::FileChooser& chooser)
		{
#ifdef USE_GTK4
		(void)chooser;
#else
		chooser.set_do_overwrite_confirmation(true);
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
