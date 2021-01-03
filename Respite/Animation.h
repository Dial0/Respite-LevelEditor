#pragma once
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include "loadMesh.h"
#include "Model.h"

struct m_pose
{
	float matrix[16];
};

void rebuild_vbuff(VertexData* test_verticies, GameMesh testMesh, float aniframe, m_pose& hand);