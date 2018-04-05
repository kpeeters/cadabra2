#ifndef CADABRA_CHOOSE_COLOURS_DIALOG_HH
#define CADABRA_CHOOSE_COLOURS_DIALOG_HH

#include <gtkmm/dialog.h>
#include <gtkmm/grid.h>
#include <json/json.h>
#include <gtkmm/colorbutton.h>
#include <vector>
#include <memory>
#include <map>
#include "NotebookWindow.hh"

namespace cadabra
{
	class ChooseColoursDialog : public Gtk::Dialog
	{
	public:
		enum responses
		{
			RESPONSE_PREVIEW = 101,
			RESPONSE_CHANGED
		};
		ChooseColoursDialog(DocumentThread::Prefs& prefs, NotebookWindow& parent);
	private:
		DocumentThread::Prefs& prefs;
		std::map<std::string, std::map<std::string, std::unique_ptr<Gtk::ColorButton>>> colour_buttons;
		std::vector<std::unique_ptr<Gtk::Widget>> anonymous_widgets;
		Gtk::Grid main_grid;
		void on_my_response(int response_id);
		void on_color_set();
		NotebookWindow& parent;

	};
}

#endif //CADABRA_CHOOSE_COLOURS_DIALOG_HH