#include <fstream>
#include "loadMesh.h"

uint32_t readUint32(uint8_t* mem, uint32_t readindex)
{
	return (uint32_t)(mem[readindex]) | (uint32_t)(mem[readindex + 1] << 8) | (uint32_t)(mem[readindex + 2]) << 16 | (uint32_t)(mem[readindex + 3] << 24);
}

uint16_t readUint16(uint8_t * mem, uint32_t readindex)
{
	return (uint16_t)(mem[readindex]) | (uint32_t)(mem[readindex + 1] << 8);
}

float readFloat(uint8_t * mem, uint32_t readindex)
{
	float f;
	memcpy(&f, &mem[readindex], sizeof(f));
	return f;
}

GameMesh loadmesh(std::string fileName)
{
	GameMesh LoadMesh;

	std::ifstream fs;
	fs.open(fileName, std::ios::in | std::ios::binary);
	fs.seekg(0, std::ios::end);
	const size_t LEN = fs.tellg();
	fs.seekg(0, std::ios::beg);
	uint8_t * mem = new uint8_t[LEN];
	fs.read((char*)mem, LEN);
	fs.close();

	uint32_t readindex = 0;

	//load tri indicies
	if (mem[readindex] == uint8_t(0xAA))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.triListIndices.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			uint32_t index = readUint32(mem, readindex);
			readindex += 4;

			LoadMesh.triListIndices.push_back(index);

		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}

	//load vertex pos
	if (mem[readindex] == uint8_t(0xBB))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.pos.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			v_pos n_pos;

			n_pos.X = readFloat(mem, readindex);
			readindex += 4;
			n_pos.Y = readFloat(mem, readindex);
			readindex += 4;
			n_pos.Z = readFloat(mem, readindex);
			readindex += 4;

			LoadMesh.pos.push_back(n_pos);
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}

	//load vertex color
	if (mem[readindex] == uint8_t(0xCC))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.color.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			v_color n_color;

			n_color.r = mem[readindex];
			readindex++;
			n_color.g = mem[readindex];
			readindex++;
			n_color.b = mem[readindex];
			readindex++;
			n_color.a = mem[readindex];
			readindex++;

			LoadMesh.color.push_back(n_color);
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}

	//load UV/ST
	if (mem[readindex] == uint8_t(0xAB))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.color.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			v_texST n_texST;
			n_texST.S = readFloat(mem, readindex);
			readindex += 4;
			n_texST.T = readFloat(mem, readindex);
			readindex += 4;
			LoadMesh.tex.push_back(n_texST);
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}

	//load vertex normal
	if (mem[readindex] == uint8_t(0xAC))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.norm.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			v_pos n_pos;

			n_pos.X = readFloat(mem, readindex);
			readindex += 4;
			n_pos.Y = readFloat(mem, readindex);
			readindex += 4;
			n_pos.Z = readFloat(mem, readindex);
			readindex += 4;

			LoadMesh.norm.push_back(n_pos);
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}

	//load bone indicies
	if (mem[readindex] == uint8_t(0xDD))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.boneIndices.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			LoadMesh.boneIndices.push_back(BoneIDs());
			for (size_t j = 0; j < 4; j++)
			{
				uint16_t index = readUint16(mem, readindex);
				readindex += 2;
				LoadMesh.boneIndices[i].push_back(index);
			}
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}

	//load bone weights
	if (mem[readindex] == uint8_t(0xEE))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.BoneWeights.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			LoadMesh.BoneWeights.push_back(BoneWeights());
			for (size_t j = 0; j < 4; j++)
			{
				float weight = readFloat(mem, readindex);
				readindex += 4;
				LoadMesh.BoneWeights[i].push_back(weight);
			}
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}


	//load joints
	if (mem[readindex] == uint8_t(0xFF))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.joints.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			LoadMesh.joints.push_back(Joint());
			//read id
			LoadMesh.joints[i].joint_id = readUint16(mem, readindex);
			readindex += 2;

			//read matrix
			for (size_t j = 0; j < 16; j++)
			{
				LoadMesh.joints[i].transform_matrix[j] = readFloat(mem, readindex);
				readindex += 4;
			}

			//read num children
			uint16_t childrend = readUint16(mem, readindex);
			readindex += 2;
			//for num children
			for (size_t j = 0; j < childrend; j++)
			{
				uint16_t cid = readUint16(mem, readindex);
				readindex += 2;
				LoadMesh.joints[i].children.push_back(cid);
			}
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}



	if (mem[readindex] == uint8_t(0xA1))
	{
		readindex++;
		uint32_t size = readUint32(mem, readindex);
		readindex += 4;
		LoadMesh.animations.reserve(size);
		for (size_t i = 0; i < size; i++)
		{
			animation n_anm;
			n_anm.time = readFloat(mem, readindex);
			readindex += 4;
			n_anm.steps = readUint16(mem, readindex);
			readindex += 2;
			n_anm.num_joints = readUint16(mem, readindex);
			readindex += 2;

			for (size_t j = 0; j < n_anm.steps; j++)
			{
				std::vector<anim_frame> n_frames;
				for (size_t k = 0; k < n_anm.num_joints; k++)
				{
					anim_frame n_frame;

					n_frame.loc.X = readFloat(mem, readindex);
					readindex += 4;
					n_frame.loc.Y = readFloat(mem, readindex);
					readindex += 4;
					n_frame.loc.Z = readFloat(mem, readindex);
					readindex += 4;

					n_frame.rot.W = readFloat(mem, readindex);
					readindex += 4;
					n_frame.rot.X = readFloat(mem, readindex);
					readindex += 4;
					n_frame.rot.Y = readFloat(mem, readindex);
					readindex += 4;
					n_frame.rot.Z = readFloat(mem, readindex);
					readindex += 4;

					n_frames.push_back(n_frame);
				}
				n_anm.frames.push_back(n_frames);
			}
			LoadMesh.animations.push_back(n_anm);
		}

		uint8_t end = mem[readindex];
		readindex++;
		if (end != 0x00)
		{
			//error
		}
	}
	else
	{
		//error
	}

	delete[] mem;

	return LoadMesh;
}