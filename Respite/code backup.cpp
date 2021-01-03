#include <entt/entt.hpp>
#include <SDL.h>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <SDL_syswm.h>
#include <bx/math.h>
#include <bimg/bimg.h>
#include <random>
#include "loadMesh.h"
#include "Animation.h"
#include "Model.h"
#include "terrain.h"
#include "actor.h"
#include "ActorComponets.h"

#include "ShaderLoader.h"

#include "GenerateMap.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const int WIDTH = 1024;
const int HEIGHT = 768;

#define ID_DIM 8

//object occlusion
//
//draw a rectangle behind the controlled character, the same height and width of the character
//
//get the near plane co-ords of this rect.
//
//create bounding box from the near and far rect, then test this agains the other object boudning boxes to determine which objects to dissolve.


static void* sdlNativeWindowHandle(SDL_Window* _window)
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(_window, &wmi))
	{
		return NULL;
	}

#	if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#		if ENTRY_CONFIG_USE_WAYLAND
	wl_egl_window* win_impl = (wl_egl_window*)SDL_GetWindowData(_window, "wl_egl_window");
	if (!win_impl)
	{
		int width, height;
		SDL_GetWindowSize(_window, &width, &height);
		struct wl_surface* surface = wmi.info.wl.surface;
		if (!surface)
			return nullptr;
		win_impl = wl_egl_window_create(surface, width, height);
		SDL_SetWindowData(_window, "wl_egl_window", win_impl);
	}
	return (void*)(uintptr_t)win_impl;
#		else
	return (void*)wmi.info.x11.window;
#		endif
#	elif BX_PLATFORM_OSX
	return wmi.info.cocoa.window;
#	elif BX_PLATFORM_WINDOWS
	return wmi.info.win.window;
#	elif BX_PLATFORM_STEAMLINK
	return wmi.info.vivante.window;
#	endif // BX_PLATFORM_
}

inline bool sdlSetWindow(SDL_Window* _window)
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(_window, &wmi))
	{
		return false;
	}

	bgfx::PlatformData pd;
#	if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#		if ENTRY_CONFIG_USE_WAYLAND
	pd.ndt = wmi.info.wl.display;
#		else
	pd.ndt = wmi.info.x11.display;
#		endif
#	elif BX_PLATFORM_OSX
	pd.ndt = NULL;
#	elif BX_PLATFORM_WINDOWS
	pd.ndt = NULL;
#	elif BX_PLATFORM_STEAMLINK
	pd.ndt = wmi.info.vivante.display;
#	endif // BX_PLATFORM_
	pd.nwh = sdlNativeWindowHandle(_window);

	pd.context = NULL;
	pd.backBuffer = NULL;
	pd.backBufferDS = NULL;
	bgfx::setPlatformData(pd);

	return true;
}





bool intersectPlane(const bx::Vec3& n, const bx::Vec3& p0, const bx::Vec3& l0, const bx::Vec3& l, float& t)
{
	// assuming vectors are all normalized
	float denom = bx::dot(l, n);
	if (denom > 1e-6) {
		bx::Vec3 p0l0 = bx::sub(p0, l0);
		t = bx::dot(p0l0, n) / denom;
		return (t >= 0);
	}

	return false;
}

void updateactor(actor& actor, double dT)
{
	float theta = bx::atan2(actor.pos.x - actor.target.x, actor.pos.y - actor.target.y);
	if (theta < 0.0)
		theta += bx::kPi2;
	//update heading
	actor.heading = theta;
	bx::Vec3 difference = bx::sub(actor.target, actor.pos);
	if (bx::distance(difference, bx::Vec3(0, 0, 0)))
	{
		actor.pos = bx::add((bx::mul(bx::normalize(difference), actor.speed)), actor.pos);
	}
}



void update(std::uint64_t dt, entt::registry& registry)
{

}

// Vertex layout for our screen space quad (used in deferred rendering)
struct PosTexCoord0Vertex
{
	float m_x;
	float m_y;
	float m_z;
	float m_u;
	float m_v;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	}

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosTexCoord0Vertex::ms_decl;

// Utility function to draw a screen space quad for deferred rendering
void screenSpaceQuad(float _textureWidth, float _textureHeight, float _texelHalf, bool _originBottomLeft, float _width = 1.0f, float _height = 1.0f)
{
	if (3 == bgfx::getAvailTransientVertexBuffer(3, PosTexCoord0Vertex::ms_decl))
	{
		bgfx::TransientVertexBuffer vb;
		bgfx::allocTransientVertexBuffer(&vb, 3, PosTexCoord0Vertex::ms_decl);
		PosTexCoord0Vertex* vertex = (PosTexCoord0Vertex*)vb.data;

		const float minx = -_width;
		const float maxx = _width;
		const float miny = 0.0f;
		const float maxy = _height * 2.0f;

		const float texelHalfW = _texelHalf / _textureWidth;
		const float texelHalfH = _texelHalf / _textureHeight;
		const float minu = -1.0f + texelHalfW;
		const float maxu = 1.0f + texelHalfH;

		const float zz = 0.0f;

		float minv = texelHalfH;
		float maxv = 2.0f + texelHalfH;

		if (_originBottomLeft)
		{
			float temp = minv;
			minv = maxv;
			maxv = temp;

			minv -= 1.0f;
			maxv -= 1.0f;
		}

		vertex[0].m_x = minx;
		vertex[0].m_y = miny;
		vertex[0].m_z = zz;
		vertex[0].m_u = minu;
		vertex[0].m_v = minv;

		vertex[1].m_x = maxx;
		vertex[1].m_y = miny;
		vertex[1].m_z = zz;
		vertex[1].m_u = maxu;
		vertex[1].m_v = minv;

		vertex[2].m_x = maxx;
		vertex[2].m_y = maxy;
		vertex[2].m_z = zz;
		vertex[2].m_u = maxu;
		vertex[2].m_v = maxv;

		bgfx::setVertexBuffer(0, &vb);
	}
}

struct RenResourceHandles {

	bgfx::ProgramHandle BasicProgram;

	uint16_t  m_shadowMapSize;
	bgfx::ProgramHandle ShadowProgram;
	bgfx::UniformHandle TexColorUniform;
	bgfx::UniformHandle s_shadowMap;
	bgfx::UniformHandle u_lightPos;
	bgfx::UniformHandle u_lightMtx;
	bgfx::UniformHandle u_depthScaleOffset;
	bgfx::TextureHandle shadowMapTexture;
	bgfx::FrameBufferHandle ShadowMapFB;


	bgfx::ProgramHandle SSAO_Program;
	bgfx::TextureHandle SSAO_GbufferTexture[4];
	bgfx::UniformHandle SSAO_Uniform[4];
	bgfx::FrameBufferHandle SSAOFB;
	bgfx::UniformHandle SSAO_KernalSamples;

	bgfx::UniformHandle SSAO_ProjMatrix;

	bgfx::UniformHandle SSAO_Noise;
	bgfx::TextureHandle SSAO_Noise_Texture;

	const bgfx::Memory* ssao_sampleKernal;


	bgfx::UniformHandle SSAO_Result_Uniform;
	bgfx::TextureHandle SSAO_result;
	bgfx::FrameBufferHandle SSAO_result_FB;
	bgfx::ProgramHandle m_progssaoblurmerge;

};


struct camera {
	float* view; //[16] 4x4 matrix
	float* proj; //[16] 4x4 matrix
};

struct screen {
	unsigned int WIDTH;
	unsigned int HEIGHT;
};



float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}


void loadRenderResources(RenResourceHandles& ResourceHandles, screen screen)
{


	PosTexCoord0Vertex::init();

	ResourceHandles.m_shadowMapSize = 512;
	bgfx::TextureHandle fbtextures[] =
	{
		bgfx::createTexture2D(
			  ResourceHandles.m_shadowMapSize
			, ResourceHandles.m_shadowMapSize
			, false
			, 1
			, bgfx::TextureFormat::D16
			, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL
			),
	};
	ResourceHandles.shadowMapTexture = fbtextures[0];
	ResourceHandles.ShadowMapFB = bgfx::createFrameBuffer(BX_COUNTOF(fbtextures), fbtextures, true);
	ResourceHandles.u_lightPos = bgfx::createUniform("u_lightPos", bgfx::UniformType::Vec4);
	ResourceHandles.u_lightMtx = bgfx::createUniform("u_lightMtx", bgfx::UniformType::Mat4);
	ResourceHandles.u_depthScaleOffset = bgfx::createUniform("u_depthScaleOffset", bgfx::UniformType::Vec4);
	ResourceHandles.s_shadowMap = bgfx::createUniform("s_shadowMap", bgfx::UniformType::Sampler);
	// Get renderer capabilities info.
	const bgfx::Caps* caps = bgfx::getCaps();

	float depthScaleOffset[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
	if (caps->homogeneousDepth)
	{
		depthScaleOffset[0] = 0.5f;
		depthScaleOffset[1] = 0.5f;
	}
	bgfx::setUniform(ResourceHandles.u_depthScaleOffset, depthScaleOffset);


	//SSAO




	const uint64_t tsFlags = 0
		| BGFX_SAMPLER_MIN_POINT
		| BGFX_SAMPLER_MAG_POINT
		| BGFX_SAMPLER_MIP_POINT
		| BGFX_SAMPLER_U_CLAMP
		| BGFX_SAMPLER_V_CLAMP
		;

	ResourceHandles.SSAO_GbufferTexture[0] = bgfx::createTexture2D(uint16_t(screen.WIDTH), uint16_t(screen.HEIGHT), false, 1, bgfx::TextureFormat::RGBA32F, BGFX_TEXTURE_RT | BGFX_SAMPLER_U_CLAMP
		| BGFX_SAMPLER_V_CLAMP | 0); //position	
	ResourceHandles.SSAO_GbufferTexture[1] = bgfx::createTexture2D(uint16_t(screen.WIDTH), uint16_t(screen.HEIGHT), false, 1, bgfx::TextureFormat::RGBA16F, BGFX_TEXTURE_RT | tsFlags); //normal
	ResourceHandles.SSAO_GbufferTexture[2] = bgfx::createTexture2D(uint16_t(screen.WIDTH), uint16_t(screen.HEIGHT), false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | tsFlags); //colour
	ResourceHandles.SSAO_GbufferTexture[3] = bgfx::createTexture2D(uint16_t(screen.WIDTH), uint16_t(screen.HEIGHT), false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT | BGFX_SAMPLER_COMPARE_LEQUAL); //depth


	bgfx::TextureHandle fbtex_ssao_result[] =
	{
		bgfx::createTexture2D(uint16_t(screen.WIDTH), uint16_t(screen.HEIGHT), false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT | tsFlags),
	};

	ResourceHandles.SSAO_result = fbtex_ssao_result[0];

	bgfx::Attachment gbufferAt[4];
	for (size_t i = 0; i < 4; i++)
	{
		gbufferAt[i].init(ResourceHandles.SSAO_GbufferTexture[i]);
	}


	ResourceHandles.SSAO_result_FB = bgfx::createFrameBuffer(1, fbtex_ssao_result,true);
	ResourceHandles.SSAOFB = bgfx::createFrameBuffer(BX_COUNTOF(gbufferAt), gbufferAt, true);
	ResourceHandles.SSAO_Result_Uniform = bgfx::createUniform("SSAO_result", bgfx::UniformType::Sampler);


	ResourceHandles.SSAO_Uniform[0] = bgfx::createUniform("SSAO_position", bgfx::UniformType::Sampler);
	ResourceHandles.SSAO_Uniform[1] = bgfx::createUniform("SSAO_normal", bgfx::UniformType::Sampler);
	ResourceHandles.SSAO_Uniform[2] = bgfx::createUniform("SSAO_color", bgfx::UniformType::Sampler);
	ResourceHandles.SSAO_Uniform[3] = bgfx::createUniform("SSAO_depth", bgfx::UniformType::Sampler);



	struct Float4 {
		Float4(float _x, float _y, float _z, float _w) {
			x = _x;
			y = _y;
			z = _z;
			w = _w;
		}
		float x, y, z, w;
	};

	// generate sample kernel
	// ----------------------
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
	std::default_random_engine generator;
	std::vector<bx::Vec3> ssaoKernel;
	std::vector< Float4> ssaoSampleKernel;
	for (unsigned int i = 0; i < 64; ++i)
	{
		bx::Vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, randomFloats(generator));
		sample = bx::normalize(sample);
		sample = bx::mul(sample, randomFloats(generator));
		float scale = float(i) / 64.0;

		// scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample = bx::mul(sample, scale);

		
		ssaoSampleKernel.push_back(Float4(sample.x, sample.y, sample.z,0.0));
		ssaoKernel.push_back(sample);
	}


	ResourceHandles.SSAO_KernalSamples = bgfx::createUniform("SSAO_Sample_Kernal", bgfx::UniformType::Vec4, 64);
	size_t KernalSize = ssaoSampleKernel.size() * sizeof(Float4);
	ResourceHandles.ssao_sampleKernal = bgfx::copy(ssaoSampleKernel.data(), KernalSize);

	// generate noise texture
	// ----------------------



	std::vector<Float4> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		Float4 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}
	size_t NoiseSize = ssaoNoise.size() * sizeof(Float4);
	const bgfx::Memory* NoiseImage = bgfx::copy(ssaoNoise.data(), NoiseSize);

	ResourceHandles.SSAO_Noise = bgfx::createUniform("SSAO_Noise", bgfx::UniformType::Sampler);
	ResourceHandles.SSAO_Noise_Texture = bgfx::createTexture2D(4, 4, false, 1, bgfx::TextureFormat::RGBA32F, 0 | BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, NoiseImage);

	ResourceHandles.SSAO_ProjMatrix = bgfx::createUniform("SSAO_ProjMatrix", bgfx::UniformType::Mat4);

}

void renderFrame(camera cam, screen screen, entt::registry& entitiesRegistry, RenResourceHandles ResourceHandles)
{
	uint16_t RENDER_SHADOW_PASS_ID = 0;
	uint16_t RENDER_SCENE_PASS_ID = 1;
	uint16_t RENDER_SSAO_PASS_ID = 2;
	uint16_t RENDER_SSAO_BLURMERGE_ID = 3;

	// Setup lights.
	float lightPos[4];
	lightPos[0] = 0.0f;
	lightPos[1] = 150.0f;
	lightPos[2] = -200.0f;
	lightPos[3] = 0.0f;

	bgfx::setUniform(ResourceHandles.u_lightPos, lightPos);

	// Define matrices.
	float lightView[16];
	float lightProj[16];

	const bx::Vec3 at = { 0.0f,  0.0f,   0.0f };
	const bx::Vec3 eye = { -lightPos[0], -lightPos[1], -lightPos[2] };
	bx::mtxLookAt(lightView, eye, at);

	const bgfx::Caps* caps = bgfx::getCaps();
	const float area = 270.0f;
	bx::mtxOrtho(lightProj, -area, area, -area, area, -100.0f, 1000.0f, 0.0f, caps->homogeneousDepth);

	bool m_shadowSamplerSupported = 0 != (caps->supported & BGFX_CAPS_TEXTURE_COMPARE_LEQUAL);

	bgfx::setViewRect(RENDER_SHADOW_PASS_ID, 0, 0, ResourceHandles.m_shadowMapSize, ResourceHandles.m_shadowMapSize);
	bgfx::setViewFrameBuffer(RENDER_SHADOW_PASS_ID, ResourceHandles.ShadowMapFB);
	bgfx::setViewTransform(RENDER_SHADOW_PASS_ID, lightView, lightProj);

	bgfx::setViewRect(RENDER_SCENE_PASS_ID, 0, 0, screen.WIDTH, screen.HEIGHT);
	bgfx::setViewTransform(RENDER_SCENE_PASS_ID, cam.view, cam.proj);
	bgfx::setViewFrameBuffer(RENDER_SCENE_PASS_ID, ResourceHandles.SSAOFB);
	//bgfx::setViewTransform(RENDER_SCENE_PASS_ID, lightView, lightProj);

	bgfx::setViewRect(RENDER_SSAO_PASS_ID, 0, 0, screen.WIDTH, screen.HEIGHT);
	float ss_proj[16];
	bx::mtxOrtho(ss_proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, caps->homogeneousDepth);
	bgfx::setViewFrameBuffer(RENDER_SSAO_PASS_ID, ResourceHandles.SSAO_result_FB);
	bgfx::setViewTransform(RENDER_SSAO_PASS_ID, NULL, ss_proj);

	bgfx::setViewRect(RENDER_SSAO_BLURMERGE_ID, 0, 0, screen.WIDTH, screen.HEIGHT);
	bgfx::setViewTransform(RENDER_SSAO_BLURMERGE_ID, NULL, ss_proj);


	// Clear backbuffer and shadowmap framebuffer at beginning.
	bgfx::setViewClear(RENDER_SHADOW_PASS_ID
		, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		, 0x303030ff, 1.0f, 0
	);

	bgfx::setViewClear(RENDER_SCENE_PASS_ID
		, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		, 0x000000FF, 1.0f, 0
	);

	bgfx::setViewClear(RENDER_SSAO_PASS_ID
		, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		, 0x443355FF, 1.0f, 0
	);

	bgfx::setViewClear(RENDER_SSAO_BLURMERGE_ID
		, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		, 0x443355FF, 1.0f, 0
	);


	//render

	float mtxShadow[16];
	float lightMtx[16];

	const float sy = caps->originBottomLeft ? 0.5f : -0.5f;
	const float sz = caps->homogeneousDepth ? 0.5f : 1.0f;
	const float tz = caps->homogeneousDepth ? 0.5f : 0.0f;
	const float mtxCrop[16] =
	{
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f,   sy, 0.0f, 0.0f,
		0.0f, 0.0f, sz,   0.0f,
		0.5f, 0.5f, tz,   1.0f,
	};

	float mtxTmp[16];
	bx::mtxMul(mtxTmp, lightProj, mtxCrop);
	bx::mtxMul(mtxShadow, lightView, mtxTmp);



	auto staticModelView = entitiesRegistry.view<Model>();

	//shadows pass
	for (auto entity : staticModelView) {
		auto& model = staticModelView.get<Model>(entity);

		bx::mtxMul(lightMtx, model.matrixTransform, mtxShadow);
		bgfx::setUniform(ResourceHandles.u_lightMtx, lightMtx);
		// Set model matrix for rendering.
		bgfx::setTransform(model.matrixTransform);

		// Set vertex and index buffer.
		bgfx::setVertexBuffer(0, model.vbh);
		//bgfx::setIndexBuffer(model.ibh);

		unsigned long long state = 0
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_MSAA
			| BGFX_STATE_CULL_CW
			;


		// Set render states.
		bgfx::setState(state);

		// Submit primitive for rendering to view 0.
		bgfx::submit(RENDER_SHADOW_PASS_ID, ResourceHandles.ShadowProgram);

	}


	for (auto entity : staticModelView) {
		auto& model = staticModelView.get<Model>(entity);

		bx::mtxMul(lightMtx, model.matrixTransform, mtxShadow);
		bgfx::setUniform(ResourceHandles.u_lightMtx, lightMtx);
		// Set model matrix for rendering.
		bgfx::setTransform(model.matrixTransform);

		// Set vertex and index buffer.
		bgfx::setVertexBuffer(0, model.vbh);
		//bgfx::setIndexBuffer(model.ibh);

		bgfx::setTexture(0, ResourceHandles.s_shadowMap, ResourceHandles.shadowMapTexture);

		bgfx::setTexture(1, ResourceHandles.TexColorUniform, model.texh);

		unsigned long long state = 0
			| BGFX_STATE_WRITE_RGB
			| BGFX_STATE_WRITE_A
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_MSAA
			| BGFX_STATE_CULL_CCW
			;

		// Set render states.
		bgfx::setState(state);

		// Submit primitive for rendering to view 0.
		bgfx::submit(RENDER_SCENE_PASS_ID, ResourceHandles.BasicProgram);

	}



	//bgfx::setTexture(0, s_albedo, m_gbufferTex[0]);
	//for (size_t i = 0; i < 4; i++)
	//{
	//	bgfx::setTexture(i, ResourceHandles.SSAO_Uniform[i], ResourceHandles.SSAO_GbufferTexture[i]);
	//}

	bgfx::setTexture(0, ResourceHandles.SSAO_Uniform[0], ResourceHandles.SSAO_GbufferTexture[0]);
	bgfx::setTexture(1, ResourceHandles.SSAO_Uniform[1], ResourceHandles.SSAO_GbufferTexture[1]);
	bgfx::setTexture(2, ResourceHandles.SSAO_Noise, ResourceHandles.SSAO_Noise_Texture);



	float m_proj[16];
	for (size_t i = 0; i < 16; i++)
	{
		m_proj[i] = (float)i * 0.1;
	}
	bgfx::UniformHandle ProjMat= bgfx::createUniform("SSAO_ProjMatrix", bgfx::UniformType::Mat4);
	bgfx::setUniform(ProjMat, cam.proj);
	bgfx::setUniform(ResourceHandles.SSAO_KernalSamples, ResourceHandles.ssao_sampleKernal->data, 64);
	bgfx::setState(0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
	);
	screenSpaceQuad((float)screen.WIDTH, (float)screen.HEIGHT, 0.0f, caps->originBottomLeft);
	bgfx::submit(RENDER_SSAO_PASS_ID, ResourceHandles.SSAO_Program);



	bgfx::setTexture(0, ResourceHandles.SSAO_Result_Uniform, ResourceHandles.SSAO_result);
	bgfx::setTexture(1, ResourceHandles.SSAO_Uniform[2], ResourceHandles.SSAO_GbufferTexture[2]);

	bgfx::setState(0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
	);
	screenSpaceQuad((float)screen.WIDTH, (float)screen.HEIGHT, 0.0f, caps->originBottomLeft);
	bgfx::submit(RENDER_SSAO_BLURMERGE_ID, ResourceHandles.m_progssaoblurmerge);



	//float mtx[16];
	//bx::mtxRotateZ(mtx, 0.0f);

	//// position x,y,z
	//mtx[12] = 0;
	//mtx[13] = 0;
	//mtx[14] = 0.0f;

	//// Set model matrix for rendering.
	//bgfx::setTransform(mtx);

	//// Set vertex and index buffer.
	//bgfx::setVertexBuffer(0, terrain_vbh);
	////bgfx::setIndexBuffer(RenderModels[0].ibh);

	//bgfx::setTexture(0, s_texColor, ground_tex);

	//unsigned long long state = 0
	//	| BGFX_STATE_WRITE_RGB
	//	| BGFX_STATE_WRITE_A
	//	| BGFX_STATE_WRITE_Z
	//	| BGFX_STATE_DEPTH_TEST_LESS
	//	| BGFX_STATE_MSAA
	//	| BGFX_STATE_BLEND_ALPHA;


	//// Set render states.
	//bgfx::setState(state);

	//// Submit primitive for rendering to view 0.
	//bgfx::submit(0, m_program);









	//auto staticModelView = registry.view<Position, Model>();

	//for (auto entity : staticModelView) {
	//	// a component at a time ...
	//	auto& position = staticModelView.get<Position>(entity);
	//	auto& model = staticModelView.get<Model>(entity);

	//	{
	//		float mtx[16];
	//		bx::mtxRotateZ(mtx, 0);

	//		// position x,y,z
	//		mtx[12] = position.position.x;
	//		mtx[13] = position.position.y;
	//		mtx[14] = 10 * position.position.z;

	//		float scalemtx[16];
	//		bx::mtxScale(scalemtx, position.position.z * 2.2);

	//		//bx::mtxRotateZ(mtx, bx::kPi);
	//		bx::mtxMul(mtx, scalemtx, mtx);

	//		//bx::mtxRotateX(mtx, bx::kPi);
	//		//bx::mtxMul(r_mtx, mtx, r_mtx);

	//		// Set model matrix for rendering.
	//		bgfx::setTransform(mtx);

	//		// Set vertex and index buffer.
	//		bgfx::setVertexBuffer(0, model.vbh);
	//		bgfx::setIndexBuffer(model.ibh);

	//		bgfx::setTexture(0, s_texColor, Tree_tex);

	//		unsigned long long state = 0
	//			| BGFX_STATE_WRITE_RGB
	//			| BGFX_STATE_WRITE_A
	//			| BGFX_STATE_WRITE_Z
	//			| BGFX_STATE_DEPTH_TEST_LESS
	//			| BGFX_STATE_MSAA;


	//		// Set render states.
	//		bgfx::setState(state);

	//		// Submit primitive for rendering to view 0.
	//		bgfx::submit(0, m_program);
	//	}


	//}

	//{
	//	float mtx[16];
	//	bx::mtxRotateZ(mtx, testplayer.heading);

	//	// position x,y,z
	//	mtx[12] = testplayer.pos.x;
	//	mtx[13] = testplayer.pos.y;
	//	mtx[14] = testplayer.pos.z;

	//	float r_mtx[16];
	//	bx::mtxMul(r_mtx, hand.matrix, mtx);

	//	//bx::mtxRotateZ(mtx, bx::kPi);
	//	//bx::mtxMul(r_mtx, mtx, r_mtx);

	//	//bx::mtxRotateX(mtx, bx::kPi);
	//	//bx::mtxMul(r_mtx, mtx, r_mtx);

	//	// Set model matrix for rendering.
	//	bgfx::setTransform(r_mtx);

	//	// Set vertex and index buffer.
	//	bgfx::setVertexBuffer(0, Axe.vbh);
	//	bgfx::setIndexBuffer(Axe.ibh);

	//	bgfx::setTexture(0, s_texColor, axe_tex);

	//	unsigned long long state = 0
	//		| BGFX_STATE_WRITE_RGB
	//		| BGFX_STATE_WRITE_A
	//		| BGFX_STATE_WRITE_Z
	//		| BGFX_STATE_DEPTH_TEST_LESS
	//		| BGFX_STATE_MSAA;


	//	// Set render states.
	//	bgfx::setState(state);

	//	// Submit primitive for rendering to view 0.
	//	bgfx::submit(0, m_program);
	//}


	//{
	//	float mtx[16];
	//	bx::mtxRotateZ(mtx, testplayer.heading);

	//	// position x,y,z
	//	mtx[12] = testplayer.pos.x;
	//	mtx[13] = testplayer.pos.y;
	//	mtx[14] = testplayer.pos.z;

	//	// Set model matrix for rendering.
	//	bgfx::setTransform(mtx);

	//	// Set vertex and index buffer.

	//	bgfx::setVertexBuffer(0, vbh);
	//	bgfx::setIndexBuffer(RenderModels[1].ibh);

	//	bgfx::setTexture(0, s_texColor, RenderModels[1].texh);

	//	unsigned long long state = 0
	//		| BGFX_STATE_WRITE_RGB
	//		| BGFX_STATE_WRITE_A
	//		| BGFX_STATE_WRITE_Z
	//		| BGFX_STATE_DEPTH_TEST_LESS
	//		| BGFX_STATE_MSAA;


	//	// Set render states.
	//	bgfx::setState(state);

	//	// Submit primitive for rendering to view 0.
	//	bgfx::submit(0, m_program);
	//}



	bgfx::frame();
}


void* load(const char* filename, size_t& size) {
	/// Opens a file and returns a bgfx::Memory of the raw data. The lifetime of the data is controlled by bgfx
	std::ifstream fs(filename, std::ios::in | std::ios::binary);
	if (!fs.is_open()) {
		return NULL;
	}
	fs.seekg(0, std::ios::end);
	const size_t LEN = fs.tellg();
	fs.seekg(0, std::ios::beg);
	size = LEN;
	void* mem = malloc(LEN);
	fs.read((char*)mem, LEN);
	fs.close();
	return mem;
}

const bgfx::Memory* readTexture(const char* filename) {
	/// Opens a file and returns a bgfx::Memory of the raw data. The lifetime of the data is controlled by bgfx
	std::ifstream fs(filename, std::ios::in | std::ios::binary);
	if (!fs.is_open()) {
		return NULL;
	}
	fs.seekg(0, std::ios::end);
	const size_t LEN = fs.tellg();
	fs.seekg(0, std::ios::beg);

	const bgfx::Memory* mem = bgfx::alloc(LEN);
	fs.read((char*)mem->data, LEN);
	fs.close();
	return mem;
}


int main(int argc, char* argv[])
{


	VertexData* terrain = generate_terrain(26, 26, 20);





	//generate map

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;




	GameMesh testMesh = loadmesh("knight.mesh");
	GameMesh axe = loadmesh("axe.mesh");
	GameMesh testMeshold = loadmesh("knightold.mesh");
	GameMesh pinetree = loadmesh("pinetree.mesh");
	//load tree mesh


	unsigned int size = testMesh.pos.size();
	VertexData* test_verticies = new VertexData[size];
	m_pose unused;
	rebuild_vbuff(test_verticies, testMesh, 0, unused);

	SDL_Init(0);
	SDL_Window* window = SDL_CreateWindow("Respite",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		WIDTH, HEIGHT, SDL_WINDOW_SHOWN);

	sdlSetWindow(window);

	// Render an empty frame
	bgfx::renderFrame();

	// Initialize bgfx
	bgfx::init();





	std::vector<Model> RenderModels;
	//RenderModels.push_back(LoadModel(groundMesh));
	RenderModels.push_back(LoadModel(testMesh));
	RenderModels.push_back(LoadModel(testMesh));

	bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
		// Static data can be passed with bgfx::makeRef
		bgfx::makeRef(test_verticies, sizeof(VertexData) * size),
		VertexData::ms_decl
	);

	Model Axe = LoadModel(axe);
	Model PineTree = LoadModel(pinetree);



	bgfx::VertexBufferHandle terrain_vbh = bgfx::createVertexBuffer(
		// Static data can be passed with bgfx::makeRef
		bgfx::makeRef(terrain, sizeof(VertexData) * (26 - 1) * (26 - 1) * 6),
		VertexData::ms_decl
	);

	size_t texsize;
	void* data = load("TreeTest.dds", texsize);

	bimg::ImageContainer imageContainer;
	bimg::imageParse(imageContainer, data, texsize);

	const bgfx::Memory* mem = bgfx::makeRef(
		imageContainer.m_data
		, imageContainer.m_size
	);

	PineTree.texh = bgfx::createTexture(readTexture("TreeTest.dds"));

	int x, y, n;
	unsigned char* image = stbi_load("Grass_7.png", &x, &y, &n, 0);
	const bgfx::Memory* mem_image = bgfx::makeRef(image, x * y * n);
	bgfx::TextureHandle ground_tex = bgfx::createTexture2D(x, y, false, 1, bgfx::TextureFormat::RGBA8, 0 | BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, mem_image);



	unsigned char* image_axe = stbi_load("Axe_tex.png", &x, &y, &n, 0);
	const bgfx::Memory* mem_image_axe = bgfx::makeRef(image_axe, x * y * n);
	bgfx::TextureHandle axe_tex = bgfx::createTexture2D(x, y, false, 1, bgfx::TextureFormat::RGBA8, 0 | BGFX_TEXTURE_RT
		| BGFX_SAMPLER_MIN_POINT
		| BGFX_SAMPLER_MAG_POINT
		| BGFX_SAMPLER_MIP_POINT
		| BGFX_SAMPLER_U_CLAMP
		| BGFX_SAMPLER_V_CLAMP, mem_image_axe);


	//unsigned char* image_Tree = stbi_load("TreeTexdark2.png", &x, &y, &n, 0);
	//const bgfx::Memory* mem_image_Tree = bgfx::makeRef(image_Tree, x * y * n);
	//bgfx::TextureHandle Tree_tex = bgfx::createTexture2D(x, y, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE, mem_image_Tree);



	bgfx::UniformHandle s_texColor = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);

	bgfx::ProgramHandle m_progShadow = loadProgram("vs_sms_shadow.bin", "fs_sms_shadow.bin");
	bgfx::ProgramHandle m_progMesh = loadProgram("vs_sms_mesh.bin", "fs_sms_mesh.bin");
	bgfx::ProgramHandle m_progssao = loadProgram("J:\\Users\\Ethan\\Documents\\bgfx\\bgfx\\scripts\\shaders\\dx11\\vs_ssao.bin", "J:\\Users\\Ethan\\Documents\\bgfx\\bgfx\\scripts\\shaders\\dx11\\fs_ssao_2.bin");
	bgfx::ProgramHandle m_progssaoblurmerge = loadProgram("J:\\Users\\Ethan\\Documents\\bgfx\\bgfx\\scripts\\shaders\\dx11\\vs_ssaoblurmerge.bin", "J:\\Users\\Ethan\\Documents\\bgfx\\bgfx\\scripts\\shaders\\dx11\\fs_ssaoblurmerge.bin");

	// Reset window
	bgfx::reset(WIDTH, HEIGHT, BGFX_RESET_VSYNC);

	// Enable debug text.
	bgfx::setDebug(BGFX_DEBUG_TEXT);//| BGFX_DEBUG_STATS);

	//// Set view rectangle for 0th view
	//bgfx::setViewRect(0, 0, 0, uint16_t(WIDTH), uint16_t(HEIGHT));

	//// Clear the view rect
	//bgfx::setViewClear(0,
	//	BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
	//	0x443355FF, 1.0f, 0);


	//// Set empty primitive on screen
	//bgfx::touch(0);

	//bgfx::frame();

	// Poll for events and wait till user closes window
	bool quit = false;
	SDL_Event currentEvent;
	const Uint8* state = SDL_GetKeyboardState(NULL);
	float z_rot = 0.0f;
	float x_trans = 0.0f;
	float y_trans = 0.0f;
	bx::Vec3 camera_heading = { 0.0f,0.0f,0.0f };
	float camera_height = 150.0f;
	float camera_distance = 150.0f;

	float animation_index = 0;

	Uint32 lastUpdate = SDL_GetTicks();
	Uint32 current = SDL_GetTicks();


	actor testplayer;
	testplayer.speed = 0.5;
	testplayer.pos.x = 0.0f;
	testplayer.pos.y = 0.0f;
	testplayer.pos.z = 3.0f;
	testplayer.heading = 0;

	testplayer.target.x = 0.0f;
	testplayer.target.y = 0.0f;
	testplayer.target.z = 3.0f;


	//-------------------------
	//Add Models to the registry
	//-------------------------

	//Trees
	entt::registry registry;
	std::vector<bx::Vec3> trees = buildmap(1, 500, 500);
	for (size_t i = 0; i < trees.size(); i++)
	{
		auto newTree = registry.create();
		registry.assign<Position>(newTree, trees[i]);

		float* mtx = new float[16];

		bx::mtxRotateZ(mtx, 0);

		// position x,y,z
		mtx[12] = trees[i].x - 250;
		mtx[13] = trees[i].y - 250;
		mtx[14] = 11 * trees[i].z;

		float scalemtx[16];
		bx::mtxScale(scalemtx, trees[i].z * 2.2);

		//bx::mtxRotateZ(mtx, bx::kPi);
		bx::mtxMul(mtx, scalemtx, mtx);

		Model NewTreeModel;
		NewTreeModel.ibh = PineTree.ibh;
		NewTreeModel.texh = PineTree.texh;
		NewTreeModel.vbh = PineTree.vbh;
		NewTreeModel.matrixTransform = mtx;

		registry.assign<Model>(newTree, NewTreeModel);
	}


	//Terrain
	Model terrain_model;
	terrain_model.vbh = terrain_vbh;
	terrain_model.texh = ground_tex;
	auto TerrainEntity = registry.create();
	registry.assign<Position>(TerrainEntity, bx::Vec3(-250.0, -250.0, 0.0));

	float mtx[16];
	bx::mtxRotateZ(mtx, 0.0f);

	// position x,y,z
	mtx[12] = -250.0f;
	mtx[13] = -250.0f;
	mtx[14] = 0.0f;

	terrain_model.matrixTransform = mtx;

	registry.assign<Model>(TerrainEntity, terrain_model);







	auto testplayer_ent = registry.create();
	registry.assign<Position>(testplayer_ent, bx::Vec3(0.0, 0.0, 0.0));
	registry.assign<Target>(testplayer_ent, bx::Vec3(1.0, 0.0, 0.0));
	registry.assign<entt::tag<"controlled"_hs>>(testplayer_ent);

	auto [posi, targ] = registry.get<Position, Target>(testplayer_ent);

	targ.target.y = 2;



	screen RenScreen;
	RenScreen.HEIGHT = HEIGHT;
	RenScreen.WIDTH = WIDTH;

	RenResourceHandles RenResHandles;
	loadRenderResources(RenResHandles, RenScreen);
	RenResHandles.BasicProgram = m_progMesh;
	RenResHandles.ShadowProgram = m_progShadow;
	RenResHandles.TexColorUniform = s_texColor;
	RenResHandles.SSAO_Program = m_progssao;
	RenResHandles.m_progssaoblurmerge = m_progssaoblurmerge;

	while (!quit)
	{
		lastUpdate = current;
		current = SDL_GetTicks();
		float dT = (current - lastUpdate) / 1000.0f;

		animation_index += dT;
		if (animation_index > 1)
		{
			animation_index = 0;
		}

		camera_heading.x = bx::cos(z_rot);
		camera_heading.y = bx::sin(z_rot);

		const bx::Vec3 at = { 0.0f, 0.0f,   0.0f };
		const bx::Vec3 eye = { 0.0f, -camera_distance, camera_height };

		// Set view and projection matrix for view 0.
		float view[16];
		bx::mtxLookAt(view, eye, at);

		float r_mtx[16];
		bx::mtxRotateZ(r_mtx, z_rot);

		float t_mtx[16];
		bx::mtxTranslate(t_mtx, x_trans, y_trans, 0);

		float new_view[16];
		bx::mtxMul(new_view, r_mtx, view);
		bx::mtxMul(new_view, t_mtx, new_view);

		float proj[16];
		bx::mtxProj(proj,
			30.0f,
			float(WIDTH) / float(HEIGHT),
			0.1f, 1000.0f,
			bgfx::getCaps()->homogeneousDepth);









		while (SDL_PollEvent(&currentEvent) != 0) // only use this to update flags or switch to push events?
		{

			if (currentEvent.type == SDL_QUIT) {
				quit = true;
			}

			if (currentEvent.type == SDL_MOUSEWHEEL)
			{
				if (currentEvent.wheel.y > 0) // scroll up
				{
					camera_distance--;
					camera_height -= 2;
				}
				else if (currentEvent.wheel.y < 0) // scroll down
				{
					camera_distance++;
					camera_height += 2;
				}
			}
		}






		int mx, my;
		if (SDL_GetMouseState(&mx, &my) & SDL_BUTTON(SDL_BUTTON_LEFT))
		{

			//shouldnt put all this picking code in here, just return the coords and a flag?

			SDL_Log("Mouse Button 1 (left) is pressed.");
			float viewProj[16];
			bx::mtxMul(viewProj, new_view, proj);

			float invViewProj[16];
			bx::mtxInverse(invViewProj, viewProj);

			float mouseXNDC = (mx / (float)WIDTH) * 2.0f - 1.0f;
			float mouseYNDC = ((HEIGHT - my) / (float)HEIGHT) * 2.0f - 1.0f;

			const bx::Vec3 pickEye = bx::mulH({ mouseXNDC, mouseYNDC, 0.0f }, invViewProj);
			const bx::Vec3 pickAt = bx::mulH({ mouseXNDC, mouseYNDC, 1.0f }, invViewProj);

			bx::Vec3 dist = bx::sub(pickEye, pickAt);
			float d_x = dist.x / dist.z;
			float d_y = dist.y / dist.z;

			float pos_x = (d_x * -pickEye.z) + pickEye.x;
			float pos_y = (d_y * -pickEye.z) + pickEye.y;

			testplayer.target.x = pos_x;
			testplayer.target.y = pos_y;

			targ.target.x = pos_x;
			targ.target.y = pos_y;
		}


		if (state[SDL_SCANCODE_A])
		{
			y_trans -= camera_heading.y;
			x_trans -= camera_heading.x;
		}
		if (state[SDL_SCANCODE_D])
		{
			y_trans += camera_heading.y;
			x_trans += camera_heading.x;
		}

		if (state[SDL_SCANCODE_W])
		{
			y_trans -= camera_heading.x;
			x_trans += camera_heading.y;
		}
		if (state[SDL_SCANCODE_S])
		{
			y_trans += camera_heading.x;
			x_trans -= camera_heading.y;
		}

		if (state[SDL_SCANCODE_Q])
		{
			z_rot += 0.03;
		}
		if (state[SDL_SCANCODE_E])
		{
			z_rot -= 0.03;
		}

		if (state[SDL_SCANCODE_I])
		{
			animation_index += 0.1;
			if (animation_index > 1)
			{
				animation_index = 0;
			}
		}

		updateactor(testplayer, dT);
		m_pose hand;
		rebuild_vbuff(test_verticies, testMesh, animation_index, hand);
		bgfx::destroy(vbh);
		vbh = bgfx::createVertexBuffer(
			// Static data can be passed with bgfx::makeRef
			bgfx::makeRef(test_verticies, sizeof(VertexData) * size),
			VertexData::ms_decl
		);

		camera RenCam;

		RenCam.proj = proj;
		RenCam.view = new_view;

		float testmat[16] = {2.79904, 0.00, 0.00, 0.00,
			0.00, 3.73205, 0.00, 0.00,
			0.00, 0.00, 1.0001, 1.00,
			0.00, 0.00, -0.10001, 0.00};



		float vec[4] = { 0.6, 0.7, 60.0, 1.0 };
		float result[4];
		bx::vec4MulMtx(result,vec, testmat);



		renderFrame(RenCam, RenScreen, registry, RenResHandles);



	}

	bgfx::shutdown();
	// Free up window
	SDL_DestroyWindow(window);
	// Shutdown SDL
	SDL_Quit();

	return 0;
}