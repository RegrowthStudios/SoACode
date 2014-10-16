#pragma once
#include "LoadMonitor.h"
#include "BlockData.h"
#include "FileSystem.h"
#include "GameManager.h"
#include "Player.h"

// This is hacky and temporary, it does way to much
class LoadBlockDataTask : public ILoadTask {
    virtual void load() {
        initConnectedTextures();

        if (!(fileManager.loadBlocks("Data/BlockData.ini"))) exit(123432);
        //    cout << SDL_GetTicks() - stt << endl;
        fileManager.saveBlocks("Data/test.ini");

        LoadTextures();

        SetBlockAvgTexColors();

        // Hacky shit.
        GameManager::player = new Player();
        Player* player = GameManager::player;

#define SPEEDMOD 0.0095f
        player->setMoveSpeed(SPEEDMOD, 0.166f);

        // Initialize objects
        for (size_t i = 1; i < Blocks.size(); i++){
            if (Blocks[i].active && !(i >= LOWWATER && i < FULLWATER || i > FULLWATER)){
                if (ObjectList[i] != NULL){
                    printf("Object ID %d already taken by %s. Requested by %s.\n", i, ObjectList[i]->name.c_str(), Blocks[i].name.c_str());
                    int a;
                    cin >> a;
                    exit(198);
                }
                ObjectList[i] = new Item(i, 1, Blocks[i].name, ITEM_BLOCK, Blocks[i].weight, 0, Blocks[i].value);
                player->inventory.push_back(new Item(i, 9999999, Blocks[i].name, ITEM_BLOCK, Blocks[i].weight, 0, Blocks[i].value));
            }
        }

        //for use in pressure explosions
        Blocks[VISITED_NODE] = Blocks[NONE];
        Blocks[VISITED_NODE].ID = VISITED_NODE;
        Blocks[VISITED_NODE].name = "Visited Node";
    }
};