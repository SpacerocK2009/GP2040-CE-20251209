#include "gridgradient.h"
#include "storagemanager.h"

#include <algorithm>
#include <cmath>

static constexpr float TWO_PI = 6.28318530718f;
static constexpr float COLUMN_OFFSET = 1.57079632679f;

GridGradient::GridGradient(PixelMatrix &matrix) : Animation(matrix) {
    setupButtons();
    setupLeverPositions();
}

void GridGradient::setupButtons() {
    const std::vector<std::pair<uint32_t, uint8_t>> buttonMap = {
        { GAMEPAD_MASK_A2, 0 },
        { GAMEPAD_MASK_B3, 0 },
        { GAMEPAD_MASK_B4, 1 },
        { GAMEPAD_MASK_R1, 2 },
        { GAMEPAD_MASK_L1, 3 },
        { GAMEPAD_MASK_B1, 0 },
        { GAMEPAD_MASK_B2, 1 },
        { GAMEPAD_MASK_R2, 2 },
        { GAMEPAD_MASK_L2, 3 },
        { GAMEPAD_MASK_R3, 0 },
        { GAMEPAD_MASK_L3, 1 },
    };

    for (auto &row : matrix->pixels) {
        for (auto &pixel : row) {
            if (pixel.index == NO_PIXEL.index)
                continue;

            for (auto &mapping : buttonMap) {
                if (pixel.mask == mapping.first && !pixel.positions.empty()) {
                    gridButtons.push_back({pixel, mapping.second});
                    break;
                }
            }
        }
    }
}

void GridGradient::setupLeverPositions() {
    std::set<uint8_t> leverIndex;
    const uint32_t leverMasks[] = { GAMEPAD_MASK_DU, GAMEPAD_MASK_DD, GAMEPAD_MASK_DL, GAMEPAD_MASK_DR };

    for (auto &row : matrix->pixels) {
        for (auto &pixel : row) {
            if (pixel.index == NO_PIXEL.index)
                continue;

            for (auto mask : leverMasks) {
                if (pixel.mask == mask) {
                    for (auto pos : pixel.positions) {
                        leverIndex.insert(pos);
                    }
                    break;
                }
            }
        }
    }

    leverPositions.assign(leverIndex.begin(), leverIndex.end());
}

RGB GridGradient::mixColors(const RGB &a, const RGB &b, float weight) const {
    float clamped = std::clamp(weight, 0.0f, 1.0f);
    return RGB(
        static_cast<uint8_t>(a.r + (b.r - a.r) * clamped),
        static_cast<uint8_t>(a.g + (b.g - a.g) * clamped),
        static_cast<uint8_t>(a.b + (b.b - a.b) * clamped)
    );
}

RGB GridGradient::columnColor(uint8_t column, const RGB &colorA, const RGB &colorB) const {
    float offset = COLUMN_OFFSET * static_cast<float>(3 - column);
    float value = std::sin(phase + offset);
    float weight = (value + 1.0f) * 0.5f;
    return mixColors(colorA, colorB, weight);
}

uint32_t GridGradient::getIntervalMs(GridGradientSpeed speed) const {
    switch (speed) {
        case GRID_GRADIENT_SPEED_SLOW:
            return 120;
        case GRID_GRADIENT_SPEED_FAST:
            return 45;
        default:
            return 80;
    }
}

float GridGradient::getPhaseStep(GridGradientSpeed speed) const {
    switch (speed) {
        case GRID_GRADIENT_SPEED_SLOW:
            return 0.30f;
        case GRID_GRADIENT_SPEED_FAST:
            return 0.60f;
        default:
            return 0.45f;
    }
}

uint32_t GridGradient::getPauseMs(GridGradientPause pause) const {
    switch (pause) {
        case GRID_GRADIENT_PAUSE_1S:
            return 1000;
        case GRID_GRADIENT_PAUSE_2S:
            return 2000;
        case GRID_GRADIENT_PAUSE_3S:
            return 3000;
        default:
            return 0;
    }
}

bool GridGradient::isMaskPressed(uint32_t mask, const std::set<uint32_t> &pressedMasks) const {
    return pressedMasks.find(mask) != pressedMasks.end();
}

bool GridGradient::isLeverDirectionPressed(const std::set<uint32_t> &pressedMasks) const {
    return isMaskPressed(GAMEPAD_MASK_DU, pressedMasks) ||
           isMaskPressed(GAMEPAD_MASK_DD, pressedMasks) ||
           isMaskPressed(GAMEPAD_MASK_DL, pressedMasks) ||
           isMaskPressed(GAMEPAD_MASK_DR, pressedMasks);
}

void GridGradient::renderCaseLeds(RGB (&frame)[100], const std::set<uint32_t> &pressedMasks, const RGB &caseNormal, const RGB &casePress) {
    const LEDOptions &ledOptions = Storage::getInstance().getLedOptions();
    int32_t start = ledOptions.caseRGBIndex;
    uint32_t count = ledOptions.caseRGBCount;

    if (start < 0 || count == 0) {
        return;
    }

    uint32_t limit = std::min<uint32_t>(count, 100 - start);
    for (uint32_t i = 0; i < limit; i++) {
        frame[start + i] = caseNormal;
    }

    struct DirectionConfig {
        uint32_t mask;
        pb_size_t count;
        const int32_t *values;
    };

    const AnimationOptions &options = Storage::getInstance().getAnimationOptions();
    const DirectionConfig configs[] = {
        { GAMEPAD_MASK_DU, options.gridCaseUpIndices_count, options.gridCaseUpIndices },
        { GAMEPAD_MASK_DD, options.gridCaseDownIndices_count, options.gridCaseDownIndices },
        { GAMEPAD_MASK_DR, options.gridCaseRightIndices_count, options.gridCaseRightIndices },
        { GAMEPAD_MASK_DL, options.gridCaseLeftIndices_count, options.gridCaseLeftIndices },
    };

    for (auto &config : configs) {
        if (!isMaskPressed(config.mask, pressedMasks))
            continue;

        for (pb_size_t i = 0; i < config.count; i++) {
            int32_t offset = config.values[i];
            if (offset < 0)
                continue;

            uint32_t target = static_cast<uint32_t>(start + offset);
            if (target >= static_cast<uint32_t>(start) && target < static_cast<uint32_t>(start) + limit) {
                frame[target] = casePress;
            }
        }
    }
}

bool GridGradient::Animate(RGB (&frame)[100]) {
    if (!time_reached(this->nextRunTime)) {
        return false;
    }

    AnimationOptions &animationOptions = Storage::getInstance().getAnimationOptions();
    const GridGradientSpeed speed = static_cast<GridGradientSpeed>(animationOptions.gridGradientSpeed);
    const GridGradientPause pause = static_cast<GridGradientPause>(animationOptions.gridGradientPause);

    UpdateTime();

    std::set<uint32_t> pressedMasks;
    for (auto &p : pixels) {
        pressedMasks.insert(p.mask);
    }

    std::fill_n(frame, 100, ColorBlack);

    RGB colorA(animationOptions.gridGradientColorA);
    RGB colorB(animationOptions.gridGradientColorB);
    RGB pressColor(animationOptions.gridButtonPressColor);

    uint32_t interval = getIntervalMs(speed);

    if (time_reached(pauseUntil)) {
        phase += getPhaseStep(speed);
        if (phase > TWO_PI) {
            phase -= TWO_PI;
            uint32_t pauseMs = getPauseMs(pause);
            if (pauseMs > 0) {
                pauseUntil = make_timeout_time_ms(pauseMs);
            }
        }
    }

    nextRunTime = make_timeout_time_ms(interval);

    for (auto &button : gridButtons) {
        if (button.pixel.index == NO_PIXEL.index || button.pixel.positions.empty())
            continue;

        bool pressed = isMaskPressed(button.pixel.mask, pressedMasks);
        RGB gradientColor = columnColor(button.column, colorA, colorB);
        RGB resolved = gradientColor;

        if (pressed) {
            times[button.pixel.index] = coolDownTimeInMs;
            hitColor[button.pixel.index] = pressColor;
            resolved = pressColor;
        } else {
            DecrementFadeCounter(button.pixel.index);
            resolved = BlendColor(hitColor[button.pixel.index], gradientColor, times[button.pixel.index]);
        }

        for (auto pos : button.pixel.positions) {
            if (pos < 100) {
                frame[pos] = resolved;
            }
        }
    }

    RGB leverNormal(animationOptions.gridLeverNormalColor);
    RGB leverPress(animationOptions.gridLeverPressColor);
    RGB leverColor = isLeverDirectionPressed(pressedMasks) ? leverPress : leverNormal;
    for (auto pos : leverPositions) {
        if (pos < 100) {
            frame[pos] = leverColor;
        }
    }

    RGB caseNormal(animationOptions.gridCaseNormalColor);
    RGB casePress(animationOptions.gridCaseLeverPressColor);
    renderCaseLeds(frame, pressedMasks, caseNormal, casePress);

    return true;
}

void GridGradient::ParameterUp() {
    AnimationOptions &animationOptions = Storage::getInstance().getAnimationOptions();
    uint32_t speed = animationOptions.gridGradientSpeed;
    speed = (speed + 1) % 3;
    animationOptions.gridGradientSpeed = speed;
}

void GridGradient::ParameterDown() {
    AnimationOptions &animationOptions = Storage::getInstance().getAnimationOptions();
    uint32_t speed = animationOptions.gridGradientSpeed;
    if (speed == 0) {
        speed = 2;
    } else {
        speed -= 1;
    }
    animationOptions.gridGradientSpeed = speed;
}
