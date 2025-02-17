
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
	: logical_width_at_start(logical_width_)
	, sizing(false)
	, prev_x(0)
	, prev_y(0)
	, height_at_press(0)
	, width_at_press(0)
	{
	area.display_scale = display_scale_;
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
		rerender(cw);
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
	// The data is ok:
	// std::ofstream tst("out2.png");
	// tst << Glib::Base64::decode(b64);
	// tst.close();

	area.decoded=Glib::Base64::decode(b64);
	area.is_raster=true;
	rerender(logical_width_at_start);
	}

void ImageView::set_image_from_svg(const std::string& svg)
	{
	area.decoded=Glib::Base64::decode(svg);
	area.is_raster=false;
	rerender(logical_width_at_start);
	}

void ImageView::rerender(int width)
	{
	auto str = Gio::MemoryInputStream::create();
	str->add_data(area.decoded.c_str(), area.decoded.size());

	// Widths set here are all logical pixel widths, not device pixel widths.
	area.pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(str, width * area.display_scale, -1, true);
	// std::cerr << "creating at " << width * area.display_scale << std::endl;

	if(!area.pixbuf) {
		std::cerr << "cadabra-client: unable to create image from data" << std::endl;
		}

	set_size_request( area.pixbuf->get_width()/area.display_scale,
							area.pixbuf->get_height()/area.display_scale );
	queue_resize();
	}

bool ImageView::ImageArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	cr->scale(1.0/display_scale, 1.0/display_scale);
	Gdk::Cairo::set_source_pixbuf(cr, pixbuf, 0, 0);
	cr->paint();
	return true;
	}

