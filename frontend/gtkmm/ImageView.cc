
#include "ImageView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>

using namespace cadabra;

ImageView::ImageView()
	{
	add(image);
	show_all();
	}

ImageView::~ImageView()
	{
	std::cerr << "*** ~ImageView" << std::endl;
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

	Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_stream(str);
	if(pixbuf==0)
		std::cerr << "cadabra-client: unable to create image from data" << std::endl;
	else {
		image.set(pixbuf);
		}
	}
