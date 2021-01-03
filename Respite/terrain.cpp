#include "Model.h"
#include <stdlib.h>
#include <bx/math.h>

VertexData* generate_terrain(int sizeX, int sizeY, float gridSize)
{
	//create a X by Y array
	float* height_map = new float[sizeX * sizeY];

	//assign that array a height map
	for (size_t y = 0; y < sizeY; y++)
	{
		for (size_t x = 0; x < sizeX; x++)
		{
			int i = x + sizeX * y;
			//float height = 10 * sin(2 * 3.14f * 200 * y);
			float height = 0;// (float)(rand() % 100) / 20.0f;


			
			height_map[i] = height; //change this to our height algorithm
		}
	}
	
	int num_verts = (sizeX - 1) * (sizeY - 1) * 6;
	//build mesh from that array
	VertexData* v_terrain = new VertexData[num_verts];
	int i_vert = 0;

	int polys = num_verts / 6;

	int poly_per_row = sizeX - 1;

	//top tri
	int t_tri_x[3] = { 0,1,0 };
	int t_tri_y[3] = { 0,0,1 };

	//bot tri
	int b_tri_x[3] = { 1,1,0 };
	int b_tri_y[3] = { 0,1,1 };

	//loop for polys
	for (size_t i = 0; i < polys; i++)
	{
		int pol_x = i % poly_per_row;
		int pol_y = i / poly_per_row;

		//calculate normal here, at the poly/square/grid level then apply to all verts in proceeding loops

		//do both tris that make up the poly
		for (size_t j = 0; j < 2; j++)
		{
			//vertices in each tri
			bx::Vec3 tri_verts[3];
			for (size_t k = 0; k < 3; k++)
			{
				int v_pos_x, v_pos_y;
				if (j == 0) //top tri
				{
					v_pos_x = t_tri_x[k] + pol_x;
					v_pos_y = t_tri_y[k] + pol_y;

					v_terrain[i_vert].m_s = (float)t_tri_x[k];
					v_terrain[i_vert].m_t = (float)t_tri_y[k];

				}
				else //bottom tri
				{
					v_pos_x = b_tri_x[k] + pol_x;
					v_pos_y = b_tri_y[k] + pol_y;

					v_terrain[i_vert].m_s = (float)b_tri_x[k];
					v_terrain[i_vert].m_t = (float)b_tri_y[k];
				}

				v_terrain[i_vert].m_x = (v_pos_x * gridSize) - (gridSize * ((sizeX-1) / 2.0f));
				v_terrain[i_vert].m_y = (v_pos_y * gridSize) - (gridSize * ((sizeY-1) / 2.0f));
				int i_heightMap = v_pos_x + sizeX * v_pos_y;
				v_terrain[i_vert].m_z = height_map[i_heightMap];


				//Calculate normal

				int Zleft_i = (v_pos_x-1) + sizeX * v_pos_y;
				int Zright_i = (v_pos_x + 1) + sizeX * v_pos_y;
				int Zupright_i = (v_pos_x + 1) + sizeX * (v_pos_y -1);
				int Zdownleft_i = (v_pos_x - 1) + sizeX * (v_pos_y + 1);
				int Zup_i = v_pos_x + sizeX * (v_pos_y - 1);
				int Zdown_i = v_pos_x + sizeX * (v_pos_y + 1);

				int adj_verts_i[6] = { Zleft_i ,Zright_i ,Zupright_i ,Zdownleft_i ,Zup_i ,Zdown_i };

				for (size_t l = 0; l < 6; l++)
				{
					if (adj_verts_i[l] >= (sizeX * sizeY) || adj_verts_i[l] < 0)
					{
						adj_verts_i[l] = i_heightMap;
					}
				}

				float Zleft = height_map[adj_verts_i[0]];
				float Zright = height_map[adj_verts_i[1]];
				float Zupright = height_map[adj_verts_i[2]];
				float Zdownleft = height_map[adj_verts_i[3]];
				float Zup = height_map[adj_verts_i[4]];
				float Zdown = height_map[adj_verts_i[5]];


				float ax = gridSize;
				float ay = gridSize;
				bx::Vec3 V_norm;
				V_norm.x = -((2 * (Zleft - Zright) - Zupright + Zdownleft + Zup - Zdown)) / ax;
				V_norm.y = -(2 * (Zdown - Zup) + Zupright + Zdownleft - Zup - Zleft) / ay;
				V_norm.z = 6;

				V_norm = bx::normalize(V_norm);
				

				v_terrain[i_vert].m_normal_x = -V_norm.x;
				v_terrain[i_vert].m_normal_y = -V_norm.y;
				v_terrain[i_vert].m_normal_z = -V_norm.z;

				i_vert++;
			}
		}
	}

	delete[] height_map;
	return v_terrain;
}