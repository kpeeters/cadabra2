
#include "GraphicsView.hh"
#include <giomm/memoryinputstream.h>
#include <glibmm/base64.h>
#include <iostream>
#include <fstream>
#include <gdk/gdkx.h>
#include <filament/FilamentAPI.h>
#include <filament/Engine.h>
#include <filament/Viewport.h>
#include <filament/ColorGrading.h>
#include <gltfio/AssetLoader.h>
#include <gltfio/ResourceLoader.h>
#include <filament/LightManager.h>
#include <filament/Texture.h>
#include <filament/IndirectLight.h>
 
using namespace cadabra;

// LIBGL_ALWAYS_SOFTWARE=true cadabra2-gtk
// a=server.send("hello", "graphics_view", 0, 123, False)
// https://github.com/nitronoid/qt_filament/

static constexpr uint8_t RESOURCES_BAKEDCOLOR_DATA[] = {
#include "bakedcolor.inc"
};

GraphicsView::GraphicsView(filament::Engine *engine_, filament::Renderer *renderer_)
	: glview(engine_, renderer_), sizing(false), prev_x(0), prev_y(0), height_at_press(0), width_at_press(0)
	{
	add(vbox);
	vbox.add(glview);
	set_events(Gdk::ENTER_NOTIFY_MASK
	           | Gdk::LEAVE_NOTIFY_MASK
	           | Gdk::BUTTON_PRESS_MASK
	           | Gdk::BUTTON_RELEASE_MASK
	           | Gdk::POINTER_MOTION_MASK);

	set_name("GraphicsView"); // to be able to style it with CSS
	set_size_request( 400, 600 );
	show_all();
	}

void GraphicsView::set_gltf(const std::string& str)
	{
	glview.set_gltf(str);
	}

GraphicsView::GLView::GLView(filament::Engine *engine_, filament::Renderer *renderer_)
	: engine(engine_), renderer(renderer_), need_setup_on_first_render(true), zoom(1.0)
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

void GraphicsView::GLView::set_gltf(const std::string& str)
	{
	gltf_str = 	Glib::Base64::decode(str);
	}

GraphicsView::GLView::~GLView()
	{
	std::cerr << "GLView going away, still implement! ****" << std::endl;
	}

void GraphicsView::GLView::first_render()
	{
	// Setup swapchain.
	auto gdk_window = get_window();
	if(!gdk_window || !GDK_IS_X11_WINDOW(gdk_window->gobj()))
		throw std::logic_error("GraphicsView::GLView::first_render: not inside an X11 window?");
	
	Window x11_window_id = gdk_x11_window_get_xid(gdk_window->gobj());

	if(engine==0)
		throw std::logic_error("GraphicsView: engine not initialised.");
	
	swapChain = engine->createSwapChain((void*)x11_window_id, 
													filament::SwapChain::CONFIG_TRANSPARENT);

	if(swapChain==0)
		throw std::logic_error("GraphicsView: cannot create swapchain.");

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
	if(scene==0)
		throw std::logic_error("GraphicsView: cannot create scene.");
	
	view = engine->createView();
	if(view==0)
		throw std::logic_error("GraphicsView: cannot create view.");
	
	view->setViewport(filament::Viewport(0, 0, 400, 400));
	camera = utils::EntityManager::get().create();
	cam = engine->createCamera(camera);
	view->setCamera(cam);
	view->setScene(scene);

	// Main sun light overhead.
	mainlight = utils::EntityManager::get().create();
	filament::LightManager::Builder(filament::LightManager::Type::DIRECTIONAL)
		.color(filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(1.0f, 1.0f, 1.0f)))
		.intensity(800000.0)
		.position({0.0, 10.0, 0.0})
		.direction({0.0, -1.0, 0.0})
		.castShadows(true)
		.build(*engine, mainlight);
	scene->addEntity(mainlight);

	// Spotlight for structure
	spotlight = utils::EntityManager::get().create();
	filament::LightManager::Builder(filament::LightManager::Type::SPOT)
		.color(filament::Color::toLinear<filament::ACCURATE>(filament::sRGBColor(1.0f, 1.0f, 1.0f)))
		.falloff(10)
		.intensity(100000000.0)
		.spotLightCone(0.001, 0.4)
		.position({5.0, 0, 5})
		.direction({-0.71, 0.0, -0.71})
		.castShadows(true)
		.build(*engine, spotlight);
	scene->addEntity(spotlight);

	// Ambient light using a white texture.
	static unsigned char oneWhitePixel[] = { 0xFF, 0xFF, 0xFF };
	filament::backend::PixelBufferDescriptor whitePixBuf( oneWhitePixel, sizeof(oneWhitePixel),
																			filament::backend::PixelDataFormat::RGB,
																			filament::backend::PixelDataType::UBYTE );
	static const filament::math::float3 irr[]={ filament::math::float3({1.0f, 1.0f, 1.0f}) };
	ambient_texture = filament::Texture::Builder()
		.width(1)
		.height(1)
		.levels(1)
		.sampler( filament::backend::SamplerType::SAMPLER_CUBEMAP )
		.format( filament::backend::TextureFormat::RGB8 )
		.build( *engine );
	filament::Texture::FaceOffsets offsets( 0 );
	ambient_texture->setImage( *engine, 0, std::move(whitePixBuf), offsets );
	ambient_light = filament::IndirectLight::Builder()
		.reflections(ambient_texture)
		.radiance(1, irr)
		.intensity(10000.0)
		.build(*engine);
	scene->setIndirectLight(ambient_light);

	
//	skybox = filament::Skybox::Builder().color({0.1, 0.125, 0.25, 1.0}).build(*engine);
	skybox = filament::Skybox::Builder().color({4.0, 4.0, 4.0, 1.0}).build(*engine);	
	scene->setSkybox(skybox);
	filament::PBRNeutralToneMapper tone_mapper;
	auto color_grading = filament::ColorGrading::Builder().toneMapper(&tone_mapper).build(*engine);
	view->setColorGrading(color_grading);
	
	view->setPostProcessingEnabled(true);
	filament::Renderer::ClearOptions co;
	co.clear=true;
	co.discard=true;
//	co.clearColor=filament::math::float4({1.0,1.0,0.0,0.5});
	co.clearColor=filament::math::float4({1.0,1.0,1.0,1.0});	
	renderer->setClearOptions(co);

	if(gltf_str.size()>0) {
		if(!load(gltf_str)) {
			std::cerr << "GraphicsView::GLView::first_render: adding test triangle." << std::endl;
			scene->addEntity(renderable);
			}
		}
	else {
		std::cerr << "GraphicsView::GLView::first_render: adding a test triangle." << std::endl;		
		scene->addEntity(renderable);
		}
	}

//bool GraphicsView::GLView::on_render(const Glib::RefPtr< Gdk::GLContext > &context)
bool GraphicsView::GLView::on_draw(const Cairo::RefPtr<Cairo::Context>& context)
	{
	int a, b;
	// context->get_version(a, b);
	// std::cerr << "GLView::on_render " << std::endl;

	if(need_setup_on_first_render) {
		need_setup_on_first_render=false;
		std::cerr << "first render" << std::endl;
		first_render();
		}

	// Always set camera, as its position may change from one frame
	// to the next.
	setup_camera();

	if(renderer->beginFrame(swapChain) || true /* always render */) {
		// std::cerr << "filament rendering" << std::endl;
		renderer->render(view);
		renderer->endFrame();
		}
	else {
		std::cerr << "filament skipped" << std::endl;
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

void GraphicsView::GLView::setup_camera()
	{
	// Get the width and height of our window, scaled by the pixel ratio
	const auto pixel_ratio = 1.0; // FIXME: devicePixelRatio();
	const uint32_t w = static_cast<uint32_t>(get_width() * pixel_ratio);
	const uint32_t h = static_cast<uint32_t>(get_height() * pixel_ratio);
	
	// Set our view-port size
	view->setViewport({0, 0, w, h});
	
	// setup view matrix
//	const filament::math::float3 eye(0.f, 0.f, 1.f);
	const filament::math::float3 eye(2.f, 2.0f*(std::cos(6.28*angle/1000.0)), 1.f);
	const filament::math::float3 target(0.f, 0.f, 0.f);
	const filament::math::float3 up(0.f, 1.f, 0.f);
	cam->lookAt(eye, target, up);
	
	// setup projection matrix
	const float aspect = float(w) / h;
	// std::cerr << "aspect = " << aspect << ", zoom = " << zoom << std::endl;
	// cam->setProjection(filament::Camera::Projection::PERSPECTIVE,
	// 							 -aspect * zoom,
	// 							 aspect * zoom,
	// 							 -zoom,
	// 							 zoom,
	// 							 0,
	// 							 100.0);
	cam->setProjection(80.0, // fov
							 aspect,
							 0.01,
							 100.0
							 );
//	cam->setExposure(1); //, 1.2, 100.0);
	}

bool GraphicsView::update(long timestamp_millis)
	{
	glview.angle = (glview.angle+1)%1000;
	return true;
	}

bool GraphicsView::GLView::load(const std::string& gltf)
	{
	auto materials = filament::gltfio::createJitShaderProvider(engine);
//	auto decoder = filament::gltfio::createStbProvider(engine);
	auto loader = filament::gltfio::AssetLoader::create({engine, materials});

	// Parse the glTF content and create Filament entities.
	filament::gltfio::FilamentAsset* asset = loader->createAsset((uint8_t *)gltf.data(), gltf.size());
	if(!asset) {
		std::cerr << "GraphicsView::GLView::load: failed to parse/convert gltf." << std::endl;
		return false;
		}
	
	// Load buffers and textures from disk.
	filament::gltfio::ResourceLoader resourceLoader({engine, ".", true});
//	resourceLoader.addTextureProvider("image/png", decoder);
//	resourceLoader.addTextureProvider("image/jpeg", decoder);
	resourceLoader.loadResources(asset);
	
	// Free the glTF hierarchy as it is no longer needed.
	asset->releaseSourceData();
	
	// Add renderables to the scene.
	scene->addEntities(asset->getEntities(), asset->getEntityCount());

	std::cerr << "GraphicsView::GLView::load: " << asset->getEntityCount() << " gltf entities added." << std::endl;
	return true;
	}
