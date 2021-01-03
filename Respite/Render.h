#pragma once
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <entt/entt.hpp>
#include "ShaderLoader.h"
#include "Model.h"

// Render passes
#define RENDER_PASS_GBUFFER 0  // GBuffer for normals and albedo
#define RENDER_PASS_COMBINE 1  // Directional light and final result

// Gbuffer has multiple render targets
#define GBUFFER_RT_NORMAL 0
#define GBUFFER_RT_COLOR  1
#define GBUFFER_RT_DEPTH  2 

#define SAMPLER_POINT_CLAMP  (BGFX_SAMPLER_POINT|BGFX_SAMPLER_UVW_CLAMP)
#define SAMPLER_POINT_MIRROR (BGFX_SAMPLER_POINT|BGFX_SAMPLER_UVW_MIRROR)
#define SAMPLER_LINEAR_CLAMP (BGFX_SAMPLER_UVW_CLAMP)

#define SSAO_DEPTH_MIP_LEVELS                       4

//object occlusion
//
//draw a rectangle behind the controlled character, the same height and width of the character
//
//get the near plane co-ords of this rect.
//
//create bounding box from the near and far rect, then test this agains the other object boudning boxes to determine which objects to dissolve.


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
	};

	static bgfx::VertexDecl ms_decl;
};


struct camera {
	float* view; //[16] 4x4 matrix
	float* proj; //[16] 4x4 matrix
	float* proj2; //[16] 4x4 matrix non homogenus
};

struct screen {
	unsigned int WIDTH;
	unsigned int HEIGHT;
};



inline float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}




struct Settings
{
	float   m_radius;                            // [0.0,  ~ ] World (view) space size of the occlusion sphere.
	float   m_shadowMultiplier;                  // [0.0, 5.0] Effect strength linear multiplier
	float   m_shadowPower;                       // [0.5, 5.0] Effect strength pow modifier
	float   m_shadowClamp;                       // [0.0, 1.0] Effect max limit (applied after multiplier but before blur)
	float   m_horizonAngleThreshold;             // [0.0, 0.2] Limits self-shadowing (makes the sampling area less of a hemisphere, more of a spherical cone, to avoid self-shadowing and various artifacts due to low tessellation and depth buffer imprecision, etc.)
	float   m_fadeOutFrom;                       // [0.0,  ~ ] Distance to start start fading out the effect.
	float   m_fadeOutTo;                         // [0.0,  ~ ] Distance at which the effect is faded out.
	int32_t m_qualityLevel;                      // [ -1,  3 ] Effect quality; -1 - lowest (low, half res checkerboard), 0 - low, 1 - medium, 2 - high, 3 - very high / adaptive; each quality level is roughly 2x more costly than the previous, except the q3 which is variable but, in general, above q2.
	float   m_adaptiveQualityLimit;              // [0.0, 1.0] (only for Quality Level 3)
	int32_t m_blurPassCount;                     // [  0,   6] Number of edge-sensitive smart blur passes to apply. Quality 0 is an exception with only one 'dumb' blur pass used.
	float   m_sharpness;                         // [0.0, 1.0] (How much to bleed over edges; 1: not at all, 0.5: half-half; 0.0: completely ignore edges)
	float   m_temporalSupersamplingAngleOffset;  // [0.0,  PI] Used to rotate sampling kernel; If using temporal AA / supersampling, suggested to rotate by ( (frame%3)/3.0*PI ) or similar. Kernel is already symmetrical, which is why we use PI and not 2*PI.
	float   m_temporalSupersamplingRadiusOffset; // [0.0, 2.0] Used to scale sampling kernel; If using temporal AA / supersampling, suggested to scale by ( 1.0f + (((frame%3)-1.0)/3.0)*0.1 ) or similar.
	float   m_detailShadowStrength;              // [0.0, 5.0] Used for high-res detail AO using neighboring depth pixels: adds a lot of detail but also reduces temporal stability (adds aliasing).
	bool    m_generateNormals;                   // [true/false] If true normals will be generated from depth.

	Settings()
	{
		m_radius = 1.2f;
		m_shadowMultiplier = 1.0f;
		m_shadowPower = 1.50f;
		m_shadowClamp = 0.98f;
		m_horizonAngleThreshold = 0.06f;
		m_fadeOutFrom = 50.0f;
		m_fadeOutTo = 200.0f;
		m_adaptiveQualityLimit = 0.45f;
		m_qualityLevel = 0;
		m_blurPassCount = 2;
		m_sharpness = 0.98f;
		m_temporalSupersamplingAngleOffset = 0.0f;
		m_temporalSupersamplingRadiusOffset = 1.0f;
		m_detailShadowStrength = 0.5f;
		m_generateNormals = true;
	}
};

struct Uniforms
{
	enum { NumVec4 = 19 };

	void init()
	{
		u_params = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, NumVec4);
	}

	void submit()
	{
		bgfx::setUniform(u_params, m_params, NumVec4);
	}

	void destroy()
	{
		bgfx::destroy(u_params);
	}

	union
	{
		struct
		{
			/*  0    */ struct { float m_viewportPixelSize[2]; float m_halfViewportPixelSize[2]; };
			/*  1    */ struct { float m_depthUnpackConsts[2]; float m_unused0[2]; };
			/*  2    */ struct { float m_ndcToViewMul[2]; float m_ndcToViewAdd[2]; };
			/*  3    */ struct { float m_perPassFullResCoordOffset[2]; float m_perPassFullResUVOffset[2]; };
			/*  4    */ struct { float m_viewport2xPixelSize[2]; float m_viewport2xPixelSize_x_025[2]; };
			/*  5    */ struct { float m_effectRadius; float m_effectShadowStrength; float m_effectShadowPow; float m_effectShadowClamp; };
			/*  6    */ struct { float m_effectFadeOutMul; float m_effectFadeOutAdd; float m_effectHorizonAngleThreshold; float m_effectSamplingRadiusNearLimitRec; };
			/*  7    */ struct { float m_depthPrecisionOffsetMod; float m_negRecEffectRadius; float m_loadCounterAvgDiv; float m_adaptiveSampleCountLimit; };
			/*  8    */ struct { float m_invSharpness; float m_passIndex; float m_quarterResPixelSize[2]; };
			/*  9-13 */ struct { float m_patternRotScaleMatrices[5][4]; };
			/* 14    */ struct { float m_normalsUnpackMul; float m_normalsUnpackAdd; float m_detailAOStrength; float m_layer; };
			/* 15-18 */ struct { float m_normalsWorldToViewspaceMatrix[16]; };
		};

		float m_params[NumVec4 * 4];
	};

	bgfx::UniformHandle u_params;
};

inline void vec2Set(float* _v, float _x, float _y)
{
	_v[0] = _x;
	_v[1] = _y;
}

inline void vec4Set(float* _v, float _x, float _y, float _z, float _w)
{
	_v[0] = _x;
	_v[1] = _y;
	_v[2] = _z;
	_v[3] = _w;
}

inline void vec4iSet(int32_t* _v, int32_t _x, int32_t _y, int32_t _z, int32_t _w)
{
	_v[0] = _x;
	_v[1] = _y;
	_v[2] = _z;
	_v[3] = _w;
}

static const int32_t cMaxBlurPassCount = 6;

struct RenderResources {

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






	//ASSAO

	 // Resource handles
	bgfx::ProgramHandle m_gbufferProgram;
	bgfx::ProgramHandle m_combineProgram;

	bgfx::ProgramHandle m_prepareDepthsProgram;
	bgfx::ProgramHandle m_prepareDepthsAndNormalsProgram;
	bgfx::ProgramHandle m_prepareDepthsHalfProgram;
	bgfx::ProgramHandle m_prepareDepthsAndNormalsHalfProgram;
	bgfx::ProgramHandle m_prepareDepthMipProgram;
	bgfx::ProgramHandle m_generateQ0Program;
	bgfx::ProgramHandle m_generateQ1Program;
	bgfx::ProgramHandle m_generateQ2Program;
	bgfx::ProgramHandle m_generateQ3Program;
	bgfx::ProgramHandle m_generateQ3BaseProgram;
	bgfx::ProgramHandle m_smartBlurProgram;
	bgfx::ProgramHandle m_smartBlurWideProgram;
	bgfx::ProgramHandle m_nonSmartBlurProgram;
	bgfx::ProgramHandle m_applyProgram;
	bgfx::ProgramHandle m_nonSmartApplyProgram;
	bgfx::ProgramHandle m_nonSmartHalfApplyProgram;
	bgfx::ProgramHandle m_generateImportanceMapProgram;
	bgfx::ProgramHandle m_postprocessImportanceMapAProgram;
	bgfx::ProgramHandle m_postprocessImportanceMapBProgram;
	bgfx::ProgramHandle m_loadCounterClearProgram;

	bgfx::FrameBufferHandle m_gbuffer;

	// Shader uniforms
	bgfx::UniformHandle u_rect;
	bgfx::UniformHandle u_combineParams;

	// Uniforms to identify texture samples
	bgfx::UniformHandle s_normal;
	bgfx::UniformHandle s_depth;
	bgfx::UniformHandle s_color;
	bgfx::UniformHandle s_albedo;
	bgfx::UniformHandle s_ao;
	bgfx::UniformHandle s_blurInput;
	bgfx::UniformHandle s_finalSSAO;
	bgfx::UniformHandle s_depthSource;
	bgfx::UniformHandle s_viewspaceDepthSource;
	bgfx::UniformHandle s_viewspaceDepthSourceMirror;
	bgfx::UniformHandle s_importanceMap;


	float   m_view[16];
	float   m_proj[16];
	float   m_proj2[16];
	int32_t m_size[2];
	int32_t m_halfSize[2];
	int32_t m_quarterSize[2];
	int32_t m_fullResOutScissorRect[4];
	int32_t m_halfResOutScissorRect[4];
	int32_t m_border;

	Uniforms ASSAO_Uniforms;
	Settings ASSAO_Settings;

	// Various render targets
	bgfx::TextureHandle m_halfDepths[4];
	bgfx::TextureHandle m_pingPongHalfResultA;
	bgfx::TextureHandle m_pingPongHalfResultB;
	bgfx::TextureHandle m_finalResults;
	bgfx::TextureHandle m_aoMap;
	bgfx::TextureHandle m_normals;


	// Only needed for quality level 3 (adaptive quality)
	bgfx::TextureHandle m_importanceMap;
	bgfx::TextureHandle m_importanceMapPong;
	bgfx::DynamicIndexBufferHandle m_loadCounter;



	//Combine
	bgfx::ProgramHandle m_progssaoblurmerge;

};


void loadRenderResources(RenderResources& ResourceHandles, screen screen);
void renderFrame(camera cam, screen screen, entt::registry& entitiesRegistry, RenderResources RenResources);