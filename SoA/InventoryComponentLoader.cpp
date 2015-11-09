#include "stdafx.h"
#include "InventoryComponentLoader.h"

#include "GameSystemComponents.h"
#include "Item.h"

#include <Vorb/io/IOManager.h>
#include <Vorb/utils.h>
#include <fstream>

const ui32 INV_HEADER_ID = 0x12345678;

const ui32 INV_LOADER_VERSION_NONE = 0;
const ui32 INV_LOADER_VERSION_1 = 1;
const ui32 INV_LOADER_VERSION = INV_LOADER_VERSION_1;

// Code reduction macros
#define WRITE_INT(i) BufferUtils::setInt(buffer, (i)); file.write((char*)buffer, (i))
#define READ_INT(i) BufferUtils::extractInt(&fileData[0] + (i))

bool InventoryComponentLoader::load(const nString& filePath, OUT InventoryComponent& inv) {
    // TODO(Ben): Maybe pass in iomanager.
    std::vector<ui8> fileData;
    if (!vio::IOManager().readFileToData(filePath, fileData)) return false;
    
    ui32 version = loadHeader(fileData);

    switch (version) {
        case INV_LOADER_VERSION_1:
            return loadV1(fileData, inv);
            break;
        default:
            return false;
    }

    return true;
}

bool InventoryComponentLoader::save(const nString& filePath, const InventoryComponent& inv) {    

    ui8 buffer[8];

    std::ofstream file(filePath, std::ios::binary);
    if (file.fail()) return false;

    // Write header
    WRITE_INT(INV_HEADER_ID);

    // Write version
    WRITE_INT(INV_LOADER_VERSION);

    // Check for version issues
    static_assert(sizeof(ItemStack().id) == 4, "Wrong sized ItemStack::id");
    static_assert(sizeof(ItemStack().count) == 4, "Wrong sized ItemStack::count");
    static_assert(sizeof(ItemStack().durability) == 4, "Wrong sized ItemStack::durability");

    // Write the number of items
    WRITE_INT(inv.items.size());

    // Write all the items
    for (const ItemStack& i : inv.items) {
        WRITE_INT(i.id);
        WRITE_INT(i.count);
        WRITE_INT(i.durability);
    }

    return true;
}

ui32 InventoryComponentLoader::loadHeader(const std::vector<ui8>& fileData) {
    // Check the header
    if (READ_INT(0) != INV_HEADER_ID) return 0;
    // Return the version
    return READ_INT(4);
}

bool InventoryComponentLoader::loadV1(const std::vector<ui8>& fileData, OUT InventoryComponent& inv) {
    ui32 filePos = 8;
    ui32 numItems = READ_INT(filePos);
    filePos += sizeof(ui32);

    for (ui32 i = 0; i < numItems; i++) {
        if (filePos + 12 > fileData.size()) return false;
        ItemStack item;
        item.id = READ_INT(filePos);
        filePos += sizeof(ui32);
        item.count = READ_INT(filePos);
        filePos += sizeof(ui32);
        item.durability = READ_INT(filePos);
        filePos += sizeof(ui32);

        if (item.id > inv.items.size()) inv.items.resize(item.id + 1);
        inv.items[item.id] = item;
    }

    return true;
}
