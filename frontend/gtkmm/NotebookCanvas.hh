
#pragma once

#include <map>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/separator.h>
#include <gtkmm/eventbox.h>
#include <glibmm/dispatcher.h>

#include "VisualCell.hh"

/// NotebookCanvas is an actual view on the document. There can be any
/// number of them active inside the NotebookWindow. Each DataCell in the
/// notebook document has a corresponding VisualCell in the NotebookCanvas,
/// which gets added by NotebookCanvas::add_cell.
///
/// Cells which contain child cells (e.g. CodeInput, which can contain
/// child cells corresponding to the TeXView output) will also be
/// hierarchically ordered in the visual tree. That is, any visual cell
/// which can contain a child cell will have it stored inside a Gtk::Box
/// inside the visual cell. Removing any cell will therefore also
/// immediately remove the child cells.

namespace cadabra {

	class NotebookWindow;

	class SmoothScroller {
		private:
			Glib::RefPtr<Gtk::Adjustment> adjustment_;
			double target_value_;
			double start_value_;
			double duration_ms_;
			double elapsed_ms_;
			sigc::connection timeout_connection_;
		
			static constexpr double FRAME_RATE_MS = 16.67; // ~60 FPS
		
			bool on_timeout()
				{
				elapsed_ms_ += FRAME_RATE_MS;
			
				if (elapsed_ms_ >= duration_ms_) {
					adjustment_->set_value(target_value_);
					timeout_connection_.disconnect();
					return false;
					}
			
				// Use easing function (ease-out cubic) for smooth animation
				double progress = elapsed_ms_ / duration_ms_;
				double cubic = 1 - std::pow(1 - progress, 3);
			
				double current = start_value_ + 
					(target_value_ - start_value_) * cubic;
			
				adjustment_->set_value(current);
				return true;
				}
		
		public:
			SmoothScroller(Glib::RefPtr<Gtk::Adjustment> adj) 
				: adjustment_(adj)
				, elapsed_ms_(0)
				, duration_ms_(400) // 400ms default duration
				{}
		
			void scroll_to(double target)
				{
				timeout_connection_.disconnect(); // Stop any existing animation
			
				target_value_ = target;
				start_value_ = adjustment_->get_value();
				elapsed_ms_ = 0;
			
				// Start the animation loop
				timeout_connection_ = Glib::signal_timeout().connect(
					sigc::mem_fun(*this, &SmoothScroller::on_timeout),
					FRAME_RATE_MS
																					  );
				}
		
			void set_duration(double ms) {
			duration_ms_ = ms;
			}
	};

	class NotebookCanvas : public Gtk::Paned {
		public:
			NotebookCanvas();
			~NotebookCanvas();

			std::map<DataCell *, VisualCell> visualcells;

			Gtk::EventBox             ebox;
			Gtk::ScrolledWindow       scroll;
			Gtk::Separator            bottomline;
			SmoothScroller            scroller;

			void refresh_all();

		};

	}
