#pragma once
#include <bgfx/bgfx.h>
#include "loadMesh.h"

struct VertexData {
	float m_x;
	float m_y;
	float m_z;
	float m_normal_x;
	float m_normal_y;
	float m_normal_z;
	uint32_t m_abgr;
	float m_s;
	float m_t;

	static void init() {
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
			.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
			.end();
	};

	static bgfx::VertexDecl ms_decl;
};



struct Model
{
	bgfx::VertexBufferHandle vbh;
	bgfx::IndexBufferHandle ibh;
	bgfx::TextureHandle texh;
	float* matrixTransform;
};

struct Animation
{
	int animation_id;
	float time;

};

struct postition
{
	float x, y, z;
};

struct velocity
{
	float x, y, z;
};

Model LoadModel(GameMesh testMesh);