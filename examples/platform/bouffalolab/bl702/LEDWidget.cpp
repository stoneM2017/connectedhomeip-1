/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */


#include <bl702_glb.h>
#include <bl_sys.h>
#include <bl_gpio.h>
#include <hosal_gpio.h>
#include <board.h>
#include <demo_pwm.h>

#include "LEDWidget.h"

void LEDWidget::Init()
{
    mPin              = LED1_PIN;

    hosal_gpio_dev_t gpio_led = {
    .config = OUTPUT_OPEN_DRAIN_NO_PULL,
    .priv = NULL
    };
    gpio_led.port = mPin;

    hosal_gpio_init(&gpio_led);
    SetOnoff(false);
}

void LEDWidget::Toggle(void)
{
    hosal_gpio_dev_t gpio_led = {
        .port = mPin,
        .config = OUTPUT_OPEN_DRAIN_NO_PULL,
        .priv = NULL
    };

    SetOnoff(1 - mOnoff);
}

void LEDWidget::SetOnoff(bool state)
{
    hosal_gpio_dev_t gpio_led = {
        .port = mPin,
        .config = OUTPUT_OPEN_DRAIN_NO_PULL,
        .priv = NULL
    };

    mOnoff = state;

    if (state) {
        hosal_gpio_output_set(&gpio_led, 1);
    }
    else {
        hosal_gpio_output_set(&gpio_led, 0);
    }
}

bool LEDWidget::GetOnoff(void)
{
    hosal_gpio_dev_t gpio_led = {
        .port = mPin,
        .config = OUTPUT_OPEN_DRAIN_NO_PULL,
        .priv = NULL
    };

    return mOnoff ? true: false;
}

void DimmableLEDWidget::Init()
{
    light_onoff = light_v = 0;

    demo_hosal_pwm_init();
    demo_hosal_pwm_start();
}

void DimmableLEDWidget::SetOnoff(bool state)
{
    SetLevel(state ? 254 : 0);
}

bool DimmableLEDWidget::GetOnoff(void)
{
    return light_onoff ? 1: 0;
}

void DimmableLEDWidget::SetLevel(uint8_t level)
{
    set_level(level);
}

void ColorLEDWidget::Init()
{
    light_onoff = light_v = light_s = light_h = 0;

    demo_hosal_pwm_init();
    demo_hosal_pwm_start();
}

void ColorLEDWidget::SetOnoff(bool state)
{
    SetColor(state ? 254 : 0, light_h, light_s);
}

void ColorLEDWidget::SetLevel(uint8_t level)
{
    SetColor(level, light_h, light_s);
}

void ColorLEDWidget::SetColor(uint8_t level, uint8_t hue, uint8_t sat) 
{
    set_color(level, hue, sat);
}
