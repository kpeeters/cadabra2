
#include "SliderView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace cadabra;

SliderView::SliderView(std::string config)
	{
	// Extract the settings from the JSON string.
	nlohmann::json settings = nlohmann::json::parse(config);
	value = settings.value("value", 3.0);
	min_value = settings.value("min_value", 0.0);
	max_value = settings.value("max_value", 10.0);
	variable  = settings.value("variable", "");

	add(vbox);
	adjustment = Gtk::Adjustment::create(
		value,       // value
		min_value,   // lower
		max_value,   // upper
		1.0,         // step increment
		10.0,        // page_increment
		0.0          // page_size
													 );
	
	scale.set_adjustment(adjustment);
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
