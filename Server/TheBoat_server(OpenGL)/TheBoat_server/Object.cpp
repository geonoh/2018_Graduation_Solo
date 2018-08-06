#include "stdafx.h"
#include "Object.h"

Object::Object()
{
	m_v3Position.x = 0.f;
	m_v3Position.y = 0.f;
	m_v3Position.z = 0.f;

	//bounding_box.Center.x = 0.f;
	//bounding_box.Center.y = 0.f;
	//bounding_box.Center.z = 0.f;
}
//void Object::FixOBB() {
//	XMFLOAT4 ori = XMFLOAT4{ 0,0,0,1 };
//	bounding_box = BoundingOrientedBox(position, obb_extents, ori);
//	//printf("오브젝트의 OBB : [%f, %f, %f] \n", xmExtents.x, xmExtents.y, xmExtents.z);
//}
//
//void Object::SetOBB(XMFLOAT3& input_pos, XMFLOAT3& extents) {
//	position = input_pos;
//	obb_extents = extents;
//}
//
void Object::SetPosition(float pos_x, float pos_y, float pos_z) {
	m_v3Position.x = pos_x;
	m_v3Position.y = pos_y;
	m_v3Position.z = pos_z;
}
//
//
//XMFLOAT3 Object::GetExtents() {
//	return bounding_box.Extents;
//}
//
//XMFLOAT3 Object::GetPosition() {
//	return position;
//
//}

Object::~Object()
{
}

