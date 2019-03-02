#include "ChooseColoursDialog.hh"
#include <gtkmm/label.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/messagedialog.h>
#include <locale>
#include <iostream>

namespace cadabra {
	std::string capitalize_first(std::string in)
		{
		if (in.empty())
			return in;
		in[0] = std::toupper(in[0]);
		for (int i = 1; i < (int)in.size() - 1; ++i) {
			if (in[i] == ' ')
				in[i + 1] = std::toupper(in[i + 1]);
			}
		return in;
		}

	ChooseColoursDialog::ChooseColoursDialog(DocumentThread::Prefs& prefs, NotebookWindow& parent)
		: Gtk::Dialog("Choose syntax highlighting colours", parent, true)
		, prefs(prefs), button_ok("Ok"), parent(parent)
		{
		using namespace std::string_literals;

		set_transient_for(parent);
		set_modal(true);
		get_content_area()->pack_start(main_vbox);
		main_vbox.pack_start(main_grid);
		int col = 0;
		for (auto it = prefs.colours.begin(); it != prefs.colours.end(); ++it, ++col) {
			// Display the name of the language to highlight for
			auto sec_label = std::make_unique<Gtk::Label>(capitalize_first(it->first));
			auto sec_grid = std::make_unique<Gtk::Grid>();
			sec_grid->set_column_spacing(10);
			main_grid.attach(*sec_label, col, 0, 1, 1);
			main_grid.attach(*sec_grid, col, 1, 1, 1);

			//Display the highlightable things and a colour chooser
			int row = 0;
			for (auto jt = it->second.begin(); jt != it->second.end(); ++jt, ++row) {
				// Make the label and give it the colour that type of keyword is currently highlighted as
				auto kw_label = std::make_unique<Gtk::Label>(capitalize_first(jt->first));
				auto kw_button = std::make_unique<Gtk::ColorButton>();
				kw_button->property_rgba() = Gdk::RGBA(jt->second);
				kw_button->set_title("Choosing colours for "s + capitalize_first(it->first) + ": " + capitalize_first(jt->first));
				kw_button->signal_color_set().connect(sigc::mem_fun(this, &ChooseColoursDialog::on_color_set));
				sec_grid->attach(*kw_label, 0, row, 1, 1);
				sec_grid->attach(*kw_button, 1, row, 1, 1);
				anonymous_widgets.push_back(std::move(kw_label));
				colour_buttons[it->first][jt->first] = std::move(kw_button);
				}

			// Move sec_label and sec_grid to class scope so they don't get destroyed
			anonymous_widgets.push_back(std::move(sec_label));
			anonymous_widgets.push_back(std::move(sec_grid));
			}
		main_grid.set_column_spacing(30);
		main_grid.set_margin_start(30);
		main_grid.set_margin_end(30);
		main_grid.set_margin_top(10);
		main_grid.set_margin_bottom(10);

		main_vbox.pack_start(bottom_button_box);
		bottom_button_box.pack_start(button_ok);

		button_ok.signal_clicked().connect( [&]() {
			close();
			} );

		main_vbox.show_all();
		}

	void ChooseColoursDialog::on_color_set()
		{
		for (auto& language : colour_buttons) {
			for (auto& kw : language.second) {
				prefs.colours[language.first][kw.first] = kw.second->get_rgba().to_string();
				}
			}
		parent.refresh_highlighting();
		/*prefs.colours[cur_lang][cur_kw_type] = std::string(colour.to_string());
		dynamic_cast<Gtk::Label*>(label_widgets[cur_lang + cur_kw_type].get())->set_markup(
			std::string("<span foreground=\"") + colour.to_string() + "\">" + capitalize_first(cur_kw_type) + "</span>"
		);
		label_widgets[cur_lang + cur_kw_type]->show();*/
		}
	}
