#include "stdafx.h"
#include "Item.h"

ItemPack::ItemPack() :
    onItemDataAddition(this) {
    // Add default item
    append(ItemData());
}

ItemID ItemPack::append(ItemData item) {
    const ItemData* curBlock;
   
    ItemID id = m_itemList.size();
    item.id = id;
    // Add a new block
    m_itemList.push_back(item);
    // Set the correct index
    m_itemMap[item.name] = id;
    
    onItemDataAddition(item.id);
    return id;
}

void ItemPack::reserveID(const ItemIdentifier& sid, ItemID id) {
    if (id >= m_itemList.size()) m_itemList.resize(id + 1);
    m_itemMap[sid] = id;
    m_itemList[id].id = id;
}
