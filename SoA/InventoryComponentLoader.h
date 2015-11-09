//
// InventoryComponentLoader.h
// Seed of Andromeda
//
// Created by Benjamin Arnold on 8 Nov 2015
// Copyright 2014 Regrowth Studios
// All Rights Reserved
//
// Summary:
// Loads and saves inventory data.
//

#pragma once

#ifndef InventoryComponentLoader_h__
#define InventoryComponentLoader_h__

// Forward Declarations
struct InventoryComponent;

class InventoryComponentLoader {
public:
    static bool load(const nString& filePath, OUT InventoryComponent& inv);
    static bool save(const nString& filePath, const InventoryComponent& inv);

private:
    // Reads the header and returns the version. Will return 0 on fail.
    static ui32 loadHeader(const std::vector<ui8>& fileData);
    static bool loadV1(const std::vector<ui8>& fileData, OUT InventoryComponent& inv);
};

#endif // InventoryComponentLoader_h__
