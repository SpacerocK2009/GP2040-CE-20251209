#ifndef _NOANIMATION_H_
#define _NOANIMATION_H_

#include "animation.h"

class NoAnimation : public Animation {
public:
    explicit NoAnimation(PixelMatrix &matrix) : Animation(matrix) {}
    bool Animate(RGB (&frame)[100]) override { return true; }
    void ParameterUp() override {}
    void ParameterDown() override {}
};

#endif
