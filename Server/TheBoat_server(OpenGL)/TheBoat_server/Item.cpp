#include "stdafx.h"
#include "Item.h"


Item::Item()
{
	in_use = false;
	item_type = 0;
}


Item::~Item()
{
}

int Item::GetItemType() {
	return item_type;
}

void Item::SetItemType(int input_type) {
	item_type = input_type;
}