#pragma once
#include <vector>
#include <string>

struct v_pos
{
	float X;
	float Y;
	float Z;
};

struct v_texST
{
	float S = 0;
	float T = 0;
};

struct v_color
{
	uint8_t r, g, b, a;
};

typedef std::vector<uint16_t> BoneIDs;
typedef std::vector<float> BoneWeights;

struct Joint
{
	uint16_t joint_id;
	float transform_matrix[16];
	std::vector<uint16_t> children;
};

typedef std::vector<uint16_t> BoneIDs;
typedef std::vector<float> BoneWeights;

struct v_rot
{
	float W, X, Y, Z;
};

struct anim_frame
{
	v_pos loc;
	v_rot rot;
};

struct animation
{
	float time;
	uint16_t num_joints;
	uint16_t steps;
	std::vector<std::vector<anim_frame>> frames; //[step][joint]
};

struct GameMesh
{
	//Arbitary Data
	//vector<Texture> textures;
	std::vector<unsigned int> triListIndices;
	std::vector<Joint> joints;
	//animations
	std::vector<animation> animations;


	//Vertex Indexed Data,
	std::vector<v_pos> pos;
	std::vector<v_pos> norm;
	std::vector<v_texST> tex;
	std::vector<v_color> color;
	std::vector<BoneIDs> boneIndices;
	std::vector<BoneWeights> BoneWeights;
};

GameMesh loadmesh(std::string fileName);