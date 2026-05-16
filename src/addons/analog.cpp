#include "addons/analog.h"
#include "config.pb.h"
#include "enums.pb.h"
#include "hardware/adc.h"
#include "helper.h"
#include "storagemanager.h"
#include "drivermanager.h"

#include <algorithm>
#include <math.h>

#define ADC_MAX ((1 << 12) - 1) // 4095
#define ADC_PIN_OFFSET 26
#define ANALOG_MAX 1.0f
#define ANALOG_CENTER 0.5f
#define ANALOG_MINIMUM 0.0f
#define ANALOG_CALIBRATION_SAVE_DELAY_MS 5000
#define ANALOG_CALIBRATION_MIN_RANGE 16

bool AnalogInput::available() {
    return Storage::getInstance().getAddonOptions().analogOptions.enabled;
}

void AnalogInput::setup() {
    const AnalogOptions& analogOptions = Storage::getInstance().getAddonOptions().analogOptions;
    
    // Setup our ADC Pair of Sticks
    adc_pairs[0].x_pin = analogOptions.analogAdc1PinX;
    adc_pairs[0].y_pin = analogOptions.analogAdc1PinY;
    adc_pairs[0].analog_invert = analogOptions.analogAdc1Invert;
    adc_pairs[0].analog_dpad = analogOptions.analogAdc1Mode;
    adc_pairs[0].ema_option = analogOptions.analog_smoothing;
    adc_pairs[0].ema_smoothing = analogOptions.smoothing_factor / 1000.0f;
    adc_pairs[0].error_rate = analogOptions.analog_error / 1000.0f;
    adc_pairs[0].in_deadzone = analogOptions.inner_deadzone / 100.0f;
    adc_pairs[0].out_deadzone = analogOptions.outer_deadzone / 100.0f;
    adc_pairs[0].auto_calibration = analogOptions.auto_calibrate;
    adc_pairs[0].forced_circularity = analogOptions.forced_circularity;
    adc_pairs[0].x_min = analogOptions.x_min;
    adc_pairs[0].x_max = analogOptions.x_max;
    adc_pairs[0].y_min = analogOptions.y_min;
    adc_pairs[0].y_max = analogOptions.y_max;
    adc_pairs[1].x_pin = analogOptions.analogAdc2PinX;
    adc_pairs[1].y_pin = analogOptions.analogAdc2PinY;
    adc_pairs[1].analog_invert = analogOptions.analogAdc2Invert;
    adc_pairs[1].analog_dpad = analogOptions.analogAdc2Mode;
    adc_pairs[1].ema_option = analogOptions.analog_smoothing2;
    adc_pairs[1].ema_smoothing = analogOptions.smoothing_factor2 / 1000.0f;
    adc_pairs[1].error_rate = analogOptions.analog_error2 / 1000.0f;
    adc_pairs[1].in_deadzone = analogOptions.inner_deadzone2 / 100.0f;
    adc_pairs[1].out_deadzone = analogOptions.outer_deadzone2 / 100.0f;
    adc_pairs[1].auto_calibration = analogOptions.auto_calibrate2;
    adc_pairs[1].forced_circularity = analogOptions.forced_circularity2;
    adc_pairs[1].x_min = analogOptions.x_min2;
    adc_pairs[1].x_max = analogOptions.x_max2;
    adc_pairs[1].y_min = analogOptions.y_min2;
    adc_pairs[1].y_max = analogOptions.y_max2;
    

    // Setup defaults and helpers
    for (int i = 0; i < ADC_COUNT; i++) {
        adc_pairs[i].x_pin_adc = adc_pairs[i].x_pin - ADC_PIN_OFFSET;
        adc_pairs[i].y_pin_adc = adc_pairs[i].y_pin - ADC_PIN_OFFSET;
        adc_pairs[i].x_value = ANALOG_CENTER;
        adc_pairs[i].y_value = ANALOG_CENTER;
        adc_pairs[i].xy_magnitude = 0.0f;
        adc_pairs[i].x_magnitude = 0.0f;
        adc_pairs[i].y_magnitude = 0.0f;
        adc_pairs[i].x_ema = 0.0f;
        adc_pairs[i].y_ema = 0.0f;
        if (adc_pairs[i].x_max <= adc_pairs[i].x_min) {
            adc_pairs[i].x_min = 0;
            adc_pairs[i].x_max = ADC_MAX;
        }
        if (adc_pairs[i].y_max <= adc_pairs[i].y_min) {
            adc_pairs[i].y_min = 0;
            adc_pairs[i].y_max = ADC_MAX;
        }
    }

    // Intialize and auto center X/Y for each pair
    for (int i = 0; i < ADC_COUNT; i++) {
        if(isValidPin(adc_pairs[i].x_pin)) {
            adc_gpio_init(adc_pairs[i].x_pin);
            if (adc_pairs[i].auto_calibration) {
                adc_select_input(adc_pairs[i].x_pin - ADC_PIN_OFFSET);
                adc_pairs[i].x_center = adc_read();
                if (adc_pairs[i].x_min == 0 && adc_pairs[i].x_max == ADC_MAX) {
                    adc_pairs[i].x_min = adc_pairs[i].x_center;
                    adc_pairs[i].x_max = adc_pairs[i].x_center;
                }
            }
        }
        if(isValidPin(adc_pairs[i].y_pin)) {
            adc_gpio_init(adc_pairs[i].y_pin);
            if (adc_pairs[i].auto_calibration) {
                adc_select_input(adc_pairs[i].y_pin - ADC_PIN_OFFSET);
                adc_pairs[i].y_center = adc_read();
                if (adc_pairs[i].y_min == 0 && adc_pairs[i].y_max == ADC_MAX) {
                    adc_pairs[i].y_min = adc_pairs[i].y_center;
                    adc_pairs[i].y_max = adc_pairs[i].y_center;
                }
            }
        }
    }
}

void AnalogInput::process() {
    Gamepad * gamepad = Storage::getInstance().GetGamepad();
    
    uint32_t joystickMid = GAMEPAD_JOYSTICK_MID;
    uint32_t joystickMax = GAMEPAD_JOYSTICK_MAX;
    if ( DriverManager::getInstance().getDriver() != nullptr ) {
        joystickMid = DriverManager::getInstance().getDriver()->GetJoystickMidValue();
        joystickMax = joystickMid * 2; // 0x8000 mid must be 0x10000 max, but we reduce by 1 if we're maxed out
    }

    for(int i = 0; i < ADC_COUNT; i++) {
        // Read X-Axis
        if (isValidPin(adc_pairs[i].x_pin)) {
            adc_pairs[i].x_value = readPin(i, adc_pairs[i].x_pin_adc, adc_pairs[i].x_center, true);
            if (adc_pairs[i].analog_invert == InvertMode::INVERT_X || 
                adc_pairs[i].analog_invert == InvertMode::INVERT_XY) {
                adc_pairs[i].x_value = ANALOG_MAX - adc_pairs[i].x_value;
            }
            if (adc_pairs[i].ema_option) {
                adc_pairs[i].x_value = emaCalculation(i, adc_pairs[i].x_value, adc_pairs[i].x_ema);
                adc_pairs[i].x_ema = adc_pairs[i].x_value;
            }
        }
        // Read Y-Axis
        if (isValidPin(adc_pairs[i].y_pin)) {
            adc_pairs[i].y_value = readPin(i, adc_pairs[i].y_pin_adc, adc_pairs[i].y_center, false);
            if (adc_pairs[i].analog_invert == InvertMode::INVERT_Y || 
                adc_pairs[i].analog_invert == InvertMode::INVERT_XY) {
                adc_pairs[i].y_value = ANALOG_MAX - adc_pairs[i].y_value;
            }
            if (adc_pairs[i].ema_option) {
                adc_pairs[i].y_value = emaCalculation(i, adc_pairs[i].y_value, adc_pairs[i].y_ema);
                adc_pairs[i].y_ema = adc_pairs[i].y_value;
            }
        }
        // Look for dead-zones and circularity
        adc_pairs[i].xy_magnitude = magnitudeCalculation(i, adc_pairs[i]);
        if (adc_pairs[i].xy_magnitude < adc_pairs[i].in_deadzone) {
            adc_pairs[i].x_value = ANALOG_CENTER;
            adc_pairs[i].y_value = ANALOG_CENTER;
        } else {
            radialDeadzone(i, adc_pairs[i]);
        }

        // If MID is 0x8000, clamp our max to 0xFFFF incase we are at 0x10000. 0x7FFF will max at 0xFFFE
        uint16_t clampedX = (uint16_t)std::min((uint32_t)(joystickMax * std::min(adc_pairs[i].x_value, 1.0f)), (uint32_t)0xFFFF);
        uint16_t clampedY = (uint16_t)std::min((uint32_t)(joystickMax * std::min(adc_pairs[i].y_value, 1.0f)), (uint32_t)0xFFFF);

        if (adc_pairs[i].analog_dpad == DpadMode::DPAD_MODE_LEFT_ANALOG) {
            gamepad->state.lx = clampedX;
            gamepad->state.ly = clampedY;
        } else if (adc_pairs[i].analog_dpad == DpadMode::DPAD_MODE_RIGHT_ANALOG) {
            gamepad->state.rx = clampedX;
            gamepad->state.ry = clampedY;
        }
    }

    saveCalibrationBounds();
}

float AnalogInput::readPin(int stick_num, Pin_t pin_adc, uint16_t center, bool is_x_axis) {
    adc_select_input(pin_adc);
    uint16_t adc_value = adc_read();
    adc_instance& adc_inst = adc_pairs[stick_num];

    if (adc_inst.auto_calibration) {
        updateCalibrationBounds(stick_num, is_x_axis, adc_value);
        return normalizeCalibratedValue(adc_value, center,
            is_x_axis ? adc_inst.x_min : adc_inst.y_min,
            is_x_axis ? adc_inst.x_max : adc_inst.y_max);
    }

    return ((float)adc_value) / ADC_MAX;
}

bool AnalogInput::updateCalibrationBounds(int stick_num, bool is_x_axis, uint16_t adc_value) {
    adc_instance& adc_inst = adc_pairs[stick_num];
    uint16_t& min_value = is_x_axis ? adc_inst.x_min : adc_inst.y_min;
    uint16_t& max_value = is_x_axis ? adc_inst.x_max : adc_inst.y_max;

    bool changed = false;
    if (adc_value < min_value) {
        min_value = adc_value;
        changed = true;
    }
    if (adc_value > max_value) {
        max_value = adc_value;
        changed = true;
    }

    if (changed) {
        calibration_save_pending = true;
        next_calibration_save = getMillis() + ANALOG_CALIBRATION_SAVE_DELAY_MS;
    }

    return changed;
}

float AnalogInput::normalizeCalibratedValue(uint16_t adc_value, uint16_t center, uint16_t min_value, uint16_t max_value) {
    if (max_value <= min_value || (max_value - min_value) < ANALOG_CALIBRATION_MIN_RANGE || center <= min_value || center >= max_value) {
        min_value = 0;
        max_value = ADC_MAX;
        if (center <= min_value || center >= max_value) {
            center = ADC_MAX / 2;
        }
    }

    uint16_t normalized_value = ADC_MAX / 2;
    if (adc_value > center) {
        normalized_value = map(std::min(adc_value, max_value), center, max_value, ADC_MAX / 2, ADC_MAX);
    } else if (adc_value < center) {
        normalized_value = map(std::max(adc_value, min_value), min_value, center, 0, ADC_MAX / 2);
    }

    return std::clamp(((float)normalized_value) / ADC_MAX, ANALOG_MINIMUM, ANALOG_MAX);
}

void AnalogInput::saveCalibrationBounds() {
    if (!calibration_save_pending || getMillis() < next_calibration_save) {
        return;
    }

    AnalogOptions& analogOptions = Storage::getInstance().getAddonOptions().analogOptions;
    analogOptions.x_min = adc_pairs[0].x_min;
    analogOptions.x_max = adc_pairs[0].x_max;
    analogOptions.y_min = adc_pairs[0].y_min;
    analogOptions.y_max = adc_pairs[0].y_max;
    analogOptions.x_min2 = adc_pairs[1].x_min;
    analogOptions.x_max2 = adc_pairs[1].x_max;
    analogOptions.y_min2 = adc_pairs[1].y_min;
    analogOptions.y_max2 = adc_pairs[1].y_max;
    analogOptions.has_x_min = true;
    analogOptions.has_x_max = true;
    analogOptions.has_y_min = true;
    analogOptions.has_y_max = true;
    analogOptions.has_x_min2 = true;
    analogOptions.has_x_max2 = true;
    analogOptions.has_y_min2 = true;
    analogOptions.has_y_max2 = true;

    if (Storage::getInstance().save()) {
        calibration_save_pending = false;
    } else {
        next_calibration_save = getMillis() + ANALOG_CALIBRATION_SAVE_DELAY_MS;
    }
}

float AnalogInput::emaCalculation(int stick_num, float ema_value, float ema_previous) {
    return (adc_pairs[stick_num].ema_smoothing * ema_value) + ((1.0f - adc_pairs[stick_num].ema_smoothing) * ema_previous);
}

uint16_t AnalogInput::map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
    if (in_max <= in_min) {
        return out_min;
    }
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float AnalogInput::magnitudeCalculation(int stick_num, adc_instance & adc_inst) {
    adc_inst.x_magnitude = adc_inst.x_value - ANALOG_CENTER;
    adc_inst.y_magnitude = adc_inst.y_value - ANALOG_CENTER;
    return adc_pairs[stick_num].error_rate * std::sqrt((adc_inst.x_magnitude * adc_inst.x_magnitude) + (adc_inst.y_magnitude * adc_inst.y_magnitude));
}

void AnalogInput::radialDeadzone(int stick_num, adc_instance & adc_inst) {
    float deadzone_range = std::max(adc_pairs[stick_num].out_deadzone - adc_pairs[stick_num].in_deadzone, 0.0f);
    if (deadzone_range <= 0.0f || adc_inst.xy_magnitude <= 0.0f) {
        return;
    }

    float scaling_factor = (adc_inst.xy_magnitude - adc_pairs[stick_num].in_deadzone) / deadzone_range;
    if (adc_pairs[stick_num].forced_circularity == true) {
        scaling_factor = std::fmin(scaling_factor, ANALOG_CENTER);
    }
    adc_inst.x_value = ((adc_inst.x_magnitude / adc_inst.xy_magnitude) * scaling_factor) + ANALOG_CENTER;
    adc_inst.y_value = ((adc_inst.y_magnitude / adc_inst.xy_magnitude) * scaling_factor) + ANALOG_CENTER;
    adc_inst.x_value = std::clamp(adc_inst.x_value, ANALOG_MINIMUM, ANALOG_MAX);
    adc_inst.y_value = std::clamp(adc_inst.y_value, ANALOG_MINIMUM, ANALOG_MAX);
}