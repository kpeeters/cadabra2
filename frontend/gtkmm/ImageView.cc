
#include "ImageView.hh"
#include "GtkCompat.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>

#include <gtkmm/image.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/window.h>
#include <gdkmm/general.h> // set_source_pixbuf()
#include <cairomm/surface.h>
#include <cairomm/context.h>
#include <gtkmm/filechooserdialog.h>

#include <iostream>
#include <fstream>

using namespace cadabra;

ImageView::ImageView(double display_scale_, int logical_width_)
	:
#ifdef USE_GTK4
	  Gtk::Box(cdb::ORIENT_VERTICAL),
#endif
	  area(logical_width_, display_scale_)
	, sizing(false)
	, prev_x(0)
	, prev_y(0)
	, height_at_press(0)
	, width_at_press(0)
	{
	area.set_halign(cdb::ALIGN_START);
	cdb::add_child(*this, area);

#ifndef USE_GTK4
	// GTK4 has no event masks; controllers subscribe to what they need.
	set_events(Gdk::ENTER_NOTIFY_MASK
	           | Gdk::LEAVE_NOTIFY_MASK
	           | Gdk::BUTTON_PRESS_MASK
	           | Gdk::BUTTON_RELEASE_MASK
	           | Gdk::POINTER_MOTION_MASK);
#endif

#ifdef USE_GTK4
	motion_controller = Gtk::EventControllerMotion::create();
	motion_controller->signal_motion().connect(
	   sigc::mem_fun(*this, &ImageView::on_pointer_motion));
	add_controller(motion_controller);
#endif

	click_gesture = cdb::add_click_gesture(*this);
	click_gesture->set_button(0);   // react to any button; we dispatch ourselves
	click_gesture->signal_pressed().connect(sigc::mem_fun(*this, &ImageView::on_button_pressed));
	click_gesture->signal_released().connect(sigc::mem_fun(*this, &ImageView::on_button_released));

	context_menu.attach_to(*this, "imageview");
	context_menu.add_item("Save as SVG...", sigc::mem_fun(*this, &ImageView::on_save_as));
	 
	set_name("ImageView"); // to be able to style it with CSS
	// Children are shown as they are added (see cdb::add_child), so this only
	// has to show the widget itself. show_all() is removed in GTK4.
	set_visible(true);
	}

ImageView::~ImageView()
	{
	}

void ImageView::on_save_as() const
	{
	Gtk::FileChooserDialog dialog("Please enter a file name for the SVG image",
	                              Gtk::FILE_CHOOSER_ACTION_SAVE);

//	dialog.set_transient_for(*this);
	dialog.set_do_overwrite_confirmation(true);
	dialog.add_button("_Cancel", cdb::RESPONSE_CANCEL);
	dialog.add_button("Select", cdb::RESPONSE_OK);

	int result=cdb::dialog_run(dialog);

	switch(result) {
		case(cdb::RESPONSE_OK): {
			std::string name = dialog.get_filename();
			std::ofstream temp(name);
			temp << area.decoded;
			}
		}
	}

/// Drag-to-resize. Shared between gtkmm-3's on_motion_notify_event vfunc and
/// GTK4's Gtk::EventControllerMotion, which gtkmm-3 does not wrap.

void ImageView::on_pointer_motion(double x, double /* y */)
	{
	if(sizing) {
		area.logical_width = width_at_press + (x - prev_x);
		area.rerender(area.logical_width);
		}
	}

#ifndef USE_GTK4
bool ImageView::on_motion_notify_event(GdkEventMotion *event)
	{
	on_pointer_motion(event->x, event->y);
	return true;
	}
#endif

void ImageView::on_button_pressed(int n_press, double x, double y)
	{
	// Only a single (not double/triple) press, matching the old
	// event->type==GDK_BUTTON_PRESS test.
	if(n_press != 1) return;

	const guint button = click_gesture->get_current_button();
	if(button == 1) {
		sizing=true;
		prev_x=x;
		prev_y=y;
		width_at_press=area.pixbuf->get_width()/area.display_scale;
		// std::cerr << "width_at_press = " << width_at_press << std::endl;
		height_at_press=area.pixbuf->get_height()/area.display_scale;
		}
	else if(button == 3) {
		context_menu.popup_at(x, y);
		}
	click_gesture->set_state(cdb::SEQ_CLAIMED);
	}

void ImageView::on_button_released(int /* n_press */, double /* x */, double /* y */)
	{
	sizing=false;
	click_gesture->set_state(cdb::SEQ_CLAIMED);
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
	str->add_data(decoded.c_str(), decoded.size(), nullptr);

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
	set_halign(cdb::ALIGN_CENTER);
	set_valign(cdb::ALIGN_CENTER);
#ifdef USE_GTK4
	// GTK4 removed the on_draw vfunc in favour of a draw callback.
	set_draw_func(sigc::mem_fun(*this, &ImageArea::draw));
#endif
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

void ImageArea::draw(const Cairo::RefPtr<Cairo::Context>& cr, int /* width */, int /* height */)
	{
	cr->scale(1.0/display_scale, 1.0/display_scale);
	Gdk::Cairo::set_source_pixbuf(cr, pixbuf, 0, 0);
	cr->paint();
	}

#ifndef USE_GTK4
bool ImageArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
	{
	draw(cr, get_allocated_width(), get_allocated_height());
	return true;
	}
#endif

