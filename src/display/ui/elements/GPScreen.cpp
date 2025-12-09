#include "GPScreen.h"

const bool prioritySort(GPWidget * a, GPWidget * b) {
    return a->getPriority() > b->getPriority();
}

void GPScreen::draw(uint8_t pageLimit) {
    getRenderer()->clearScreen();

    // draw the display list
    if ( displayList.size() > 0 ) {
        std::sort(displayList.begin(), displayList.end(), prioritySort);
        for(std::vector<GPWidget*>::iterator it = displayList.begin(); it != displayList.end(); ++it) {
            (*it)->draw();
        }
    }
    drawScreen();
    getRenderer()->render(pageLimit);
}

void GPScreen::clear() {
    if (displayList.size() > 0) {
        displayList.clear();
        displayList.shrink_to_fit();
    }
}
