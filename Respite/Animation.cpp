#include "Animation.h"




void buildposetransforms(std::vector<Joint> joints, std::vector<m_pose> pose_transforms, std::vector<m_pose>& new_pose_transforms, unsigned int current)
{
	for (size_t i = 0; i < joints[current].children.size(); i++)
	{
		uint16_t childID = joints[current].children[i];
		bx::mtxMul(new_pose_transforms[childID].matrix, pose_transforms[childID].matrix, new_pose_transforms[current].matrix);
		buildposetransforms(joints, pose_transforms, new_pose_transforms, childID);
	}
}

bx::Quaternion quatSub(bx::Quaternion q1, bx::Quaternion q2)
{
	bx::Quaternion result;

	result.x = q1.x - q2.x;
	result.y = q1.y - q2.y;
	result.z = q1.z - q2.z;
	result.w = q1.w - q2.w;

	return result;
}

bx::Quaternion quatAdd(bx::Quaternion q1, bx::Quaternion q2)
{
	bx::Quaternion result;

	result.x = q1.x + q2.x;
	result.y = q1.y + q2.y;
	result.z = q1.z + q2.z;
	result.w = q1.w + q2.w;
	return result;
}

bx::Quaternion quatMul(float t, bx::Quaternion q2)
{
	bx::Quaternion result;

	result.x = t * q2.x;
	result.y = t * q2.y;
	result.z = t * q2.z;
	result.w = t * q2.w;
	return result;
}

bx::Quaternion slerp(bx::Quaternion v0, bx::Quaternion v1, double t) {
	// Only unit quaternions are valid rotations.
	// Normalize to avoid undefined behavior.
	v0 = bx::normalize(v0);
	v1 = bx::normalize(v1);

	// Compute the cosine of the angle between the two vectors.
	float dot = bx::dot(v0, v1);

	// If the dot product is negative, slerp won't take
	// the shorter path. Note that v1 and -v1 are equivalent when
	// the negation is applied to all four components. Fix by 
	// reversing one quaternion.
	if (dot < 0.0f) {
		bx::invert(v1);
		dot = -dot;
	}

	const double DOT_THRESHOLD = 0.9995;
	if (dot > DOT_THRESHOLD) {
		// If the inputs are too close for comfort, linearly interpolate
		// and normalize the result.

		bx::Quaternion result = quatAdd(v0 , quatMul(t , quatSub(v1,v0)));
		result = bx::normalize(result);
		return result;
	}

	// Since dot is in range [0, DOT_THRESHOLD], acos is safe
	double theta_0 = acos(dot);        // theta_0 = angle between input vectors
	double theta = theta_0 * t;          // theta = angle between v0 and result
	double sin_theta = sin(theta);     // compute this value only once
	double sin_theta_0 = sin(theta_0); // compute this value only once

	double s0 = cos(theta) - dot * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
	double s1 = sin_theta / sin_theta_0;

	return quatAdd(quatMul(s0 , v0) , quatMul(s1 , v1));
}

void rebuild_vbuff(VertexData* test_verticies, GameMesh testMesh, float aniframe, m_pose& hand) //pass in some animation state info too probabaly
{
	unsigned int size = testMesh.pos.size();
	std::vector<m_pose> pose_transforms;

	int num_frames = testMesh.animations[0].frames.size()-1;
	float frame_pos = num_frames * aniframe;
	int frame1 = bx::floor(num_frames * aniframe);
	int frame2 = bx::ceil(num_frames * aniframe);
	float slurpage = frame_pos - (float)frame1;

	//quaternions to matrix
	for (size_t i = 0; i < testMesh.joints.size(); i++)
	{
		anim_frame root = testMesh.animations[0].frames[frame1][i];
		bx::Quaternion quat1;
		quat1.w = root.rot.W;
		quat1.x = root.rot.X;
		quat1.y = root.rot.Y;
		quat1.z = root.rot.Z;

		anim_frame root2 = testMesh.animations[0].frames[frame2][i];
		bx::Quaternion quat2;
		quat2.w = root2.rot.W;
		quat2.x = root2.rot.X;
		quat2.y = root2.rot.Y;
		quat2.z = root2.rot.Z;

		bx::Quaternion quat = slerp(quat1, quat2, slurpage);

		float matrix[16];
		bx::mtxQuat(matrix, quat);
		float t_matrix[16];
		bx::mtxTranspose(t_matrix, matrix);
		t_matrix[12] = root.loc.X;
		t_matrix[13] = root.loc.Y;
		t_matrix[14] = root.loc.Z;

		m_pose temp;
		for (size_t j = 0; j < 16; j++)
		{
			temp.matrix[j] = t_matrix[j];
		}

		pose_transforms.push_back(temp);
	}

	std::vector<m_pose> w_pose_transforms;
	for (size_t i = 0; i < testMesh.joints.size(); i++)
	{
		w_pose_transforms.push_back(m_pose());
	}



	float origin[16] = { -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 1.741501, 0, 0, 0, 1 };
	float t_origin[16];
	bx::mtxTranspose(t_origin, origin);

	float new_waist_pose[16];
	bx::mtxMul(new_waist_pose, pose_transforms[0].matrix, t_origin);

	for (size_t i = 0; i < 16; i++)
	{
		pose_transforms[0].matrix[i] = new_waist_pose[i];
	}

	w_pose_transforms[0] = pose_transforms[0];
	buildposetransforms(testMesh.joints, pose_transforms, w_pose_transforms, 0);


	std::vector<m_pose> ws_v_pose;
	for (size_t i = 0; i < testMesh.joints.size(); i++)
	{
		m_pose n;
		bx::mtxMul(n.matrix, testMesh.joints[i].transform_matrix, w_pose_transforms[i].matrix);
		ws_v_pose.push_back(n);
	}


	//grab out the transform for the hand and apply to equipment

	hand = w_pose_transforms[6]; //probs hand transform
	//bx::mtxTranslate(hand.matrix, h_pos_x, h_pos_y, h_pos_z);
	//bx::mtxMul(hand.matrix, ws_v_pose[6].matrix, handpos);
	//float mtx[16];
	//bx::mtxRotateZ(mtx, bx::kPi);
	//bx::mtxMul(hand.matrix, mtx, hand.matrix);

	//bx::mtxRotateX(mtx, bx::kPi);
	//bx::mtxMul(hand.matrix, mtx, hand.matrix);


	
	for (size_t i = 0; i < size; i++)
	{
		BoneIDs boneids = testMesh.boneIndices[i];
		BoneWeights boneweights = testMesh.BoneWeights[i];
		v_pos pos = testMesh.pos[i];
		bx::Vec3 c_pos(pos.X, pos.Y, pos.Z);
		bx::Vec3 f_pos(0, 0, 0);
		for (size_t j = 0; j < 4; j++)
		{
			m_pose bone_transform = ws_v_pose[boneids[j]];
			float weight = boneweights[j];
			bx::Vec3 newpos = bx::mul(c_pos, bone_transform.matrix);
			newpos.x *= weight;
			newpos.y *= weight;
			newpos.z *= weight;
			f_pos = bx::add(newpos, f_pos);
		}

		VertexData pcv;
		pcv.m_x = f_pos.x;
		pcv.m_y = f_pos.y;
		pcv.m_z = f_pos.z;
		pcv.m_normal_x = testMesh.norm[i].X;
		pcv.m_normal_y = testMesh.norm[i].Y;
		pcv.m_normal_z = testMesh.norm[i].Z;
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
}