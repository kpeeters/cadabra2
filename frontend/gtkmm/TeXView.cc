
#include "TeXView.hh"
#include <iostream>

using namespace cadabra;

TeXView::TeXView(TeXEngine& eng, const std::string& texb, int hmargin)
	: content(0), vbox(false, 10), hbox(false, hmargin), engine(eng)
	{
	content = engine.checkin(texb, "", "");

	add(vbox);
	vbox.set_margin_top(10);
	vbox.set_margin_bottom(0);
	vbox.pack_start(hbox, Gtk::PACK_SHRINK, 0);
	hbox.pack_start(image, Gtk::PACK_SHRINK, hmargin);
//	set_state(Gtk::STATE_PRELIGHT);
	override_background_color(Gdk::RGBA("white"));
	}

void TeXView::on_show()
	{
	try {
		engine.convert_all();

		Glib::RefPtr<Gdk::Pixbuf> pixbuf = 
			Gdk::Pixbuf::create_from_data(content->image().data(), Gdk::COLORSPACE_RGB, 
													true,
													8, 
													content->width(), content->height(),
													4*content->width());
		
		image.set(pixbuf);
		
		Gtk::EventBox::on_show();
		}
	catch(TeXEngine::TeXException& ex) {
		tex_error.emit(ex.what());
		}
	}

void TeXView::update_image()
	{
	std::cerr << "update" << std::endl;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf = 
		Gdk::Pixbuf::create_from_data(content->image().data(), Gdk::COLORSPACE_RGB, 
												true,
												8, 
												content->width(), content->height(),
												4*content->width());

	image.set(pixbuf);
	}


