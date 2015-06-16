#include "stdafx.h"
#include "BlockTexturePack.h"

#include "Errors.h"

BlockTexturePack::~BlockTexturePack() {
    dispose();
}

void BlockTexturePack::init(ui32 resolution) {
    m_resolution = resolution;
    m_pageWidthPixels = m_resolution * m_stitcher.getTilesPerRow();
    // Calculate max mipmap level
    m_mipLevels = 0;
    int width = m_pageWidthPixels;
    while (width > m_stitcher.getTilesPerRow()) {
        width >>= 1;
        m_mipLevels++;
    }
}

// TODO(Ben): Lock?
void BlockTexturePack::addTexture(BlockTextureLayer& layer, color4* pixels) {
    // Map the texture
    if (layer.size.y > 1) {
        layer.index = m_stitcher.mapBox(layer.size.x, layer.size.y);
    } else if (layer.size.x > 1) {
        layer.index = m_stitcher.mapContiguous(layer.size.x);
    } else {
        layer.index = m_stitcher.mapSingle();
    }

    // Flag dirty pages and allocate new in ram if needed
    int firstPageIndex = layer.index % m_stitcher.getTilesPerPage();
    int lastPageIndex = (layer.index + (layer.size.y - 1) * m_stitcher.getTilesPerRow() + (layer.size.x - 1)) % m_stitcher.getTilesPerPage();
    flagDirtyPage(firstPageIndex);
    if (lastPageIndex != firstPageIndex) flagDirtyPage(lastPageIndex);

    // Map data
    switch (layer.method) {
        case ConnectedTextureMethods::CONNECTED:
            writeToAtlasContiguous(layer.index, pixels, 12, 4, 47);
            break;
        case ConnectedTextureMethods::RANDOM:
            writeToAtlasContiguous(layer.index, pixels, layer.numTiles, 1, layer.numTiles);
            break;
        case ConnectedTextureMethods::REPEAT:
            writeToAtlas(layer.index, pixels, m_resolution * layer.size.x, m_resolution * layer.size.y);
            break;
        case ConnectedTextureMethods::GRASS:
            writeToAtlasContiguous(layer.index, pixels, 3, 3, 9);
            break;
        case ConnectedTextureMethods::HORIZONTAL:
            writeToAtlasContiguous(layer.index, pixels, 4, 1, 4);
            break;
        case ConnectedTextureMethods::VERTICAL:
            writeToAtlasContiguous(layer.index, pixels, 1, 4, 4);
            break;
        case ConnectedTextureMethods::FLORA:
            writeToAtlasContiguous(layer.index, pixels, layer.size.x, layer.size.y, layer.numTiles);
            break;
        default:
            writeToAtlas(layer.index, pixels, m_resolution, m_resolution);
            break;
    }

    // Cache the texture description
    AtlasTextureDescription tex;
    tex.index = layer.index;
    tex.size = layer.size;
    m_lookupMap[layer.path] = tex;
}

AtlasTextureDescription BlockTexturePack::findTexture(const nString& filePath) {
    auto& it = m_lookupMap.find(filePath);
    if (it != m_lookupMap.end()) {
        return it->second;
    }
    return {};
}

void BlockTexturePack::update() {
    bool needsMipmapGen = false;
    if (m_needsRealloc) {
        allocatePages();
        m_needsRealloc = false;
        needsMipmapGen = true;
        // Upload all pages
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_atlasTexture);
        for (int i = 0; i < m_pages.size(); i++) {
            uploadPage(i);
        }
        std::vector<int>().swap(m_dirtyPages);
    } else if (m_dirtyPages.size()) {
        needsMipmapGen = true;
        // Upload dirty pages
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_atlasTexture);
        for (auto& i : m_dirtyPages) {
            uploadPage(i);
        }
        std::vector<int>().swap(m_dirtyPages);
    }

    if (needsMipmapGen) {
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }
}

void BlockTexturePack::dispose() {
    if (m_atlasTexture) glDeleteTextures(1, &m_atlasTexture);
    m_atlasTexture = 0;
    for (auto& p : m_pages) {
        delete[] p.pixels;
    }
    std::vector<AtlasPage>().swap(m_pages);
    m_stitcher.dispose();
}

void BlockTexturePack::flagDirtyPage(ui32 pageIndex) {
    // If we need to allocate new pages, do so
    if (pageIndex > m_pages.size()) {
        int i = m_pages.size();
        m_pages.resize(pageIndex + 1);
        for (; i < m_pages.size(); i++) {
            m_pages[i].pixels = new color4[m_pageWidthPixels * m_pageWidthPixels];
        }
        m_needsRealloc = true;    
    }
    m_pages[pageIndex].dirty = true;
}

void BlockTexturePack::allocatePages() {
    // Set up the storage
    if (!m_atlasTexture) glGenTextures(1, &m_atlasTexture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_atlasTexture);

    // Set up all the mipmap storage
    ui32 width = m_pageWidthPixels;
    for (ui32 i = 0; i < m_mipLevels; i++) {
        glTexImage3D(GL_TEXTURE_2D_ARRAY, i, GL_RGBA8, width, width, m_pages.size(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
        width >>= 1;
        if (width < 1) width = 1;
    }

    // Set up tex parameters
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, (int)m_mipLevels);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LOD, (int)m_mipLevels);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);

    // Unbind
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    // Check if we had any errors
    checkGlError("BlockTexturePack::allocatePages");
}

void BlockTexturePack::uploadPage(ui32 pageIndex) {
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, pageIndex, m_pageWidthPixels, m_pageWidthPixels, 1, GL_RGBA, GL_UNSIGNED_BYTE, m_pages[pageIndex].pixels);
}

void BlockTexturePack::writeToAtlas(BlockTextureIndex texIndex, color4* pixels, ui32 pixelWidth, ui32 pixelHeight) {

    // Get the location in the array
    ui32 i = texIndex % m_stitcher.getTilesPerPage();
    ui32 dx = i % m_stitcher.getTilesPerRow();
    ui32 dy = i / m_stitcher.getTilesPerRow();
    ui32 pageIndex = texIndex / m_stitcher.getTilesPerPage();

    // Temp variables to reduce multiplications
    ui32 destOffset;
    ui32 pixelsOffset;
    ui32 yDestOffset;
    ui32 yPixelsOffset;

    // Start of destination
    color4* dest = m_pages[pageIndex].pixels + dx * m_resolution + dy * m_resolution * m_pageWidthPixels;
    float alpha;

    // Copy the block of pixels
    for (ui32 y = 0; y < pixelHeight; y++) {
        // Calculate Y offsets
        yDestOffset = y * m_pageWidthPixels;
        yPixelsOffset = y * m_pageWidthPixels;
        // Need to do alpha blending for every pixel against a black background
        for (ui32 x = 0; x < pixelWidth; x++) {
            // Calculate offsets
            destOffset = yDestOffset + x;
            pixelsOffset = yPixelsOffset + x;
            // Convert 0-255 to 0-1 for alpha mult
            alpha = (float)pixels[pixelsOffset].a / 255.0f;

            // Set the colors. Add  + 0.01f to make sure there isn't any rounding error when we truncate
            dest[destOffset].r = (ui8)((float)pixels[pixelsOffset].r * alpha + 0.01f); // R
            dest[destOffset].g = (ui8)((float)pixels[pixelsOffset].g * alpha + 0.01f); // G
            dest[destOffset].b = (ui8)((float)pixels[pixelsOffset].b * alpha + 0.01f); // B
            dest[destOffset].a = pixels[pixelsOffset].a; // A 
        }
    }
}

void BlockTexturePack::writeToAtlasContiguous(BlockTextureIndex texIndex, color4* pixels, ui32 width, ui32 height, ui32 numTiles) {
    ui32 pixelsPerTileRow = m_resolution * m_pageWidthPixels;

    ui32 n = 0;
    for (ui32 y = 0; y < height; y++) {
        for (ui32 x = 0; x < width && n < numTiles; x++, n++) {
            // Get pointer to source data
            color4* src = pixels + y * pixelsPerTileRow + x * m_resolution;

            writeToAtlas(texIndex++, src, m_resolution, m_resolution);
        }
    }
}