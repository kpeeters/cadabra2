
#include "GraphicsView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>

using namespace cadabra;

// a=server.send("hello", "graphics_view", 0, 123, False)

GraphicsView::GraphicsView()
	: sizing(false), prev_x(0), prev_y(0), height_at_press(0), width_at_press(0)
	{
	add(vbox);
	vbox.add(glview);
	set_events(Gdk::ENTER_NOTIFY_MASK
	           | Gdk::LEAVE_NOTIFY_MASK
	           | Gdk::BUTTON_PRESS_MASK
	           | Gdk::BUTTON_RELEASE_MASK
	           | Gdk::POINTER_MOTION_MASK);

	set_name("GraphicsView"); // to be able to style it with CSS
	set_size_request( 400, 400 );
	show_all();
	}

bool GraphicsView::GLView::on_render(const Glib::RefPtr< Gdk::GLContext > &context)
	{
	int a, b;
	context->get_version(a, b);
	std::cerr << "GLView::on_render " <<  a << ", " << b << std::endl;
	return true;
	}

GraphicsView::~GraphicsView()
	{
	}

bool GraphicsView::on_motion_notify_event(GdkEventMotion *event)
	{
	//	std::cerr << event->x << ", " << event->y << std::endl;
	if(sizing) {
		auto cw = width_at_press  + (event->x - prev_x);
		}
	return true;
	}

bool GraphicsView::on_button_press_event(GdkEventButton *event)
	{
	if(event->type==GDK_BUTTON_PRESS) {
		sizing=true;
		prev_x=event->x;
		prev_y=event->y;
		}
	return true;
	}

bool GraphicsView::on_button_release_event(GdkEventButton *event)
	{
	if(event->type==GDK_BUTTON_RELEASE) {
		sizing=false;
		}

	return true;
	}
