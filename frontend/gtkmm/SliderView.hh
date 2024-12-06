
#pragma once

#include <gtkmm/box.h>
#include <gtkmm/scale.h>
#include <gtkmm/eventbox.h>
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

	class SliderView : public Gtk::EventBox {
		public:
			SliderView(std::string config);
			virtual ~SliderView();

			std::string get_variable() const;
			
			Glib::RefPtr<Gtk::Adjustment> adjustment;

		private:
			Gtk::VBox   vbox;
			Gtk::Scale  scale;

			double value, min_value, max_value;
			std::string variable;
		};

	};
