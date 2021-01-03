#include "Render.h"


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





void loadRenderResources(RenderResources& ResourceHandles, screen screen)
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

	// Create uniforms
	ResourceHandles.u_combineParams = bgfx::createUniform("u_combineParams", bgfx::UniformType::Vec4, 2);
	ResourceHandles.u_rect = bgfx::createUniform("u_rect", bgfx::UniformType::Vec4);  // viewport/scissor rect for compute
	Uniforms m_uniforms;
	m_uniforms.m_layer = 0;
	m_uniforms.init();
	ResourceHandles.ASSAO_Uniforms = m_uniforms;
	// Create texture sampler uniforms (used when we bind textures)
	ResourceHandles.s_normal = bgfx::createUniform("s_normal", bgfx::UniformType::Sampler);  // Normal gbuffer
	ResourceHandles.s_depth = bgfx::createUniform("s_depth", bgfx::UniformType::Sampler);  // Normal gbuffer
	ResourceHandles.s_color = bgfx::createUniform("s_color", bgfx::UniformType::Sampler);  // Color (albedo) gbuffer
	ResourceHandles.s_albedo = bgfx::createUniform("s_albedo", bgfx::UniformType::Sampler);

	ResourceHandles.s_ao = bgfx::createUniform("s_ao", bgfx::UniformType::Sampler);
	ResourceHandles.s_blurInput = bgfx::createUniform("s_blurInput", bgfx::UniformType::Sampler);
	ResourceHandles.s_finalSSAO = bgfx::createUniform("s_finalSSAO", bgfx::UniformType::Sampler);
	ResourceHandles.s_depthSource = bgfx::createUniform("s_depthSource", bgfx::UniformType::Sampler);
	ResourceHandles.s_viewspaceDepthSource = bgfx::createUniform("s_viewspaceDepthSource", bgfx::UniformType::Sampler);
	ResourceHandles.s_viewspaceDepthSourceMirror = bgfx::createUniform("s_viewspaceDepthSourceMirror", bgfx::UniformType::Sampler);
	ResourceHandles.s_importanceMap = bgfx::createUniform("s_importanceMap", bgfx::UniformType::Sampler);

	// Create program from shaders.
	ResourceHandles.m_gbufferProgram = loadProgram("assao\\vs_assao_gbuffer.bin", "assao\\fs_assao_gbuffer.bin");  // Gbuffer
	ResourceHandles.m_combineProgram = loadProgram("assao\\vs_assao.bin", "assao\\fs_assao_deferred_combine.bin");

	ResourceHandles.m_prepareDepthsProgram = loadProgram("assao\\cs_assao_prepare_depths.bin", NULL);
	ResourceHandles.m_prepareDepthsAndNormalsProgram = loadProgram("assao\\cs_assao_prepare_depths_and_normals.bin", NULL);
	ResourceHandles.m_prepareDepthsHalfProgram = loadProgram("assao\\cs_assao_prepare_depths_half.bin", NULL);
	ResourceHandles.m_prepareDepthsAndNormalsHalfProgram = loadProgram("assao\\cs_assao_prepare_depths_and_normals_half.bin", NULL);
	ResourceHandles.m_prepareDepthMipProgram = loadProgram("assao\\cs_assao_prepare_depth_mip.bin", NULL);
	ResourceHandles.m_generateQ0Program = loadProgram("assao\\cs_assao_generate_q0.bin", NULL);
	ResourceHandles.m_generateQ1Program = loadProgram("assao\\cs_assao_generate_q1.bin", NULL);
	ResourceHandles.m_generateQ2Program = loadProgram("assao\\cs_assao_generate_q2.bin", NULL);
	ResourceHandles.m_generateQ3Program = loadProgram("assao\\cs_assao_generate_q3.bin", NULL);
	ResourceHandles.m_generateQ3BaseProgram = loadProgram("assao\\cs_assao_generate_q3base.bin", NULL);
	ResourceHandles.m_smartBlurProgram = loadProgram("assao\\cs_assao_smart_blur.bin", NULL);
	ResourceHandles.m_smartBlurWideProgram = loadProgram("assao\\cs_assao_smart_blur_wide.bin", NULL);
	ResourceHandles.m_nonSmartBlurProgram = loadProgram("assao\\cs_assao_non_smart_blur.bin", NULL);
	ResourceHandles.m_applyProgram = loadProgram("assao\\cs_assao_apply.bin", NULL);
	ResourceHandles.m_nonSmartApplyProgram = loadProgram("assao\\cs_assao_non_smart_apply.bin", NULL);
	ResourceHandles.m_nonSmartHalfApplyProgram = loadProgram("assao\\cs_assao_non_smart_half_apply.bin", NULL);
	ResourceHandles.m_generateImportanceMapProgram = loadProgram("assao\\cs_assao_generate_importance_map.bin", NULL);
	ResourceHandles.m_postprocessImportanceMapAProgram = loadProgram("assao\\cs_assao_postprocess_importance_map_a.bin", NULL);
	ResourceHandles.m_postprocessImportanceMapBProgram = loadProgram("assao\\cs_assao_postprocess_importance_map_b.bin", NULL);
	ResourceHandles.m_loadCounterClearProgram = loadProgram("assao\\cs_assao_load_counter_clear.bin", NULL);


	ResourceHandles.m_loadCounter = bgfx::createDynamicIndexBuffer(1, BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32);




	ResourceHandles.m_border = 0;




	ResourceHandles.m_size[0] = screen.WIDTH + 2 * ResourceHandles.m_border;
	ResourceHandles.m_size[1] = screen.HEIGHT + 2 * ResourceHandles.m_border;
	ResourceHandles.m_halfSize[0] = (ResourceHandles.m_size[0] + 1) / 2;
	ResourceHandles.m_halfSize[1] = (ResourceHandles.m_size[1] + 1) / 2;
	ResourceHandles.m_quarterSize[0] = (ResourceHandles.m_halfSize[0] + 1) / 2;
	ResourceHandles.m_quarterSize[1] = (ResourceHandles.m_halfSize[1] + 1) / 2;

	vec4iSet(ResourceHandles.m_fullResOutScissorRect, ResourceHandles.m_border, ResourceHandles.m_border, screen.WIDTH + ResourceHandles.m_border, screen.HEIGHT + ResourceHandles.m_border);
	vec4iSet(ResourceHandles.m_halfResOutScissorRect, ResourceHandles.m_fullResOutScissorRect[0] / 2, ResourceHandles.m_fullResOutScissorRect[1] / 2, (ResourceHandles.m_fullResOutScissorRect[2] + 1) / 2, (ResourceHandles.m_fullResOutScissorRect[3] + 1) / 2);

	int32_t blurEnlarge = cMaxBlurPassCount + bx::max(0, cMaxBlurPassCount - 2);  // +1 for max normal blurs, +2 for wide blurs
	vec4iSet(ResourceHandles.m_halfResOutScissorRect, bx::max(0, ResourceHandles.m_halfResOutScissorRect[0] - blurEnlarge), bx::max(0, ResourceHandles.m_halfResOutScissorRect[1] - blurEnlarge),
		bx::min(ResourceHandles.m_halfSize[0], ResourceHandles.m_halfResOutScissorRect[2] + blurEnlarge), bx::min(ResourceHandles.m_halfSize[1], ResourceHandles.m_halfResOutScissorRect[3] + blurEnlarge));

	// Make gbuffer and related textures
	const uint64_t tsFlags = 0
		| BGFX_TEXTURE_RT
		| BGFX_SAMPLER_MIN_POINT
		| BGFX_SAMPLER_MAG_POINT
		| BGFX_SAMPLER_MIP_POINT
		| BGFX_SAMPLER_U_CLAMP
		| BGFX_SAMPLER_V_CLAMP
		;

	bgfx::TextureHandle gbufferTex[3];
	gbufferTex[GBUFFER_RT_NORMAL] = bgfx::createTexture2D(uint16_t(ResourceHandles.m_size[0]), uint16_t(ResourceHandles.m_size[1]), false, 1, bgfx::TextureFormat::BGRA8, tsFlags);
	gbufferTex[GBUFFER_RT_COLOR] = bgfx::createTexture2D(uint16_t(ResourceHandles.m_size[0]), uint16_t(ResourceHandles.m_size[1]), false, 1, bgfx::TextureFormat::BGRA8, tsFlags);
	gbufferTex[GBUFFER_RT_DEPTH] = bgfx::createTexture2D(uint16_t(ResourceHandles.m_size[0]), uint16_t(ResourceHandles.m_size[1]), false, 1, bgfx::TextureFormat::D24, tsFlags);
	ResourceHandles.m_gbuffer = bgfx::createFrameBuffer(BX_COUNTOF(gbufferTex), gbufferTex, true);

	for (int32_t i = 0; i < 4; i++)
	{
		ResourceHandles.m_halfDepths[i] = bgfx::createTexture2D(uint16_t(ResourceHandles.m_halfSize[0]), uint16_t(ResourceHandles.m_halfSize[1]), true, 1, bgfx::TextureFormat::R16F, BGFX_TEXTURE_COMPUTE_WRITE | SAMPLER_POINT_CLAMP);
	}

	ResourceHandles.m_pingPongHalfResultA = bgfx::createTexture2D(uint16_t(ResourceHandles.m_halfSize[0]), uint16_t(ResourceHandles.m_halfSize[1]), false, 2, bgfx::TextureFormat::RG8, BGFX_TEXTURE_COMPUTE_WRITE);
	ResourceHandles.m_pingPongHalfResultB = bgfx::createTexture2D(uint16_t(ResourceHandles.m_halfSize[0]), uint16_t(ResourceHandles.m_halfSize[1]), false, 2, bgfx::TextureFormat::RG8, BGFX_TEXTURE_COMPUTE_WRITE);

	ResourceHandles.m_finalResults = bgfx::createTexture2D(uint16_t(ResourceHandles.m_halfSize[0]), uint16_t(ResourceHandles.m_halfSize[1]), false, 4, bgfx::TextureFormat::RG8, BGFX_TEXTURE_COMPUTE_WRITE | SAMPLER_LINEAR_CLAMP);

	ResourceHandles.m_normals = bgfx::createTexture2D(uint16_t(ResourceHandles.m_size[0]), uint16_t(ResourceHandles.m_size[1]), false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_COMPUTE_WRITE);

	ResourceHandles.m_importanceMap = bgfx::createTexture2D(uint16_t(ResourceHandles.m_quarterSize[0]), uint16_t(ResourceHandles.m_quarterSize[1]), false, 1, bgfx::TextureFormat::R8, BGFX_TEXTURE_COMPUTE_WRITE | SAMPLER_LINEAR_CLAMP);
	ResourceHandles.m_importanceMapPong = bgfx::createTexture2D(uint16_t(ResourceHandles.m_quarterSize[0]), uint16_t(ResourceHandles.m_quarterSize[1]), false, 1, bgfx::TextureFormat::R8, BGFX_TEXTURE_COMPUTE_WRITE | SAMPLER_LINEAR_CLAMP);

	ResourceHandles.m_aoMap = bgfx::createTexture2D(uint16_t(ResourceHandles.m_size[0]), uint16_t(ResourceHandles.m_size[1]), false, 1, bgfx::TextureFormat::R8, BGFX_TEXTURE_COMPUTE_WRITE | SAMPLER_POINT_CLAMP);

}

void updateUniforms(int32_t _pass, Uniforms& m_uniforms, RenderResources& ResHandles, Settings m_settings)
{
	vec2Set(m_uniforms.m_viewportPixelSize, 1.0f / (float)ResHandles.m_size[0], 1.0f / (float)ResHandles.m_size[1]);
	vec2Set(m_uniforms.m_halfViewportPixelSize, 1.0f / (float)ResHandles.m_halfSize[0], 1.0f / (float)ResHandles.m_halfSize[1]);

	vec2Set(m_uniforms.m_viewport2xPixelSize, m_uniforms.m_viewportPixelSize[0] * 2.0f, m_uniforms.m_viewportPixelSize[1] * 2.0f);
	vec2Set(m_uniforms.m_viewport2xPixelSize_x_025, m_uniforms.m_viewport2xPixelSize[0] * 0.25f, m_uniforms.m_viewport2xPixelSize[1] * 0.25f);

	float depthLinearizeMul = -ResHandles.m_proj2[3 * 4 + 2]; // float depthLinearizeMul = ( clipFar * clipNear ) / ( clipFar - clipNear );
	float depthLinearizeAdd = ResHandles.m_proj2[2 * 4 + 2]; // float depthLinearizeAdd = clipFar / ( clipFar - clipNear );
											   // correct the handedness issue. need to make sure this below is correct, but I think it is.

	if (depthLinearizeMul * depthLinearizeAdd < 0)
	{
		depthLinearizeAdd = -depthLinearizeAdd;
	}

	vec2Set(m_uniforms.m_depthUnpackConsts, depthLinearizeMul, depthLinearizeAdd);

	float tanHalfFOVY = 1.0f / ResHandles.m_proj2[1 * 4 + 1];    // = tanf( drawContext.Camera.GetYFOV( ) * 0.5f );
	float tanHalfFOVX = 1.0F / ResHandles.m_proj2[0];    // = tanHalfFOVY * drawContext.Camera.GetAspect( );

	if (bgfx::getRendererType() == bgfx::RendererType::OpenGL)
	{
		vec2Set(m_uniforms.m_ndcToViewMul, tanHalfFOVX * 2.0f, tanHalfFOVY * 2.0f);
		vec2Set(m_uniforms.m_ndcToViewAdd, tanHalfFOVX * -1.0f, tanHalfFOVY * -1.0f);
	}
	else
	{
		vec2Set(m_uniforms.m_ndcToViewMul, tanHalfFOVX * 2.0f, tanHalfFOVY * -2.0f);
		vec2Set(m_uniforms.m_ndcToViewAdd, tanHalfFOVX * -1.0f, tanHalfFOVY * 1.0f);
	}

	m_uniforms.m_effectRadius = bx::clamp(m_settings.m_radius, 0.0f, 100000.0f);
	m_uniforms.m_effectShadowStrength = bx::clamp(m_settings.m_shadowMultiplier * 4.3f, 0.0f, 10.0f);
	m_uniforms.m_effectShadowPow = bx::clamp(m_settings.m_shadowPower, 0.0f, 10.0f);
	m_uniforms.m_effectShadowClamp = bx::clamp(m_settings.m_shadowClamp, 0.0f, 1.0f);
	m_uniforms.m_effectFadeOutMul = -1.0f / (m_settings.m_fadeOutTo - m_settings.m_fadeOutFrom);
	m_uniforms.m_effectFadeOutAdd = m_settings.m_fadeOutFrom / (m_settings.m_fadeOutTo - m_settings.m_fadeOutFrom) + 1.0f;
	m_uniforms.m_effectHorizonAngleThreshold = bx::clamp(m_settings.m_horizonAngleThreshold, 0.0f, 1.0f);

	// 1.2 seems to be around the best trade off - 1.0 means on-screen radius will stop/slow growing when the camera is at 1.0 distance, so, depending on FOV, basically filling up most of the screen
	// This setting is viewspace-dependent and not screen size dependent intentionally, so that when you change FOV the effect stays (relatively) similar.
	float effectSamplingRadiusNearLimit = (m_settings.m_radius * 1.2f);

	// if the depth precision is switched to 32bit float, this can be set to something closer to 1 (0.9999 is fine)
	m_uniforms.m_depthPrecisionOffsetMod = 0.9992f;

	// used to get average load per pixel; 9.0 is there to compensate for only doing every 9th InterlockedAdd in PSPostprocessImportanceMapB for performance reasons
	m_uniforms.m_loadCounterAvgDiv = 9.0f / (float)(ResHandles.m_quarterSize[0] * ResHandles.m_quarterSize[1] * 255.0);

	// Special settings for lowest quality level - just nerf the effect a tiny bit
	if (m_settings.m_qualityLevel <= 0)
	{
		effectSamplingRadiusNearLimit *= 1.50f;

		if (m_settings.m_qualityLevel < 0)
		{
			m_uniforms.m_effectRadius *= 0.8f;
		}
	}

	effectSamplingRadiusNearLimit /= tanHalfFOVY; // to keep the effect same regardless of FOV

	m_uniforms.m_effectSamplingRadiusNearLimitRec = 1.0f / effectSamplingRadiusNearLimit;

	m_uniforms.m_adaptiveSampleCountLimit = m_settings.m_adaptiveQualityLimit;

	m_uniforms.m_negRecEffectRadius = -1.0f / m_uniforms.m_effectRadius;

	if (bgfx::getCaps()->originBottomLeft)
	{
		vec2Set(m_uniforms.m_perPassFullResCoordOffset, (float)(_pass % 2), 1.0f - (float)(_pass / 2));
		vec2Set(m_uniforms.m_perPassFullResUVOffset, ((_pass % 2) - 0.0f) / ResHandles.m_size[0], (1.0f - ((_pass / 2) - 0.0f)) / ResHandles.m_size[1]);
	}
	else
	{
		vec2Set(m_uniforms.m_perPassFullResCoordOffset, (float)(_pass % 2), (float)(_pass / 2));
		vec2Set(m_uniforms.m_perPassFullResUVOffset, ((_pass % 2) - 0.0f) / ResHandles.m_size[0], ((_pass / 2) - 0.0f) / ResHandles.m_size[1]);
	}

	m_uniforms.m_invSharpness = bx::clamp(1.0f - m_settings.m_sharpness, 0.0f, 1.0f);
	m_uniforms.m_passIndex = (float)_pass;
	vec2Set(m_uniforms.m_quarterResPixelSize, 1.0f / (float)ResHandles.m_quarterSize[0], 1.0f / (float)ResHandles.m_quarterSize[1]);

	float additionalAngleOffset = m_settings.m_temporalSupersamplingAngleOffset;  // if using temporal supersampling approach (like "Progressive Rendering Using Multi-frame Sampling" from GPU Pro 7, etc.)
	float additionalRadiusScale = m_settings.m_temporalSupersamplingRadiusOffset; // if using temporal supersampling approach (like "Progressive Rendering Using Multi-frame Sampling" from GPU Pro 7, etc.)
	const int32_t subPassCount = 5;
	for (int32_t subPass = 0; subPass < subPassCount; subPass++)
	{
		int32_t a = _pass;
		int32_t b = subPass;

		int32_t spmap[5]{ 0, 1, 4, 3, 2 };
		b = spmap[subPass];

		float ca, sa;
		float angle0 = ((float)a + (float)b / (float)subPassCount) * (3.1415926535897932384626433832795f) * 0.5f;
		angle0 += additionalAngleOffset;

		ca = bx::cos(angle0);
		sa = bx::sin(angle0);

		float scale = 1.0f + (a - 1.5f + (b - (subPassCount - 1.0f) * 0.5f) / (float)subPassCount) * 0.07f;
		scale *= additionalRadiusScale;

		vec4Set(m_uniforms.m_patternRotScaleMatrices[subPass], scale * ca, scale * -sa, -scale * sa, -scale * ca);
	}

	m_uniforms.m_normalsUnpackMul = 2.0f;
	m_uniforms.m_normalsUnpackAdd = -1.0f;

	m_uniforms.m_detailAOStrength = m_settings.m_detailShadowStrength;

	if (m_settings.m_generateNormals)
	{
		bx::mtxIdentity(m_uniforms.m_normalsWorldToViewspaceMatrix);
	}
	else
	{
		bx::mtxTranspose(m_uniforms.m_normalsWorldToViewspaceMatrix, ResHandles.m_view);
	}
}

void renderFrame(camera cam, screen screen, entt::registry& entitiesRegistry, RenderResources RenResources)
{
	uint16_t RENDER_SHADOW_PASS_ID = 0;
	uint16_t RENDER_SCENE_PASS_ID = 1;

	// Setup lights.
	float lightPos[4];
	lightPos[0] = 0.0f;
	lightPos[1] = 150.0f;
	lightPos[2] = -200.0f;
	lightPos[3] = 0.0f;

	bgfx::setUniform(RenResources.u_lightPos, lightPos);

	// Define matrices.
	float lightView[16];
	float lightProj[16];

	const bx::Vec3 at = { 0.0f,  0.0f,   0.0f };
	const bx::Vec3 eye = { -lightPos[0], -lightPos[1], -lightPos[2] };
	bx::mtxLookAt(lightView, eye, at);

	const bgfx::Caps* caps = bgfx::getCaps();
	const float area = 100.0f;
	bx::mtxOrtho(lightProj, -area, area, -area, area, -100.0f, 1000.0f, 0.0f, caps->homogeneousDepth);

	bool m_shadowSamplerSupported = 0 != (caps->supported & BGFX_CAPS_TEXTURE_COMPARE_LEQUAL);

	bgfx::setViewRect(RENDER_SHADOW_PASS_ID, 0, 0, RenResources.m_shadowMapSize, RenResources.m_shadowMapSize);
	bgfx::setViewFrameBuffer(RENDER_SHADOW_PASS_ID, RenResources.ShadowMapFB);
	bgfx::setViewTransform(RENDER_SHADOW_PASS_ID, lightView, lightProj);
	bgfx::setViewName(RENDER_SHADOW_PASS_ID, "Shadow Map");

	bgfx::setViewRect(RENDER_SCENE_PASS_ID, 0, 0, screen.WIDTH, screen.HEIGHT);
	bgfx::setViewTransform(RENDER_SCENE_PASS_ID, cam.view, cam.proj);
	bgfx::setViewFrameBuffer(RENDER_SCENE_PASS_ID, RenResources.m_gbuffer);
	bgfx::setViewName(RENDER_SCENE_PASS_ID, "Gbuffer");

	// Clear backbuffer and shadowmap framebuffer at beginning.
	bgfx::setViewClear(RENDER_SHADOW_PASS_ID
		, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		, 0x303030ff, 1.0f, 0
	);

	bgfx::setViewClear(RENDER_SCENE_PASS_ID
		, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		, 0x000000FF, 1.0f, 0
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
		bgfx::setUniform(RenResources.u_lightMtx, lightMtx);
		// Set model matrix for rendering.
		bgfx::setTransform(model.matrixTransform);

		// Set vertex and index buffer.
		bgfx::setVertexBuffer(0, model.vbh);
		bgfx::setIndexBuffer(model.ibh);

		unsigned long long state = 0
			| BGFX_STATE_WRITE_Z
			| BGFX_STATE_DEPTH_TEST_LESS
			| BGFX_STATE_MSAA
			| BGFX_STATE_CULL_CW
			;


		// Set render states.
		bgfx::setState(state);

		// Submit primitive for rendering to view 0.
		bgfx::submit(RENDER_SHADOW_PASS_ID, RenResources.ShadowProgram);

	}


	for (auto entity : staticModelView) {
		auto& model = staticModelView.get<Model>(entity);

		bx::mtxMul(lightMtx, model.matrixTransform, mtxShadow);
		bgfx::setUniform(RenResources.u_lightMtx, lightMtx);
		// Set model matrix for rendering.
		bgfx::setTransform(model.matrixTransform);

		// Set vertex and index buffer.
		bgfx::setVertexBuffer(0, model.vbh);
		bgfx::setIndexBuffer(model.ibh);

		bgfx::setTexture(0, RenResources.s_shadowMap, RenResources.shadowMapTexture);

		bgfx::setTexture(1, RenResources.TexColorUniform, model.texh);

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
		bgfx::submit(RENDER_SCENE_PASS_ID, RenResources.BasicProgram);

	}

	updateUniforms(0, RenResources.ASSAO_Uniforms, RenResources, RenResources.ASSAO_Settings);

	Settings m_settings = RenResources.ASSAO_Settings;
	Uniforms m_uniforms = RenResources.ASSAO_Uniforms;

	bgfx::ViewId view = 2;
	bgfx::setViewName(view, "ASSAO");

	{
		bgfx::setTexture(0, RenResources.s_depthSource, bgfx::getTexture(RenResources.m_gbuffer, GBUFFER_RT_DEPTH), SAMPLER_POINT_CLAMP);
		m_uniforms.submit();

		if (m_settings.m_generateNormals)
		{
			bgfx::setImage(5, RenResources.m_normals, 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA8);
		}

		if (m_settings.m_qualityLevel < 0)
		{
			for (int32_t j = 0; j < 2; ++j)
			{
				bgfx::setImage((uint8_t)(j + 1), RenResources.m_halfDepths[j == 0 ? 0 : 3], 0, bgfx::Access::Write, bgfx::TextureFormat::R16F);
			}

			bgfx::dispatch(view, m_settings.m_generateNormals ? RenResources.m_prepareDepthsAndNormalsHalfProgram : RenResources.m_prepareDepthsHalfProgram, (RenResources.m_halfSize[0] + 7) / 8, (RenResources.m_halfSize[1] + 7) / 8);
		}
		else
		{
			for (int32_t j = 0; j < 4; ++j)
			{
				bgfx::setImage((uint8_t)(j + 1), RenResources.m_halfDepths[j], 0, bgfx::Access::Write, bgfx::TextureFormat::R16F);
			}

			bgfx::dispatch(view, m_settings.m_generateNormals ? RenResources.m_prepareDepthsAndNormalsProgram : RenResources.m_prepareDepthsProgram, (RenResources.m_halfSize[0] + 7) / 8, (RenResources.m_halfSize[1] + 7) / 8);

		}
	}

	// only do mipmaps for higher quality levels (not beneficial on quality level 1, and detrimental on quality level 0)
	if (m_settings.m_qualityLevel > 1)
	{
		uint16_t mipWidth = (uint16_t)RenResources.m_halfSize[0];
		uint16_t mipHeight = (uint16_t)RenResources.m_halfSize[1];

		for (uint8_t i = 1; i < SSAO_DEPTH_MIP_LEVELS; i++)
		{
			mipWidth = (uint16_t)bx::max(1, mipWidth >> 1);
			mipHeight = (uint16_t)bx::max(1, mipHeight >> 1);

			for (uint8_t j = 0; j < 4; ++j)
			{
				bgfx::setImage(j, RenResources.m_halfDepths[j], i - 1, bgfx::Access::Read, bgfx::TextureFormat::R16F);
				bgfx::setImage(j + 4, RenResources.m_halfDepths[j], i, bgfx::Access::Write, bgfx::TextureFormat::R16F);
			}

			m_uniforms.submit();
			float rect[4] = { 0.0f, 0.0f, (float)mipWidth, (float)mipHeight };
			bgfx::setUniform(RenResources.u_rect, rect);

			bgfx::dispatch(view, RenResources.m_prepareDepthMipProgram, (mipWidth + 7) / 8, (mipHeight + 7) / 8);
		}
	}

	// for adaptive quality, importance map pass
	for (int32_t ssaoPass = 0; ssaoPass < 2; ++ssaoPass)
	{
		if (ssaoPass == 0
			&& m_settings.m_qualityLevel < 3)
		{
			continue;
		}

		bool adaptiveBasePass = (ssaoPass == 0);

		BX_UNUSED(adaptiveBasePass);

		int32_t passCount = 4;

		int32_t halfResNumX = (RenResources.m_halfResOutScissorRect[2] - RenResources.m_halfResOutScissorRect[0] + 7) / 8;
		int32_t halfResNumY = (RenResources.m_halfResOutScissorRect[3] - RenResources.m_halfResOutScissorRect[1] + 7) / 8;
		float halfResRect[4] = { (float)RenResources.m_halfResOutScissorRect[0], (float)RenResources.m_halfResOutScissorRect[1], (float)RenResources.m_halfResOutScissorRect[2], (float)RenResources.m_halfResOutScissorRect[3] };

		for (int32_t pass = 0; pass < passCount; pass++)
		{
			if (m_settings.m_qualityLevel < 0
				&& (pass == 1 || pass == 2))
			{
				continue;
			}

			int32_t blurPasses = m_settings.m_blurPassCount;
			blurPasses = bx::min(blurPasses, cMaxBlurPassCount);

			if (m_settings.m_qualityLevel == 3)
			{
				// if adaptive, at least one blur pass needed as the first pass needs to read the final texture results - kind of awkward
				if (adaptiveBasePass)
				{
					blurPasses = 0;
				}
				else
				{
					blurPasses = bx::max(1, blurPasses);
				}
			}
			else if (m_settings.m_qualityLevel <= 0)
			{
				// just one blur pass allowed for minimum quality
				blurPasses = bx::min(1, m_settings.m_blurPassCount);
			}

			updateUniforms(pass, RenResources.ASSAO_Uniforms, RenResources, RenResources.ASSAO_Settings);

			bgfx::TextureHandle pPingRT = RenResources.m_pingPongHalfResultA;
			bgfx::TextureHandle pPongRT = RenResources.m_pingPongHalfResultB;

			// Generate
			{
				bgfx::setImage(6, blurPasses == 0 ? RenResources.m_finalResults : pPingRT, 0, bgfx::Access::Write, bgfx::TextureFormat::RG8);

				bgfx::setUniform(RenResources.u_rect, halfResRect);

				bgfx::setTexture(0, RenResources.s_viewspaceDepthSource, RenResources.m_halfDepths[pass], SAMPLER_POINT_CLAMP);
				bgfx::setTexture(1, RenResources.s_viewspaceDepthSourceMirror, RenResources.m_halfDepths[pass], SAMPLER_POINT_MIRROR);


				if (m_settings.m_generateNormals)
					bgfx::setImage(2, RenResources.m_normals, 0, bgfx::Access::Read, bgfx::TextureFormat::RGBA8);
				else
					bgfx::setImage(2, bgfx::getTexture(RenResources.m_gbuffer, GBUFFER_RT_NORMAL), 0, bgfx::Access::Read, bgfx::TextureFormat::RGBA8);

				if (!adaptiveBasePass && (m_settings.m_qualityLevel == 3))
				{
					bgfx::setBuffer(3, RenResources.m_loadCounter, bgfx::Access::Read);
					bgfx::setTexture(4, RenResources.s_importanceMap, RenResources.m_importanceMap, SAMPLER_LINEAR_CLAMP);
					bgfx::setImage(5, RenResources.m_finalResults, 0, bgfx::Access::Read, bgfx::TextureFormat::RG8);
				}

				bgfx::ProgramHandle programs[5] = { RenResources.m_generateQ0Program, RenResources.m_generateQ1Program , RenResources.m_generateQ2Program , RenResources.m_generateQ3Program , RenResources.m_generateQ3BaseProgram };
				int32_t programIndex = bx::max(0, (!adaptiveBasePass) ? (m_settings.m_qualityLevel) : (4));

				m_uniforms.m_layer = blurPasses == 0 ? (float)pass : 0.0f;
				m_uniforms.submit();
				bgfx::dispatch(view, programs[programIndex], halfResNumX, halfResNumY);
			}

			// Blur
			if (blurPasses > 0)
			{
				int32_t wideBlursRemaining = bx::max(0, blurPasses - 2);

				for (int32_t i = 0; i < blurPasses; i++)
				{
					bgfx::setViewFrameBuffer(view, BGFX_INVALID_HANDLE);
					bgfx::touch(view);

					m_uniforms.m_layer = ((i == (blurPasses - 1)) ? (float)pass : 0.0f);
					m_uniforms.submit();

					bgfx::setUniform(RenResources.u_rect, halfResRect);

					bgfx::setImage(0, i == (blurPasses - 1) ? RenResources.m_finalResults : pPongRT, 0, bgfx::Access::Write, bgfx::TextureFormat::RG8);
					bgfx::setTexture(1, RenResources.s_blurInput, pPingRT, m_settings.m_qualityLevel > 0 ? SAMPLER_POINT_MIRROR : SAMPLER_LINEAR_CLAMP);

					if (m_settings.m_qualityLevel > 0)
					{
						if (wideBlursRemaining > 0)
						{
							bgfx::dispatch(view, RenResources.m_smartBlurWideProgram, halfResNumX, halfResNumY);
							wideBlursRemaining--;
						}
						else
						{
							bgfx::dispatch(view, RenResources.m_smartBlurProgram, halfResNumX, halfResNumY);
						}
					}
					else
					{
						bgfx::dispatch(view, RenResources.m_nonSmartBlurProgram, halfResNumX, halfResNumY); // just for quality level 0 (and -1)
					}

					bgfx::TextureHandle temp = pPingRT;
					pPingRT = pPongRT;
					pPongRT = temp;
				}
			}
		}

		if (ssaoPass == 0 && m_settings.m_qualityLevel == 3)
		{	// Generate importance map
			m_uniforms.submit();
			bgfx::setImage(0, RenResources.m_importanceMap, 0, bgfx::Access::Write, bgfx::TextureFormat::R8);
			bgfx::setTexture(1, RenResources.s_finalSSAO, RenResources.m_finalResults, SAMPLER_POINT_CLAMP);
			bgfx::dispatch(view, RenResources.m_generateImportanceMapProgram, (RenResources.m_quarterSize[0] + 7) / 8, (RenResources.m_quarterSize[1] + 7) / 8);

			m_uniforms.submit();
			bgfx::setImage(0, RenResources.m_importanceMapPong, 0, bgfx::Access::Write, bgfx::TextureFormat::R8);
			bgfx::setTexture(1, RenResources.s_importanceMap, RenResources.m_importanceMap);
			bgfx::dispatch(view, RenResources.m_postprocessImportanceMapAProgram, (RenResources.m_quarterSize[0] + 7) / 8, (RenResources.m_quarterSize[1] + 7) / 8);

			bgfx::setBuffer(0, RenResources.m_loadCounter, bgfx::Access::ReadWrite);
			bgfx::dispatch(view, RenResources.m_loadCounterClearProgram, 1, 1);

			m_uniforms.submit();
			bgfx::setImage(0, RenResources.m_importanceMap, 0, bgfx::Access::Write, bgfx::TextureFormat::R8);
			bgfx::setTexture(1, RenResources.s_importanceMap, RenResources.m_importanceMapPong);
			bgfx::setBuffer(2, RenResources.m_loadCounter, bgfx::Access::ReadWrite);
			bgfx::dispatch(view, RenResources.m_postprocessImportanceMapBProgram, (RenResources.m_quarterSize[0] + 7) / 8, (RenResources.m_quarterSize[1] + 7) / 8);
			++view;
		}
	}

	// Apply
	{
		// select 4 deinterleaved AO textures (texture array)
		bgfx::setImage(0, RenResources.m_aoMap, 0, bgfx::Access::Write, bgfx::TextureFormat::R8);
		bgfx::setTexture(1, RenResources.s_finalSSAO, RenResources.m_finalResults);

		m_uniforms.submit();

		float rect[4] = { (float)RenResources.m_fullResOutScissorRect[0], (float)RenResources.m_fullResOutScissorRect[1], (float)RenResources.m_fullResOutScissorRect[2], (float)RenResources.m_fullResOutScissorRect[3] };
		bgfx::setUniform(RenResources.u_rect, rect);

		bgfx::ProgramHandle program;
		if (m_settings.m_qualityLevel < 0)
			program = RenResources.m_nonSmartHalfApplyProgram;
		else if (m_settings.m_qualityLevel == 0)
			program = RenResources.m_nonSmartApplyProgram;
		else
			program = RenResources.m_applyProgram;
		bgfx::dispatch(view, program, (RenResources.m_fullResOutScissorRect[2] - RenResources.m_fullResOutScissorRect[0] + 7) / 8,
			(RenResources.m_fullResOutScissorRect[3] - RenResources.m_fullResOutScissorRect[1] + 7) / 8);


		++view;
	}

	bgfx::setViewClear(view
		, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
		, 0x443355FF, 1.0f, 0
	);

	float ss_proj[16];
	bx::mtxOrtho(ss_proj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 100.0f, 0.0f, caps->homogeneousDepth);
	bgfx::setViewRect(view, 0, 0, screen.WIDTH, screen.HEIGHT);
	bgfx::setViewTransform(view, NULL, ss_proj);
	bgfx::setViewName(view, "Combine");

	bgfx::setTexture(0, RenResources.s_ao, RenResources.m_aoMap, SAMPLER_POINT_CLAMP);
	bgfx::setTexture(1, RenResources.s_color, bgfx::getTexture(RenResources.m_gbuffer, GBUFFER_RT_COLOR), SAMPLER_POINT_CLAMP);

	bgfx::setState(0
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_A
	);
	screenSpaceQuad((float)screen.WIDTH, (float)screen.HEIGHT, 0.0f, caps->originBottomLeft);
	bgfx::submit(view, RenResources.m_progssaoblurmerge);



	bgfx::frame();
}
