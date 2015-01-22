#pragma once

#include <Vorb/Events.hpp>
#include <Vorb/VorbPreDecl.inl>

#include "BlockData.h"
#include "CAEngine.h"

DECL_VIO(class IOManager)

class BlockPack;
class TexturePackLoader;


class GameBlockPostProcess : public IDelegate<ui16> {
public:
    GameBlockPostProcess(const vio::IOManager* iom, TexturePackLoader* tpl, CaPhysicsTypeDict* caCache);

    virtual void invoke(Sender s, ui16 id) override;
private:
    TexturePackLoader* m_texPackLoader; ///< Texture pack loader
    const vio::IOManager* m_iom; ///< IO workspace
    CaPhysicsTypeDict* m_caCache; ///< CA type cache
};

class BlockLoader
{
public:
    /// Loads blocks from a .yml file
    /// @param filePath: The .yml file path
    /// @param pack: Depository for all loaded blocks
    /// @return true on success, false on failure
    static bool loadBlocks(const nString& filePath, BlockPack* pack);

    /// Loads blocks from a .yml file
    /// @param iom: IO workspace
    /// @param filePath: The .yml file path
    /// @param pack: Depository for all loaded blocks
    /// @return true on success, false on failure
    static bool load(const vio::IOManager* iom, const cString filePath, BlockPack* pack);

    /// Saves blocks to a .yml file
    /// @param filePath: The .yml file path
    /// @param pack: Source of block data
    /// @return true on success, false on failure
    static bool saveBlocks(const nString& filePath, BlockPack* pack);
private:
    /// Sets up the water blocks. This is temporary
    /// @param blocks: Output list for blocks
    static void SetWaterBlocks(std::vector<Block>& blocks);
};

