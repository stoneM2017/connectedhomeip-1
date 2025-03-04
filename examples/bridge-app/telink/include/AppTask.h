/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#pragma once

#include "AppConfig.h"
#include "AppEvent.h"
#if CONFIG_CHIP_ENABLE_APPLICATION_STATUS_LED
#include "LEDWidget.h"
#endif
#include "PWMDevice.h"
#include <platform/CHIPDeviceLayer.h>

#if CONFIG_CHIP_FACTORY_DATA
#include <platform/telink/FactoryDataProvider.h>
#endif

#include <cstdint>

struct k_timer;
struct Identify;

class AppTask
{
public:
    CHIP_ERROR StartApp(void);

    void PostEvent(AppEvent * aEvent);
    void UpdateClusterState(void);
    PWMDevice & GetPWMDevice(void) { return mPwmRgbBlueLed; }

    static void IdentifyEffectHandler(EmberAfIdentifyEffectIdentifier aEffect);

private:
    friend AppTask & GetAppTask(void);
    CHIP_ERROR Init(void);

    static void ActionInitiated(PWMDevice::Action_t aAction, int32_t aActor);
    static void ActionCompleted(PWMDevice::Action_t aAction, int32_t aActor);
    static void ActionIdentifyStateUpdateHandler(k_timer * timer);

    void DispatchEvent(AppEvent * event);

#if CONFIG_CHIP_ENABLE_APPLICATION_STATUS_LED
    static void UpdateLedStateEventHandler(AppEvent * aEvent);
    static void LEDStateUpdateHandler(LEDWidget * ledWidget);
    static void UpdateStatusLED();
#endif
    static void LightingActionButtonEventHandler(void);
    static void FactoryResetButtonEventHandler(void);
    static void StartThreadButtonEventHandler(void);
    static void StartBleAdvButtonEventHandler(void);

    static void ChipEventHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

    static void FactoryResetTimerTimeoutCallback(k_timer * timer);

    static void FactoryResetTimerEventHandler(AppEvent * aEvent);
    static void FactoryResetHandler(AppEvent * aEvent);
    static void StartThreadHandler(AppEvent * aEvent);
    static void LightingActionEventHandler(AppEvent * aEvent);
    static void StartBleAdvHandler(AppEvent * aEvent);
    static void UpdateIdentifyStateEventHandler(AppEvent * aEvent);

    static void InitButtons(void);
    static void InitServer(intptr_t context);

    static void ThreadProvisioningHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

    static AppTask sAppTask;
    PWMDevice mPwmRgbBlueLed;
    PWMDevice mPwmIdentifyLed;

#if CONFIG_CHIP_FACTORY_DATA
    chip::DeviceLayer::FactoryDataProvider<chip::DeviceLayer::ExternalFlashFactoryData> mFactoryDataProvider;
#endif
};

inline AppTask & GetAppTask(void)
{
    return AppTask::sAppTask;
}
