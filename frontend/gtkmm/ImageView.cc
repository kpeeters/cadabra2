
#include "ImageView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>

using namespace cadabra;

ImageView::ImageView()
	{
	add(image);
	image.set_halign(Gtk::ALIGN_START);
	set_events(Gdk::ENTER_NOTIFY_MASK|Gdk::LEAVE_NOTIFY_MASK);
	set_name("ImageView"); // to be able to style it with CSS
	show_all();
	//set_size_request(300, 300);
	}

ImageView::~ImageView()
	{
	}
 
void ImageView::set_image_from_base64(const std::string& b64)
	{
	auto str = Gio::MemoryInputStream::create();

	// The data is ok: 
	// std::ofstream tst("out2.png");
	// tst << Glib::Base64::decode(b64);
	// tst.close();
	
	std::string dec=Glib::Base64::decode(b64);
	str->add_data(dec.c_str(), dec.size()); 

	Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_stream_at_scale(str,400,-1,true);
	if(!pixbuf)
		std::cerr << "cadabra-client: unable to create image from data" << std::endl;
	else {
//		pixbuf->scale_simple(400,300,Gdk::INTERP_BILINEAR);
		image.set(pixbuf);
		}
	}
