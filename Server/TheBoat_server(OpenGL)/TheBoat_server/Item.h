#pragma once
#include "Object.h"
class Item :
	public Object
{
	char item_type;
public:
	bool in_use;
	Item();
	~Item();

	int GetItemType();
	void SetItemType(int);
};

