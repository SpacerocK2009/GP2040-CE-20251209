#include "gridgradient.h"
#include "storagemanager.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

// Preset B uses the same 4-color horizontal gradient as Preset A over a 7x4 grid.
// Gradient phase is based only on the X coordinate (left to right).
// TouchPadCenter (A2) is included at grid position (3,0).
namespace {
struct GridLayoutPresetB {
    int x;
    int y;
    uint32_t mask;
};

constexpr int GRID_PRESET_B_WIDTH = 7;
constexpr int GRID_PRESET_B_HEIGHT = 4;

const GridLayoutPresetB GRID_LAYOUT_PRESET_B[] = {
    { 3, 0, GAMEPAD_MASK_A2 },
    { 1, 1, GAMEPAD_MASK_DU },
    { 3, 1, GAMEPAD_MASK_B3 },
    { 4, 1, GAMEPAD_MASK_B4 },
    { 5, 1, GAMEPAD_MASK_R1 },
    { 6, 1, GAMEPAD_MASK_L1 },
    { 0, 2, GAMEPAD_MASK_DL },
    { 2, 2, GAMEPAD_MASK_DR },
    { 3, 2, GAMEPAD_MASK_B1 },
    { 4, 2, GAMEPAD_MASK_B2 },
    { 5, 2, GAMEPAD_MASK_R2 },
    { 6, 2, GAMEPAD_MASK_L2 },
    { 1, 3, GAMEPAD_MASK_DD },
    { 3, 3, GAMEPAD_MASK_L3 },
    { 4, 3, GAMEPAD_MASK_R3 },
};
} // namespace

GridGradient::GridGradient(PixelMatrix &matrix) : Animation(matrix) {
    setupButtons();
    setupLeverPositions();
    setupPresetBCells();
}

void GridGradient::setupButtons() {
    struct ColumnDefinition {
        uint8_t column;
        std::vector<uint32_t> masks;
    };

    const ColumnDefinition columns[] = {
        { 0, { GAMEPAD_MASK_A2, GAMEPAD_MASK_B1, GAMEPAD_MASK_B3, GAMEPAD_MASK_L3 } }, // TouchpadCenter, Square, Cross, L3
        { 1, { GAMEPAD_MASK_B2, GAMEPAD_MASK_B4, GAMEPAD_MASK_R3 } },                   // Triangle, Circle, R3
        { 2, { GAMEPAD_MASK_R1, GAMEPAD_MASK_R2 } },                                    // R1, R2
        { 3, { GAMEPAD_MASK_L1, GAMEPAD_MASK_L2 } },                                    // L1, L2
    };

    // Map masks to all discovered pixels so we can apply press blending correctly
    std::map<uint32_t, std::vector<Pixel>> maskToPixels;

    for (auto &row : matrix->pixels) {
        for (auto &pixel : row) {
            if (pixel.index == NO_PIXEL.index || pixel.positions.empty())
                continue;

            maskToPixels[pixel.mask].push_back(pixel);
        }
    }

    for (auto &column : columnLeds) {
        column.clear();
    }

    gridButtons.clear();

    for (auto &definition : columns) {
        for (auto mask : definition.masks) {
            auto it = maskToPixels.find(mask);
            if (it == maskToPixels.end())
                continue;

            for (auto &pixel : it->second) {
                gridButtons.push_back({ pixel, definition.column });
                for (auto pos : pixel.positions) {
                    if (pos < 100) {
                        columnLeds[definition.column].push_back(pos);
                    }
                }
            }
        }
    }
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

void GridGradient::setupPresetBCells() {
    presetBCells.clear();

    std::map<uint32_t, std::vector<uint8_t>> maskToPositions;

    for (auto &row : matrix->pixels) {
        for (auto &pixel : row) {
            if (pixel.index == NO_PIXEL.index || pixel.positions.empty())
                continue;

            for (auto pos : pixel.positions) {
                if (pos < 100) {
                    maskToPositions[pixel.mask].push_back(pos);
                }
            }
        }
    }

    for (auto &cell : GRID_LAYOUT_PRESET_B) {
        auto it = maskToPositions.find(cell.mask);
        if (it == maskToPositions.end())
            continue;

        presetBCells.push_back({ cell.x, cell.y, cell.mask, it->second });
    }
}

GridGradientSpeed GridGradient::resolveSpeed(int32_t value) const {
    if (value < GRID_GRADIENT_SPEED_SLOW || value > GRID_GRADIENT_SPEED_VERY_FAST) {
        return GRID_GRADIENT_SPEED_NORMAL;
    }

    return static_cast<GridGradientSpeed>(value);
}

uint32_t GridGradient::getIntervalMs(GridGradientSpeed speed) const {
    // Render frequently for smooth interpolation; faster speeds tick slightly faster
    switch (speed) {
        case GRID_GRADIENT_SPEED_VERY_SLOW:
            return 30;
        case GRID_GRADIENT_SPEED_SLOW:
            return 25;
        case GRID_GRADIENT_SPEED_NORMAL:
        default:
            return 20;
        case GRID_GRADIENT_SPEED_FAST:
            return 16;
        case GRID_GRADIENT_SPEED_VERY_FAST:
            return 12;
    }
}

uint32_t GridGradient::getColumnDurationMs(GridGradientSpeed speed) const {
    switch (speed) {
        case GRID_GRADIENT_SPEED_VERY_SLOW:
            return 1800;
        case GRID_GRADIENT_SPEED_SLOW:
            return 1400;
        case GRID_GRADIENT_SPEED_NORMAL:
        default:
            return 1000;
        case GRID_GRADIENT_SPEED_FAST:
            return 750;
        case GRID_GRADIENT_SPEED_VERY_FAST:
            return 500;
    }
}

bool GridGradient::isMaskPressed(uint32_t mask, const std::set<uint32_t> &pressedMasks) const {
    return pressedMasks.find(mask) != pressedMasks.end();
}

RGB GridGradient::interpolate(const RGB &from, const RGB &to, float t) const {
    RGB out;
    out.r = static_cast<uint8_t>(from.r + (to.r - from.r) * t);
    out.g = static_cast<uint8_t>(from.g + (to.g - from.g) * t);
    out.b = static_cast<uint8_t>(from.b + (to.b - from.b) * t);
    out.w = static_cast<uint8_t>(from.w + (to.w - from.w) * t);
    return out;
}

RGB GridGradient::columnColor(float t, const RGB &colorA, const RGB &colorB, const RGB &colorC, const RGB &colorD) const {
    if (t < 0.25f) {
        return interpolate(colorA, colorB, t / 0.25f);
    }

    if (t < 0.50f) {
        return interpolate(colorB, colorC, (t - 0.25f) / 0.25f);
    }

    if (t < 0.75f) {
        return interpolate(colorC, colorD, (t - 0.50f) / 0.25f);
    }

    return interpolate(colorD, colorA, (t - 0.75f) / 0.25f);
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
    const uint8_t gridPreset = static_cast<uint8_t>(animationOptions.gridGradientPreset);

    UpdateTime();

    std::set<uint32_t> pressedMasks;
    for (auto &p : pixels) {
        pressedMasks.insert(p.mask);
    }

    std::fill_n(frame, 100, ColorBlack);

    RGB colorA(animationOptions.gridGradientColorA);
    RGB colorB(animationOptions.gridGradientColorB);
    RGB colorC(animationOptions.gridGradientColorC);
    RGB colorD(animationOptions.gridGradientColorD);
    RGB pressColor(animationOptions.gridButtonPressColor);

    uint32_t interval = getIntervalMs(speed);
    uint32_t columnDurationMs = getColumnDurationMs(speed);

    float delta = static_cast<float>(updateTimeInMs) / static_cast<float>(columnDurationMs);
    globalPhase += delta;

    if (globalPhase >= 1.0f) {
        globalPhase = std::fmod(globalPhase, 1.0f);
    }

    nextRunTime = make_timeout_time_ms(interval);

    if (gridPreset == 0) {
        // Determine per-column base colors
        std::array<RGB, 4> columnColors = { colorA, colorA, colorA, colorA };
        constexpr float phaseOffset = 1.0f / 4.0f;
        for (size_t col = 0; col < columnColors.size(); col++) {
            float columnPhase = std::fmod(globalPhase + static_cast<float>(col) * phaseOffset, 1.0f);
            columnColors[col] = columnColor(columnPhase, colorA, colorB, colorC, colorD);
        }

        // Render base gradient per column
        for (size_t col = 0; col < columnLeds.size(); col++) {
            for (auto pos : columnLeds[col]) {
                if (pos < 100) {
                    frame[pos] = columnColors[col];
                }
            }
        }

        // Apply press overlay per button pixel
        for (auto &button : gridButtons) {
            if (button.pixel.index == NO_PIXEL.index || button.pixel.positions.empty())
                continue;

            bool pressed = isMaskPressed(button.pixel.mask, pressedMasks);
            RGB baseColor = columnColors[button.column];
            RGB resolved = baseColor;

            if (pressed) {
                times[button.pixel.index] = coolDownTimeInMs;
                hitColor[button.pixel.index] = pressColor;
                resolved = pressColor;
            } else {
                DecrementFadeCounter(button.pixel.index);
                resolved = BlendColor(hitColor[button.pixel.index], baseColor, times[button.pixel.index]);
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
    } else {
        const float maxIndex = static_cast<float>(GRID_PRESET_B_WIDTH - 1);

        for (auto &cell : presetBCells) {
            if (cell.indices.empty())
                continue;

            float normalizedX = std::clamp(static_cast<float>(cell.x) / maxIndex, 0.0f, 1.0f);
            float cellPhase = std::fmod(globalPhase + normalizedX, 1.0f);
            RGB baseColor = columnColor(cellPhase, colorA, colorB, colorC, colorD);

            bool pressed = isMaskPressed(cell.mask, pressedMasks);

            for (auto pos : cell.indices) {
                if (pos >= 100)
                    continue;

                if (pressed) {
                    times[pos] = coolDownTimeInMs;
                    hitColor[pos] = pressColor;
                    frame[pos] = pressColor;
                } else {
                    DecrementFadeCounter(pos);
                    frame[pos] = BlendColor(hitColor[pos], baseColor, times[pos]);
                }
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
