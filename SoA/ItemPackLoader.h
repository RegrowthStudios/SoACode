#pragma once;

class ItemPack;

class ItemPackLoader {
public:
    static bool load(const nString& filePath, OUT ItemPack& pack);
    static bool save(const nString& filePath, const ItemPack& pack);

private:
    // Reads the header and returns the version. Will return 0 on fail.
    static ui32 loadHeader(const std::vector<ui8>& fileData);
    static bool loadV1(const std::vector<ui8>& fileData, OUT ItemPack& pack);
};