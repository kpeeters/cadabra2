
#include "GraphicsView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>
#include <gdk/gdkx.h>

using namespace cadabra;

// a=server.send("hello", "graphics_view", 0, 123, False)

static constexpr uint8_t RESOURCES_BAKEDCOLOR_DATA[] = {
#include "bakedcolor.inc"
};

GraphicsView::GraphicsView(filament::Engine *engine_)
	: glview(engine_), sizing(false), prev_x(0), prev_y(0), height_at_press(0), width_at_press(0)
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

GraphicsView::GLView::GLView(filament::Engine *engine_)
	: engine(engine_)
	{
	}


struct Vertex {
    filament::math::float2 position;
    uint32_t color;
};

static const Vertex TRIANGLE_VERTICES[3] = {
    {{1, 0}, 0xffff0000u},
    {{cos(M_PI * 2 / 3), sin(M_PI * 2 / 3)}, 0xff00ff00u},
    {{cos(M_PI * 4 / 3), sin(M_PI * 4 / 3)}, 0xff0000ffu},
};

static constexpr uint16_t TRIANGLE_INDICES[3] = { 0, 1, 2 };

using utils::Entity;
using utils::EntityManager;

void GraphicsView::GLView::first_render()
	{
	// Setup swapchain.
	auto gdk_window = get_window();
	if(!gdk_window || !GDK_IS_X11_WINDOW(gdk_window->gobj()))
		throw std::logic_error("GraphicsView::GLView::first_render: not inside an X11 window?");
	
	Window x11_window_id = gdk_x11_window_get_xid(gdk_window->gobj());
	
	swapChain = engine->createSwapChain((void*)(uintptr_t)x11_window_id, 
													filament::SwapChain::CONFIG_TRANSPARENT);

	// Setup buffers.
	vb = filament::VertexBuffer::Builder()
		.vertexCount(3)
		.bufferCount(1)
		.attribute(filament::VertexAttribute::POSITION, 0, filament::VertexBuffer::AttributeType::FLOAT2, 0, 12)
		.attribute(filament::VertexAttribute::COLOR, 0, filament::VertexBuffer::AttributeType::UBYTE4, 8, 12)
		.normalized(filament::VertexAttribute::COLOR)
		.build(*engine);
	vb->setBufferAt(*engine, 0,
						 filament::VertexBuffer::BufferDescriptor(TRIANGLE_VERTICES, 36, nullptr));
	ib = filament::IndexBuffer::Builder()
		.indexCount(3)
		.bufferType(filament::IndexBuffer::IndexType::USHORT)
		.build(*engine);
	ib->setBuffer(*engine,
					  filament::IndexBuffer::BufferDescriptor(TRIANGLE_INDICES, 6, nullptr));
	mat = filament::Material::Builder()
		.package(RESOURCES_BAKEDCOLOR_DATA, sizeof(RESOURCES_BAKEDCOLOR_DATA))
		.build(*engine);
	renderable = utils::EntityManager::get().create();
	filament::RenderableManager::Builder(1)
		.boundingBox({{ -1, -1, -1 }, { 1, 1, 1 }})
		.material(0, mat->getDefaultInstance())
		.geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0, 3)
		.culling(false)
		.receiveShadows(false)
		.castShadows(false)
		.build(*engine, renderable);

	scene = engine->createScene();
	view = engine->createView();
	scene->addEntity(renderable);
	camera = utils::EntityManager::get().create();
	cam = engine->createCamera(camera);
	view->setCamera(cam);
	view->setScene(scene);

	renderer = engine->createRenderer();
	filament::Renderer::ClearOptions co;
	co.clear=true;
	co.discard=true;
	co.clearColor=filament::math::float4({1.0,1.0,0.0,0.5});
	renderer->setClearOptions(co);
	}

//bool GraphicsView::GLView::on_render(const Glib::RefPtr< Gdk::GLContext > &context)
bool GraphicsView::GLView::on_draw(const Cairo::RefPtr<Cairo::Context>& context)
	{
	static bool first=true;
	
	int a, b;
	// context->get_version(a, b);
	std::cerr << "GLView::on_render " << std::endl;

	if(first) {
		first=false;
		std::cerr << "first render" << std::endl;
		first_render();
		}
	
	if(renderer->beginFrame(swapChain, 0) || true /* always render */) {
		std::cerr << "filament rendering" << std::endl;
		renderer->render(view);
		renderer->endFrame();
		}
	else {
		std::cerr << "filament skipped" << std::endl;
		}

	if(renderer->beginFrame(swapChain, 0) || true /* always render */) {
		std::cerr << "filament rendering pass 2" << std::endl;
		renderer->render(view);
		renderer->endFrame();
		}
	if(renderer->beginFrame(swapChain, 0) || true /* always render */) {
		std::cerr << "filament rendering pass 3" << std::endl;
		renderer->render(view);
		renderer->endFrame();
		}
	
	
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
