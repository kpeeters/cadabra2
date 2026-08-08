
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

#include <gtkmm/dialog.h>
#include <gtkmm/widget.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>
#include <glibmm/value.h>

namespace cdb {

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
