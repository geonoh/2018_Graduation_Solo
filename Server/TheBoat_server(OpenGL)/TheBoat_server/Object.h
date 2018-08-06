#pragma once

class Object
{
protected:
	glm::vec3 m_v3Position;
	//XMFLOAT3 obb_extents;	// Object OBB Size

	// 받아온 인자를 활용해 OBB고정
	//void FixOBB();
public:
	//BoundingOrientedBox bounding_box;
	//void SetOBB(XMFLOAT3& input_pos, XMFLOAT3& extents);
	////void SetOBB(float pos_x, float pos_y, float pos_z, XMFLOAT3& extents);
	//void SetPosition(XMFLOAT3& input_pos, XMFLOAT3& extents);
	void SetPosition(float pos_x, float pos_y, float pos_z);
	//XMFLOAT3 GetPosition();
	//XMFLOAT3 GetExtents();
	Object();
	~Object();
};