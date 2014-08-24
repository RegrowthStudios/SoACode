#pragma once
#include "MainGame.h"

class IGameScreen;

class ScreenList {
public:
    ScreenList(MainGame* g);

    IGameScreen* getCurrent() const {
        if (_current < 0 || _current >= _screens.size()) return nullptr;
        else return _screens[_current];
    }
    IGameScreen* moveNext();
    IGameScreen* movePrevious();

    void setScreen(i32 s);
    ScreenList* addScreens(IGameScreen** s, i32 c = 0);
    ScreenList* addScreen(IGameScreen* s);

    void destroy(GameTime gameTime);

    static const i32 NO_START_SELECTED;
    static const i32 NO_SCREEN;
protected:
    MainGame* _game;

    std::vector<IGameScreen*> _screens;
    i32 _current;
};