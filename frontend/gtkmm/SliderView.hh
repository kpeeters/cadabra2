
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/scale.h>
#ifndef USE_GTK4
#include <gtkmm/eventbox.h>
#endif
#include <gtkmm/adjustment.h>

namespace cadabra {

	/// \ingroup frontend
	///
	/// An slider widget. The configuration string is a JSON object
	/// which can contain the following key/value pairs:
	///
	///   - value:      double
	///   - min_value:  double
	///   - max_value:  double
	///   - variable:   string

#ifdef USE_GTK4
	class SliderView : public Gtk::Box {
#else
	class SliderView : public Gtk::EventBox {
#endif
		public:
			SliderView(std::string config);
			virtual ~SliderView();

			std::string get_variable() const;
			
			Glib::RefPtr<Gtk::Adjustment> adjustment;

		private:
			Gtk::VBox   vbox;
			Gtk::Scale  scale;

			double value, min_value, max_value, step_size;
			std::string variable;
		};

	};
