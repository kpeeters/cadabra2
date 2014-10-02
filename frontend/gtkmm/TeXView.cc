
#include "TeXView.hh"

using namespace cadabra;

TeXView::TeXView(TeXEngine& engine, const std::string& texb, int hmargin)
	: content(0), vbox(false, 10), hbox(false, hmargin)
	{
	content = engine.checkin(texb, "", "");

	add(vbox);
	vbox.pack_start(hbox, Gtk::PACK_SHRINK, 0);
	hbox.pack_start(image, Gtk::PACK_SHRINK, hmargin);
//	set_state(Gtk::STATE_PRELIGHT);
//	modify_bg(Gtk::STATE_NORMAL, Gdk::Color("white"));
	}

void TeXView::on_show()
	{
//	image.set(content->get_pixbuf());
	
	Gtk::EventBox::on_show();
	}

void TeXView::update_image()
	{
//	image.set(texbuf->get_pixbuf());
	}


