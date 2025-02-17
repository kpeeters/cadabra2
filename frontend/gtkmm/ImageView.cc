
#include "ImageView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>

#include <gtkmm/image.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/window.h>
#include <gdkmm/general.h> // set_source_pixbuf()
#include <cairomm/surface.h>
#include <cairomm/context.h>

#include <iostream>
#include <fstream>

using namespace cadabra;

ImageView::ImageView(double display_scale_, int logical_width_)
	: area(logical_width_, display_scale_)
	, sizing(false)
	, prev_x(0)
	, prev_y(0)
	, height_at_press(0)
	, width_at_press(0)
	{
	add(area);
	
	set_events(Gdk::ENTER_NOTIFY_MASK
	           | Gdk::LEAVE_NOTIFY_MASK
	           | Gdk::BUTTON_PRESS_MASK
	           | Gdk::BUTTON_RELEASE_MASK
	           | Gdk::POINTER_MOTION_MASK);

	set_name("ImageView"); // to be able to style it with CSS
	show_all();
	}

ImageView::~ImageView()
	{
	}

bool ImageView::on_motion_notify_event(GdkEventMotion *event)
	{
	//	std::cerr << event->x << ", " << event->y << std::endl;
	if(sizing) {
		auto cw = width_at_press  + (event->x - prev_x);
		area.rerender(cw);
		}
	return true;
	}

bool ImageView::on_button_press_event(GdkEventButton *event)
	{
	if(event->type==GDK_BUTTON_PRESS) {
		sizing=true;
		prev_x=event->x;
		prev_y=event->y;
		width_at_press=area.pixbuf->get_width()/area.display_scale;
		// std::cerr << "width_at_press = " << width_at_press << std::endl;
		height_at_press=area.pixbuf->get_height()/area.display_scale;
		}
	return true;
	}

bool ImageView::on_button_release_event(GdkEventButton *event)
	{
	if(event->type==GDK_BUTTON_RELEASE) {
		sizing=false;
		}

	return true;
	}

void ImageView::set_image_from_base64(const std::string& b64)
	{
	area.set_image_from_base64(b64);
	}

void ImageArea::set_image_from_base64(const std::string& b64)
	{
	// The data is ok:
	// std::ofstream tst("out2.png");
	// tst << Glib::Base64::decode(b64);
	// tst.close();

	decoded=Glib::Base64::decode(b64);
	is_raster=true;
	rerender(logical_width);
	}

void ImageView::set_image_from_svg(const std::string& svg)
	{
	area.set_image_from_svg(svg);
	}

void ImageArea::set_image_from_svg(const std::string& svg)
	{
	decoded=Glib::Base64::decode(svg);
	is_raster=false;
	rerender(logical_width);
	}

void ImageArea::rerender(int width)
	{
	auto str = Gio::MemoryInputStream::create();
	str->add_data(decoded.c_str(), decoded.size());

	// Widths set here are all logical pixel widths, not device pixel widths.
	pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(str, width * display_scale, -1, true);
	// std::cerr << "creating at " << width * area.display_scale << std::endl;

	if(!pixbuf) {
		std::cerr << "cadabra-client: unable to create image from data" << std::endl;
		}

	set_size_request( pixbuf->get_width()/display_scale,
							pixbuf->get_height()/display_scale );
	queue_resize();
	}

ImageArea::ImageArea(int logical_width_, double display_scale_)
	: is_raster(false), logical_width(logical_width_), display_scale(display_scale_)
	{
	set_halign(Gtk::ALIGN_CENTER);
	set_valign(Gtk::ALIGN_CENTER);
	}

std::string readFile(const std::string& filename)
	{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file) return "";
   
	std::string str(file.tellg(), 0);
	file.seekg(0);
	file.read(str.data(), str.size());
	return str;
	}

ImageArea::ImageArea(int logical_width_, double display_scale_,
							const std::string& filename, bool raster)
	: ImageArea(logical_width_, display_scale_)
	{
	is_raster = raster;
	decoded = readFile(filename);
	rerender(logical_width);
	}

bool ImageArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	cr->scale(1.0/display_scale, 1.0/display_scale);
	Gdk::Cairo::set_source_pixbuf(cr, pixbuf, 0, 0);
	cr->paint();
	return true;
	}

