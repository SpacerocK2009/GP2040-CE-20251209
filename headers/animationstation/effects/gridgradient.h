#ifndef _GRIDGRADIENT_H_
#define _GRIDGRADIENT_H_

#include "animation.h"
#include "enums.pb.h"
#include "pixel.h"
#include "pb.h"

#include <array>
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

    std::array<std::vector<uint16_t>, 4> columnLeds;
    std::vector<GridButton> gridButtons;
    std::map<uint32_t, std::vector<uint8_t>> leverPositions;
    absolute_time_t nextRunTime = nil_time;
    absolute_time_t pauseUntil = nil_time;
    uint8_t currentColumn = 0;
    float phaseProgress = 0.0f;

    enum class GradientPhase {
        Active,
        Pause
    };

    GradientPhase phase = GradientPhase::Active;

    void setupButtons();
    void setupLeverPositions();
    GridGradientSpeed resolveSpeed(int32_t value) const;
    uint32_t getIntervalMs(GridGradientSpeed speed) const;
    uint32_t getColumnDurationMs(GridGradientSpeed speed) const;
    uint32_t getPauseMs(GridGradientPause pause) const;
    bool isMaskPressed(uint32_t mask, const std::set<uint32_t> &pressedMasks) const;
    RGB interpolate(const RGB &from, const RGB &to, float t) const;
    RGB columnColor(float t, const RGB &colorA, const RGB &colorB) const;
    void renderCaseLeds(RGB (&frame)[100], const std::set<uint32_t> &pressedMasks, const RGB &caseNormal, const RGB &casePress);
};

#endif
