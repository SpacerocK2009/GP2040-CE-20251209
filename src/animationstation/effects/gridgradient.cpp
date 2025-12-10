#include "gridgradient.h"
#include "storagemanager.h"

#include <algorithm>
#include <cmath>

GridGradient::GridGradient(PixelMatrix &matrix) : Animation(matrix) {
    setupButtons();
    setupLeverPositions();
}

void GridGradient::setupButtons() {
    struct ButtonLayout {
        uint32_t mask;
        uint8_t row;
        uint8_t column;
    };

    // Fixed 4x4 logical layout
    const ButtonLayout layout[] = {
        { GAMEPAD_MASK_A2, 0, 0 }, // TouchpadCenter
        { GAMEPAD_MASK_B1, 1, 0 }, // Square
        { GAMEPAD_MASK_B2, 1, 1 }, // Triangle
        { GAMEPAD_MASK_R1, 1, 2 },
        { GAMEPAD_MASK_L1, 1, 3 },
        { GAMEPAD_MASK_B3, 2, 0 }, // Cross
        { GAMEPAD_MASK_B4, 2, 1 }, // Circle
        { GAMEPAD_MASK_R2, 2, 2 },
        { GAMEPAD_MASK_L2, 2, 3 },
        { GAMEPAD_MASK_L3, 3, 0 },
        { GAMEPAD_MASK_R3, 3, 1 },
    };

    std::map<uint32_t, Pixel> discoveredPixels;

    // Gather first valid pixel entry for each logical mask
    for (auto &row : matrix->pixels) {
        for (auto &pixel : row) {
            if (pixel.index == NO_PIXEL.index || pixel.positions.empty())
                continue;

            if (discoveredPixels.find(pixel.mask) == discoveredPixels.end()) {
                discoveredPixels.emplace(pixel.mask, pixel);
            }
        }
    }

    std::vector<GridButton> ordered;
    uint8_t order = 0;

    // Column-major left-to-right, top-to-bottom
    for (uint8_t col = 0; col < 4; col++) {
        for (uint8_t row = 0; row < 4; row++) {
            for (auto &entry : layout) {
                if (entry.column == col && entry.row == row) {
                    auto pixelIt = discoveredPixels.find(entry.mask);
                    if (pixelIt != discoveredPixels.end()) {
                        const Pixel &pixel = pixelIt->second;
                        if (pixel.index != NO_PIXEL.index && !pixel.positions.empty()) {
                            ordered.push_back({ pixel, entry.row, entry.column, order++ });
                        }
                    }
                }
            }
        }
    }

    gridButtons = std::move(ordered);
}

void GridGradient::setupLeverPositions() {
    const uint32_t leverMasks[] = { GAMEPAD_MASK_DU, GAMEPAD_MASK_DD, GAMEPAD_MASK_DL, GAMEPAD_MASK_DR };

    for (auto &row : matrix->pixels) {
        for (auto &pixel : row) {
            if (pixel.index == NO_PIXEL.index)
                continue;

            for (auto mask : leverMasks) {
                if (pixel.mask == mask) {
                    for (auto pos : pixel.positions) {
                        leverPositions[mask].push_back(pos);
                    }
                    break;
                }
            }
        }
    }
}

GridGradientSpeed GridGradient::resolveSpeed(int32_t value) const {
    if (value < GRID_GRADIENT_SPEED_SLOW || value > GRID_GRADIENT_SPEED_VERY_FAST) {
        return GRID_GRADIENT_SPEED_NORMAL;
    }

    return static_cast<GridGradientSpeed>(value);
}

uint32_t GridGradient::getIntervalMs(GridGradientSpeed speed) const {
    switch (speed) {
        case GRID_GRADIENT_SPEED_VERY_SLOW:
            return 160;
        case GRID_GRADIENT_SPEED_SLOW:
            return 120;
        case GRID_GRADIENT_SPEED_NORMAL:
        default:
            return 80;
        case GRID_GRADIENT_SPEED_FAST:
            return 60;
        case GRID_GRADIENT_SPEED_VERY_FAST:
            return 35;
    }
}

float GridGradient::getPhaseStep(GridGradientSpeed speed) const {
    switch (speed) {
        case GRID_GRADIENT_SPEED_VERY_SLOW:
            return 0.18f;
        case GRID_GRADIENT_SPEED_SLOW:
            return 0.30f;
        case GRID_GRADIENT_SPEED_NORMAL:
        default:
            return 0.45f;
        case GRID_GRADIENT_SPEED_FAST:
            return 0.60f;
        case GRID_GRADIENT_SPEED_VERY_FAST:
            return 0.80f;
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

void GridGradient::renderCaseLeds(RGB (&frame)[100], const std::set<uint32_t> &pressedMasks, const RGB &caseNormal, const RGB &casePress) {
    const LEDOptions &ledOptions = Storage::getInstance().getLedOptions();
    int32_t start = ledOptions.caseRGBIndex;
    uint32_t count = ledOptions.caseRGBCount;

    if (start < 0 || count == 0) {
        return;
    }

    uint32_t limit = std::min<uint32_t>(count, 100 - start);

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

    std::set<uint32_t> activeTargets;
    for (auto &config : configs) {
        if (!isMaskPressed(config.mask, pressedMasks))
            continue;

        for (pb_size_t i = 0; i < config.count; i++) {
            int32_t offset = config.values[i];
            if (offset < 0)
                continue;

            uint32_t target = static_cast<uint32_t>(start + offset);
            if (target >= static_cast<uint32_t>(start) && target < static_cast<uint32_t>(start) + limit) {
                activeTargets.insert(target);
            }
        }
    }

    for (uint32_t i = 0; i < limit; i++) {
        uint32_t target = static_cast<uint32_t>(start + i);
        bool pressed = activeTargets.find(target) != activeTargets.end();

        if (pressed) {
            times[target] = coolDownTimeInMs;
            hitColor[target] = casePress;
            frame[target] = casePress;
        } else {
            DecrementFadeCounter(target);
            frame[target] = BlendColor(hitColor[target], caseNormal, times[target]);
        }
    }
}

bool GridGradient::Animate(RGB (&frame)[100]) {
    if (!time_reached(this->nextRunTime)) {
        return false;
    }

    AnimationOptions &animationOptions = Storage::getInstance().getAnimationOptions();
    const GridGradientSpeed speed = resolveSpeed(animationOptions.gridGradientSpeed);
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

    if (phase == GradientPhase::Pause && time_reached(pauseUntil)) {
        phase = GradientPhase::ForwardToB;
        phaseProgress = 0.0f;
    }

    const size_t totalButtons = gridButtons.size();

    if (totalButtons > 0) {
        if (phase != GradientPhase::Pause) {
            float phaseStep = getPhaseStep(speed);
            phaseProgress += phaseStep;

            if (phase == GradientPhase::ForwardToB && phaseProgress >= static_cast<float>(totalButtons)) {
                phase = GradientPhase::ReturnToA;
                phaseProgress = 0.0f;
            } else if (phase == GradientPhase::ReturnToA && phaseProgress >= static_cast<float>(totalButtons)) {
                uint32_t pauseMs = getPauseMs(pause);
                if (pauseMs > 0) {
                    phase = GradientPhase::Pause;
                    pauseUntil = make_timeout_time_ms(pauseMs);
                } else {
                    phase = GradientPhase::ForwardToB;
                }
                phaseProgress = 0.0f;
            }
        }
    } else {
        phase = GradientPhase::Pause;
        phaseProgress = 0.0f;
    }

    nextRunTime = make_timeout_time_ms(interval);

    for (auto &button : gridButtons) {
        if (button.pixel.index == NO_PIXEL.index || button.pixel.positions.empty())
            continue;

        bool pressed = isMaskPressed(button.pixel.mask, pressedMasks);
        size_t stepIndex = static_cast<size_t>(std::floor(phaseProgress));

        RGB gradientColor = colorA;

        if (phase == GradientPhase::ForwardToB) {
            gradientColor = (button.order < stepIndex) ? colorB : colorA;
        } else if (phase == GradientPhase::ReturnToA) {
            gradientColor = (button.order < stepIndex) ? colorA : colorB;
        }
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

    for (auto &entry : leverPositions) {
        bool pressed = isMaskPressed(entry.first, pressedMasks);
        for (auto pos : entry.second) {
            if (pos >= 100)
                continue;

            if (pressed) {
                times[pos] = coolDownTimeInMs;
                hitColor[pos] = leverPress;
                frame[pos] = leverPress;
            } else {
                DecrementFadeCounter(pos);
                frame[pos] = BlendColor(hitColor[pos], leverNormal, times[pos]);
            }
        }
    }

    RGB caseNormal(animationOptions.gridCaseNormalColor);
    RGB casePress(animationOptions.gridCaseLeverPressColor);
    renderCaseLeds(frame, pressedMasks, caseNormal, casePress);

    return true;
}

void GridGradient::ParameterUp() {
    AnimationOptions &animationOptions = Storage::getInstance().getAnimationOptions();
    const GridGradientSpeed speed = resolveSpeed(animationOptions.gridGradientSpeed);
    const GridGradientSpeed ordered[] = {
        GRID_GRADIENT_SPEED_VERY_SLOW,
        GRID_GRADIENT_SPEED_SLOW,
        GRID_GRADIENT_SPEED_NORMAL,
        GRID_GRADIENT_SPEED_FAST,
        GRID_GRADIENT_SPEED_VERY_FAST,
    };

    size_t index = 0;
    while (index < 5 && ordered[index] != speed) {
        index++;
    }

    index = (index + 1) % 5;
    animationOptions.gridGradientSpeed = ordered[index];
}

void GridGradient::ParameterDown() {
    AnimationOptions &animationOptions = Storage::getInstance().getAnimationOptions();
    const GridGradientSpeed speed = resolveSpeed(animationOptions.gridGradientSpeed);
    const GridGradientSpeed ordered[] = {
        GRID_GRADIENT_SPEED_VERY_SLOW,
        GRID_GRADIENT_SPEED_SLOW,
        GRID_GRADIENT_SPEED_NORMAL,
        GRID_GRADIENT_SPEED_FAST,
        GRID_GRADIENT_SPEED_VERY_FAST,
    };

    size_t index = 0;
    while (index < 5 && ordered[index] != speed) {
        index++;
    }

    if (index == 0) {
        index = 4;
    } else {
        index--;
    }

    animationOptions.gridGradientSpeed = ordered[index];
}
