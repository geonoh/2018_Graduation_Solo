#include "stdafx.h"
#include "Object.h"

Object::Object()
{
	position.x = 0.f;
	position.y = 0.f;
	position.z = 0.f;

	bounding_box.Center.x = 0.f;
	bounding_box.Center.y = 0.f;
	bounding_box.Center.z = 0.f;
}
void Object::SetOBB(XMFLOAT3 xmCenter, XMFLOAT3 xmExtents, XMFLOAT4 xmOrientation) {
	bounding_box = BoundingOrientedBox(xmCenter, xmExtents, xmOrientation);
	//printf("오브젝트의 OBB : [%f, %f, %f] \n", xmExtents.x, xmExtents.y, xmExtents.z);
}


void Object::SetPosition(XMFLOAT3& input_pos, XMFLOAT3& extents) {
	SetOBB(input_pos, extents, XMFLOAT4(0, 0, 0, 1));
	position.x = input_pos.x;
	position.y = input_pos.y + 100;
	position.z = input_pos.z;

}

XMFLOAT3 Object::GetExtents() {
	return bounding_box.Extents;
}

XMFLOAT3 Object::GetPosition() {
	return position;

}

Object::~Object()
{
}

