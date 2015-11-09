#include "stdafx.h"
#include "ItemPackLoader.h"

#include "Item.h"

#include <Vorb/io/IOManager.h>
#include <Vorb/utils.h>
#include <fstream>

const ui32 IPACK_HEADER_ID = 0x13572468;

const ui32 IPACK_LOADER_VERSION_NONE = 0;
const ui32 IPACK_LOADER_VERSION_1 = 1;
const ui32 IPACK_LOADER_VERSION = IPACK_LOADER_VERSION_1;

// Code reduction macros
#define WRITE_INT(i) BufferUtils::setInt(buffer, (i)); file.write((char*)buffer, (i))
#define WRITE_FLOAT(i) BufferUtils::setFloat(buffer, (i)); file.write((char*)buffer, (i))
#define READ_INT(i) BufferUtils::extractInt(&fileData[0] + (i))
#define READ_FLOAT(i) BufferUtils::extractFloat(&fileData[0] + (i))

bool ItemPackLoader::load(const nString& filePath, OUT ItemPack& pack) {
    // TODO(Ben): Maybe pass in iomanager.
    std::vector<ui8> fileData;
    if (!vio::IOManager().readFileToData(filePath, fileData)) return false;

    ui32 version = loadHeader(fileData);

    switch (version) {
        case IPACK_LOADER_VERSION_1:
            return loadV1(fileData, pack);
            break;
        default:
            return false;
    }

    return true;
}

bool ItemPackLoader::save(const nString& filePath, const ItemPack& pack) {
    ui8 buffer[256]; // Big enough to hold big string names.

    std::ofstream file(filePath, std::ios::binary);
    if (file.fail()) return false;

    // Write header
    WRITE_INT(IPACK_HEADER_ID);

    // Write version
    WRITE_INT(IPACK_LOADER_VERSION);

    const std::vector<ItemData>& itemdata = pack.getItemDataList();

    // Write the name of the pack
    sprintf((char*)buffer, "%s", pack.name.c_str());
    buffer[pack.name.size()] = '\0'; // Write newline
    file.write((char*)buffer, pack.name.size() + 1);

    // Write the number of items
    WRITE_INT(itemdata.size());

    // Write all the items
    for (const ItemData& i : itemdata) {
        // Write name
        sprintf((char*)buffer, "%s", i.name.c_str());
        buffer[i.name.size()] = '\0'; // Write newline
        file.write((char*)buffer, i.name.size() + 1);
        
        WRITE_INT((ui32)i.type);
        WRITE_INT(i.id);
        WRITE_FLOAT(i.value);
        WRITE_FLOAT(i.weight);
        WRITE_INT(i.maxCount);
        // Its a union of the same type so it doesn't matter
        WRITE_INT(i.maxDurability);
    }

    return true;
}

ui32 ItemPackLoader::loadHeader(const std::vector<ui8>& fileData) {
    // Check the header
    if (READ_INT(0) != IPACK_HEADER_ID) return 0;
    // Return the version
    return READ_INT(4);
}

bool ItemPackLoader::loadV1(const std::vector<ui8>& fileData, OUT ItemPack& pack) {
    ui32 filePos = 8;

    // Read the name of pack
    pack.name = std::string((char*)fileData[filePos]);
    filePos += pack.name.size() + 1; //+1 for null terminator

    // Read the number of items
    ui32 numItems = READ_INT(filePos);
    filePos += sizeof(ui32);

    for (ui32 i = 0; i < numItems; i++) {
        ItemData item;
        // Read the string name
        item.name = std::string((char*)fileData[filePos]);
        filePos += item.name.size() + 1; //+1 for null terminator

        item.type = (ItemType)READ_INT(filePos);
        filePos += sizeof(ui32);
        item.id = READ_INT(filePos);
        filePos += sizeof(ui32);
        item.value = READ_FLOAT(filePos);
        filePos += sizeof(f32);
        item.weight = READ_FLOAT(filePos);
        filePos += sizeof(f32);
        item.maxCount = READ_INT(filePos);
        filePos += sizeof(ui32);

        // Its a union of the same type so it doesn't matter
        item.maxDurability = READ_INT(filePos);
        filePos += sizeof(ui32);

        // Add the item
        pack.reserveID(item.name, item.id);
        pack.append(item);
    }

    return true;
}
