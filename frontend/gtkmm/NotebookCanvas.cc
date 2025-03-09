
#include "NotebookCanvas.hh"
#include "NotebookWindow.hh"
#include <gtkmm/box.h>
#include <iostream>

using namespace cadabra;

NotebookCanvas::NotebookCanvas()
	: Gtk::Paned(Gtk::Orientation::ORIENTATION_VERTICAL)
	, scroller(scroll.get_vadjustment())
	{
	// Pack the scroll widget with all document cells into the top pane.
	pack1(scroll, true, true);
	scroll.set_policy(Gtk::POLICY_ALWAYS, Gtk::POLICY_ALWAYS);
	scroll.set_border_width(1);

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
	scroll.get_vscrollbar()->signal_change_value().connect(
		[this](Gtk::ScrollType, double) {
		scroller.stop();
		scroll_event();
		return false;
		});

	// Catch mousewheel / trackpad scrolls.
	if (auto viewport = dynamic_cast<Gtk::Viewport*>(scroll.get_child())) {
		viewport->add_events(Gdk::SCROLL_MASK | Gdk::SMOOTH_SCROLL_MASK);
		viewport->signal_scroll_event().connect(
			[this](GdkEventScroll* event) -> bool {
			scroller.stop();
			scroll_event();
			return false;
			});
		}

	}
