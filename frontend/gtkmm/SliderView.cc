
#include "SliderView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace cadabra;

int compute_scale_digits(double step_increment) {
    if (step_increment >= 1.0) return 0;
    
    // Convert to string in scientific notation and find 'e-X' part
    std::ostringstream ss;
    ss << std::scientific << step_increment;
    std::string str = ss.str();
    
    size_t e_pos = str.find('e');
    if (e_pos == std::string::npos) return 0;
    
    // Parse the exponent
    int exponent = std::stoi(str.substr(e_pos + 1));
    return exponent < 0 ? -exponent : 0;
}

SliderView::SliderView(std::string config)
	{
	// Extract the settings from the JSON string.
	nlohmann::json settings = nlohmann::json::parse(config);
	value = settings.value("value", 3.0);
	min_value = settings.value("min_value", 0.0);
	max_value = settings.value("max_value", 10.0);
	variable  = settings.value("variable", "");
	step_size = settings.value("step_size", 0.1);

	add(vbox);
	adjustment = Gtk::Adjustment::create(
		value,       // value
		min_value,   // lower
		max_value,   // upper
		step_size,   // step increment
		10.0,        // page_increment
		0.0          // page_size
													 );
	
	scale.set_adjustment(adjustment);
	scale.set_digits(compute_scale_digits(step_size));
	vbox.pack_start(scale, Gtk::PACK_SHRINK);
	set_name("SliderView"); // to be able to style it with CSS
	show_all();
	}

SliderView::~SliderView()
	{
	}

std::string SliderView::get_variable() const
	{
	return variable;
	}
