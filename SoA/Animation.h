#pragma once

#include "Texture.h"

struct Animation {
    Animation() : fadeOutBegin(INT_MAX) {}
    int duration;
    int fadeOutBegin;
    int repetitions;
    int xFrames;
    int yFrames;
    int frames;
    vg::Texture texture;
};