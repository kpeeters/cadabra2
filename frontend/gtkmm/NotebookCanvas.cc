
#include "NotebookCanvas.hh"
#include "NotebookWindow.hh"
#include "GtkCompat.hh"
#include <gtkmm/box.h>
#ifdef USE_GTK4
#include <gtkmm/eventcontrollerscroll.h>
#endif
#include <iostream>

using namespace cadabra;

NotebookCanvas::NotebookCanvas()
	: Gtk::Paned(cdb::ORIENT_VERTICAL)
	, scroller(scroll.get_vadjustment())
	{
	// Pack the scroll widget with all document cells into the top pane.
	cdb::paned_set_start(*this, scroll, true, true);
	scroll.set_policy(cdb::POLICY_ALWAYS, cdb::POLICY_ALWAYS);
	// set_border_width() is removed in GTK4; margins are the replacement and
	// exist in both.
	scroll.set_margin_start(1);
	scroll.set_margin_end(1);
	scroll.set_margin_top(1);
	scroll.set_margin_bottom(1);

	// Do NOT do the following. This will create areas at the top
	// and bottom where the content of the scrolledwindow is
	// covered with white (except when totally at the top or
	// bottom of the content).
	// scroll.override_background_color(Gdk::RGBA("white"));

	}

NotebookCanvas::~NotebookCanvas()
	{
	}

void NotebookCanvas::refresh_all()
	{
	auto it=visualcells.begin();
	while(it!=visualcells.end()) {
		auto ct=it->first->cell_type;
		if(ct==DataCell::CellType::output || ct==DataCell::CellType::verbatim || ct==DataCell::CellType::latex_view) {
			it->second.outbox->update_image();
			it->second.outbox->queue_resize();
			}
		++it;
		}
	}

void NotebookCanvas::connect_scroll_listener()
	{
	// Ensure that if the content of the ScrolledWindow is scrolled, we
	// immediately stop any scrolling that is still in progress (using
	// the smooth scroller).

	// Scrollbar drag.
#ifndef USE_GTK4
	scroll.get_vscrollbar()->signal_change_value().connect(
		[this](Gtk::ScrollType, double) {
		scroller.stop();
		scroll_event();
		return false;
		});
#else
	// FIXME: no GTK4 equivalent yet. Gtk::Scrollbar no longer derives from
	// Gtk::Range there, so signal_change_value() is gone, and watching the
	// adjustment instead would also fire for the smooth scroller's own
	// programmatic updates and thus immediately stop it. Needs either a
	// Gtk::GestureDrag on the scrollbar or a suppression flag around
	// SmoothScroller's writes.
#endif

	// Catch mousewheel / trackpad scrolls.
	if (auto viewport = dynamic_cast<Gtk::Viewport*>(scroll.get_child())) {
#ifdef USE_GTK4
		auto scroll_controller = Gtk::EventControllerScroll::create();
		scroll_controller->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
		scroll_controller->signal_scroll().connect(
			[this](double, double) -> bool {
			scroller.stop();
			scroll_event();
			return false;
			}, false);
		viewport->add_controller(scroll_controller);
#else
		viewport->add_events(Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK);
		viewport->signal_scroll_event().connect(
			[this](GdkEventScroll* event) -> bool {
			scroller.stop();
			scroll_event();
			return false;
			});
#endif
		}

	}
