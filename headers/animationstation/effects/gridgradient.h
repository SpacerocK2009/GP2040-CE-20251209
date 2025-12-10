#ifndef _GRIDGRADIENT_H_
#define _GRIDGRADIENT_H_

#include "animation.h"
#include "enums.pb.h"
#include "pixel.h"
#include "pb.h"

#include <map>
#include <set>
#include <vector>

class GridGradient : public Animation {
public:
    explicit GridGradient(PixelMatrix &matrix);
    bool Animate(RGB (&frame)[100]) override;
    void ParameterUp() override;
    void ParameterDown() override;

private:
    struct GridButton {
        Pixel pixel;
        uint8_t column;
    };

    std::vector<GridButton> gridButtons;
    std::map<uint32_t, std::vector<uint8_t>> leverPositions;
    absolute_time_t nextRunTime = nil_time;
    absolute_time_t pauseUntil = nil_time;
    bool pauseActive = false;
    float phase = 0.0f;

    void setupButtons();
    void setupLeverPositions();
    RGB mixColors(const RGB &a, const RGB &b, float weight) const;
    RGB columnColor(uint8_t column, const RGB &colorA, const RGB &colorB) const;
    uint32_t getIntervalMs(GridGradientSpeed speed) const;
    float getPhaseStep(GridGradientSpeed speed) const;
    uint32_t getPauseMs(GridGradientPause pause) const;
    bool isMaskPressed(uint32_t mask, const std::set<uint32_t> &pressedMasks) const;
    void renderCaseLeds(RGB (&frame)[100], const std::set<uint32_t> &pressedMasks, const RGB &caseNormal, const RGB &casePress);
};

#endif
