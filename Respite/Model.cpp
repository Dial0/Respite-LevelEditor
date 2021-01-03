#include "Model.h"
#include "loadMesh.h"
#include "stb_image.h"

bgfx::VertexDecl VertexData::ms_decl;


Model LoadModel(GameMesh testMesh)
{
	unsigned int size = testMesh.pos.size();
	int x, y, n;
	unsigned char* image = stbi_load("KnightTEX.png", &x, &y, &n, 0);
	const bgfx::Memory* mem_image = bgfx::makeRef(image, x * y * n);
	bgfx::TextureHandle texture = bgfx::createTexture2D(x, y, false, 1, bgfx::TextureFormat::RGBA8, 0 | BGFX_TEXTURE_RT
		| BGFX_SAMPLER_MIN_POINT
		| BGFX_SAMPLER_MAG_POINT
		| BGFX_SAMPLER_MIP_POINT
		| BGFX_SAMPLER_U_CLAMP
		| BGFX_SAMPLER_V_CLAMP, mem_image);

	VertexData::init();

	VertexData* test_verticies = new VertexData[size];

	

	for (size_t i = 0; i < size; i++)
	{
		BoneIDs boneids = testMesh.boneIndices[i];
		BoneWeights boneweights = testMesh.BoneWeights[i];
		v_pos pos = testMesh.pos[i];


		VertexData pcv;
		pcv.m_x = pos.X;
		pcv.m_y = pos.Y;
		pcv.m_z = pos.Z;
		pcv.m_normal_x = -testMesh.norm[i].X;
		pcv.m_normal_y = -testMesh.norm[i].Y;
		pcv.m_normal_z = -testMesh.norm[i].Z;
		if ((i % 3) == 0)
		{
			pcv.m_abgr = 0x00;
		}
		else
		{
			pcv.m_abgr = 0xFFFFFFFF;
		}

		pcv.m_s = testMesh.tex[i].S;
		pcv.m_t = testMesh.tex[i].T;

		test_verticies[i] = pcv;
		

	}


	bgfx::VertexBufferHandle vbh = bgfx::createVertexBuffer(
		// Static data can be passed with bgfx::makeRef
		bgfx::makeRef(test_verticies, sizeof(VertexData) * size),
		VertexData::ms_decl
	);

	unsigned int tri_size = testMesh.triListIndices.size();
	uint16_t* test_trilist = new uint16_t[tri_size];
	for (size_t i = 0; i < tri_size; i++)
	{
		test_trilist[i] = (uint16_t)testMesh.triListIndices[i];
	}


	bgfx::IndexBufferHandle ibh = bgfx::createIndexBuffer(
		// Static data can be passed with bgfx::makeRef
		bgfx::makeRef(test_trilist, sizeof(uint16_t) * tri_size)
	);

	Model r_model;

	r_model.texh = texture;
	r_model.ibh = ibh;
	r_model.vbh = vbh;

	return r_model;
}