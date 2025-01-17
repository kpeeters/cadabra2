
#include "ImageView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>

using namespace cadabra;

ImageView::ImageView(double scale_)
	: sizing(false), prev_x(0), prev_y(0), height_at_press(0), width_at_press(0), scale(scale_)
	{
	add(vbox);
	vbox.add(image);
	image.set_halign(Gtk::ALIGN_START);
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
//		auto ratio = pixbuf->get_width() / ((double)pixbuf->get_height());
//		image.set(pixbuf->scale_simple(cw, cw/ratio, Gdk::INTERP_BILINEAR));
//		set_size_request( cw, cw/ratio );
		}
	return true;
	}

bool ImageView::on_button_press_event(GdkEventButton *event)
	{
	if(event->type==GDK_BUTTON_PRESS) {
		sizing=true;
		prev_x=event->x;
		prev_y=event->y;
		width_at_press=image.get_allocated_width();
		height_at_press=image.get_allocated_height();
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

	decoded=Glib::Base64::decode(b64);
	is_raster=true;
	rerender();
	}

void ImageView::set_image_from_svg(const std::string& svg)
	{
	decoded=Glib::Base64::decode(svg);
	is_raster=false;
	rerender();
	}

void ImageView::rerender(int override_width)
	{
	auto str = Gio::MemoryInputStream::create();
	str->add_data(decoded.c_str(), decoded.size());

	int curwidth=400*scale;
	if(pixbuf)
		curwidth = pixbuf->get_width();
	if(override_width!=0)
		curwidth=override_width;

	pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(str, curwidth, -1, true);
	
	if(!pixbuf)
		std::cerr << "cadabra-client: unable to create image from data" << std::endl;
	else {
		image.set(pixbuf);
		image.set_size_request( pixbuf->get_width(), pixbuf->get_height() );
		}
	}
