#include "stdafx.h"
#include "PDA.h"
#include "PdaRenderStage.h"


PdaRenderStage::PdaRenderStage(const PDA* pda) :
    _pda(pda) {
    // Empty
}

void PdaRenderStage::draw() {
    if (_pda->isOpen()) {
        _pda->draw();
    }
}
