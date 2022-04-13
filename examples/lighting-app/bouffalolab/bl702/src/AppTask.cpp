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

#include <easyflash.h>
#include "AppTask.h"
#include "AppConfig.h"
#include "LEDWidget.h"
#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/on-off-server/on-off-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

#include <assert.h>

#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#include <lib/support/CodeUtils.h>

#include <platform/CHIPDeviceLayer.h>
#include <platform/bouffalolab/bl702/PlatformManagerImpl.h>

#if HEAP_MONITORING
#include "MemMonitoring.h"
#endif

#if CHIP_ENABLE_OPENTHREAD
#include <platform/bouffalolab/bl702/ThreadStackManagerImpl.h>
#include <platform/OpenThread/OpenThreadUtils.h>
#include <platform/ThreadStackManager.h>
#include <openthread_port.h>
#endif

#if CONFIG_ENABLE_CHIP_SHELL
#include <ChipShellCollection.h>
#include <src/lib/shell/Engine.h>
#include "uart.h"
#endif

extern "C" {
    #include <bl_gpio.h>
    #include <hal_gpio.h>
    #include <hosal_gpio.h>
    #include "board.h"
}

#define FACTORY_RESET_TRIGGER_TIMEOUT       3000
#define FACTORY_RESET_CANCEL_WINDOW_TIMEOUT 3000
#define APP_TASK_PRIORITY                   2
#define APP_REBOOT_RESET_COUNT              3

namespace {

LEDWidget sStatusLED;
ColorLEDWidget sLightLED;
uint8_t is_powerup_indicated = 0;

} // namespace


using namespace chip::TLV;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

#if CONFIG_ENABLE_CHIP_SHELL
using namespace chip::Shell;
#endif

AppTask AppTask::sAppTask;
StackType_t AppTask::appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
StaticTask_t AppTask::appTaskStruct;

void PlatformManagerImpl::PlatformInit(void) 
{
#if CONFIG_ENABLE_CHIP_SHELL
    AppTask::StartAppShellTask();
#endif

#if HEAP_MONITORING
    MemMonitoring::startHeapMonitoring();
#endif

    ChipLogProgress(NotSpecified, "Initializing CHIP stack");
    CHIP_ERROR ret = PlatformMgr().InitChipStack();
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "PlatformMgr().InitChipStack() failed"); 
        appError(ret);
    }

    chip::DeviceLayer::ConnectivityMgr().SetBLEDeviceName("BL702_LIGHT");

    ot_radioInit();
#if CONFIG_ENABLE_CHIP_SHELL
    cmd_otcli_init();
#endif
    ChipLogProgress(NotSpecified, "Initializing OpenThread stack");
    ret = ThreadStackMgr().InitThreadStack();
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "ThreadStackMgr().InitThreadStack() failed"); 
        appError(ret);
    }

#if CHIP_DEVICE_CONFIG_THREAD_FTD
    ret = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
#else
    ret = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice);
#endif
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "ConnectivityMgr().SetThreadDeviceType() failed"); 
        appError(ret);
    }

    chip::DeviceLayer::PlatformMgr().LockChipStack();

    // Initialize device attestation config
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());

    // Init ZCL Data Model
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();
    chip::Server::GetInstance().Init(initParams);
    chip::DeviceLayer::PlatformMgr().UnlockChipStack();

    ChipLogProgress(NotSpecified, "Starting OpenThread task");
    // Start OpenThread task
    ret = ThreadStackMgrImpl().StartThreadTask();
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "ThreadStackMgr().StartThreadTask() failed"); 
        appError(ret);
    }

    ConfigurationMgr().LogDeviceConfig();

    PrintOnboardingCodes(chip::RendezvousInformationFlag(chip::RendezvousInformationFlag::kBLE));

    GetAppTask().PostEvent(AppTask::APP_EVENT_STARTED);
    vTaskResume(GetAppTask().sAppTaskHandle);
}

void StartAppTask(void)
{
    ChipLogProgress(NotSpecified, "Initializing APP task");
    GetAppTask().sAppTaskHandle = xTaskCreateStatic(GetAppTask().AppTaskMain, APP_TASK_NAME, 
        ArraySize(GetAppTask().appStack), NULL, APP_TASK_PRIORITY, GetAppTask().appStack, &GetAppTask().appTaskStruct);
    if (GetAppTask().sAppTaskHandle == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create app task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }
}

#if CONFIG_ENABLE_CHIP_SHELL
void AppTask::AppShellTask(void *args) 
{
    Engine::Root().RunMainLoop();
}

CHIP_ERROR AppTask::StartAppShellTask()
{
    static TaskHandle_t shellTask;

    uartInit();

    cmd_misc_init();
    cmd_ping_init();
    cmd_send_init();

    xTaskCreate(AppTask::AppShellTask, "chip_shell", 1024 / sizeof(configSTACK_DEPTH_TYPE), NULL, APP_TASK_PRIORITY, &shellTask);

    return CHIP_NO_ERROR;
}
#endif

void AppTask::PostEvent(app_event_t event)
{
    if (xPortIsInsideInterrupt()) {
        BaseType_t higherPrioTaskWoken = pdFALSE;
        xTaskNotifyFromISR(sAppTaskHandle, event, eSetBits, &higherPrioTaskWoken);
    }
    else {
        xTaskNotify(sAppTaskHandle, event, eSetBits);
    }
}

void AppTask::AppTaskMain(void * pvParameter)
{
    app_event_t         appEvent;
    static uint32_t     taskDelay = portMAX_DELAY;

    ChipLogProgress(NotSpecified, "Starting Platform Manager Event Loop");
    CHIP_ERROR ret = PlatformMgr().StartEventLoopTask();
    if (ret != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "PlatformMgr().StartEventLoopTask() failed");
        appError(ret);
    }
    vTaskSuspend(NULL);

    GetAppTask().sTimer = xTimerCreate("lightTmr", pdMS_TO_TICKS(1000), false, NULL, AppTask::TimerCallback);
    if (GetAppTask().sTimer == NULL)
    {
        ChipLogError(NotSpecified, "Failed to create timer task");
        appError(APP_ERROR_EVENT_QUEUE_FAILED);
    }

    sStatusLED.Init();
    sLightLED.Init();
    ButtonInit();

    ChipLogProgress(NotSpecified, "App Task started, with heap %d left\r\n", xPortGetFreeHeapSize());

    while (true)
    {
        appEvent = APP_EVENT_NONE;
        BaseType_t eventReceived = xTaskNotifyWait( 0, APP_EVENT_ALL_MASK, (uint32_t *)&appEvent, taskDelay);

        if (eventReceived) {
            PlatformMgr().LockChipStack();
            if (APP_EVENT_STARTED & appEvent) {

                UpdateCluster_LightOnoff(true);
            }

            if (APP_EVENT_LIGHTING_MASK & appEvent) {
                UpdateLighting((app_event_t)(APP_EVENT_LIGHTING_MASK & appEvent));
            }

            if (APP_EVENT_SYS_ALL_MASK & appEvent) {

                if (APP_EVENT_FACTORY_RESET & appEvent) {
                    DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
                }
                else {
                    UpdateCluster_LightOnoff(true);
                }
            }
            PlatformMgr().UnlockChipStack();
        }

        TimerDutyCycle(appEvent);
        if (0 == is_powerup_indicated) {
            is_powerup_indicated = StartTimer();
            ChipLogProgress(NotSpecified, "Starimter call\r\n"); 
        }
    }
}

void AppTask::ChipEventHandler(const ChipDeviceEvent * event, intptr_t arg)
{
    switch (event->Type)
    {
    case DeviceEventType::kCHIPoBLEAdvertisingChange:
        if (ConnectivityMgr().NumBLEConnections()) {
            GetAppTask().PostEvent(APP_EVENT_SYS_BLE_CONN);
        }
        else {
            GetAppTask().PostEvent(APP_EVENT_SYS_BLE_ADV);
        }
        ChipLogProgress(NotSpecified, "Thread state, ble conn %d\r\n", 
            ConnectivityMgr().NumBLEConnections()); 
        break;
    case DeviceEventType::kThreadStateChange:

        if (ConnectivityMgr().IsThreadProvisioned() && ConnectivityMgr().IsThreadEnabled()) {
            GetAppTask().PostEvent(APP_EVENT_SYS_PROVISIONED);
        }

        ChipLogProgress(NotSpecified, "Thread state, prov %d, enabled %d, attached %d\r\n", 
            ConnectivityMgr().IsThreadProvisioned(), ConnectivityMgr().IsThreadEnabled(), 
            ConnectivityMgr().IsThreadAttached()); 
        break;
    case DeviceEventType::kWiFiConnectivityChange:
        GetAppTask().PostEvent(APP_EVENT_SYS_PROVISIONED);
        break;
    default:
        break;
    }
}

void AppTask::UpdateLighting(app_event_t event)
{
    uint8_t h, s, v, onoff;
    EmberAfAttributeType dataType;
    EndpointId endpoint = GetAppTask().GetEndpointId();

    do
    {
        if (EMBER_ZCL_STATUS_SUCCESS != emberAfReadAttribute(endpoint,
            ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID, 
            CLUSTER_MASK_SERVER, &onoff, 1, &dataType)) {
            break;
        }

        if (EMBER_ZCL_STATUS_SUCCESS != emberAfReadAttribute(endpoint,
            ZCL_LEVEL_CONTROL_CLUSTER_ID, 
            ZCL_CURRENT_LEVEL_ATTRIBUTE_ID, 
            CLUSTER_MASK_SERVER, &v, 1, &dataType)) {
            break;
        }
        if (EMBER_ZCL_STATUS_SUCCESS != emberAfReadAttribute(endpoint,
            ZCL_COLOR_CONTROL_CLUSTER_ID, 
            ZCL_COLOR_CONTROL_CURRENT_HUE_ATTRIBUTE_ID, 
            CLUSTER_MASK_SERVER, &h, 1, &dataType)) {
            break;
        }
        if (EMBER_ZCL_STATUS_SUCCESS != emberAfReadAttribute(endpoint,
            ZCL_COLOR_CONTROL_CLUSTER_ID, 
            ZCL_COLOR_CONTROL_CURRENT_SATURATION_ATTRIBUTE_ID, 
            CLUSTER_MASK_SERVER, &s, 1, &dataType)) {
            break;
        }

        if (0 == onoff) {
            sLightLED.SetLevel(0);
        }
        else {
            sLightLED.SetLevel(v);
        }
        ChipLogProgress(NotSpecified, "UpdateLighting (%d), onoff %d, level %d, hue %d, sta %d",
            endpoint, onoff, v, h, s);

    } while(0);
}

void AppTask::UpdateCluster_LightOnoff(uint8_t bonoff)
{
    uint8_t newValue = bonoff;
    EndpointId endpoint = GetAppTask().GetEndpointId();

    // write the new on/off value
    EmberAfStatus status = OnOffServer::Instance().setOnOffValue(endpoint, newValue, false);

    if (status != EMBER_ZCL_STATUS_SUCCESS) {
        ChipLogError(NotSpecified, "ERR: updating on/off %x", status);
    }
}

bool AppTask::StartTimer(void)
{
    uint32_t aTimeoutMs = sStatusLED.GetOnoff() ? GetAppTask().mBlinkOnTimeMS : GetAppTask().mBlinkOffTimeMS;
    if (!aTimeoutMs) {
        return false;
    }

    if (xTimerIsTimerActive(GetAppTask().sTimer)) {
        CancelTimer();
    }

    if (xTimerChangePeriod(GetAppTask().sTimer, pdMS_TO_TICKS(aTimeoutMs), pdMS_TO_TICKS(100)) != pdPASS) {
        ChipLogError(NotSpecified, "Failed to access timer with 100 ms delay.");
    }

    return true;
}

void AppTask::CancelTimer(void)
{
    xTimerStop(GetAppTask().sTimer, 0);
}

void AppTask::TimerCallback(TimerHandle_t xTimer)
{
    GetAppTask().PostEvent(APP_EVENT_TIMER);
}

void AppTask::TimerEventHandler(void)
{
    if (GetAppTask().buttonPressedTimeout) {
        /** device is being in factory reset confirm state*/
        if (ButtonPressed()) {
            if (GetAppTask().buttonPressedTimeout < chip::System::SystemClock().GetMonotonicMilliseconds64().count()) {
                /** do factory reset */
                GetAppTask().PostEvent(APP_EVENT_FACTORY_RESET);
            }
        }
        else {
            /** factory reset cancelled */
            GetAppTask().buttonPressedTimeout = 0;
            GetAppTask().PostEvent(APP_EVENT_BTN_FACTORY_RESET_CANCEL);
        }
    }

    sStatusLED.Toggle();
    StartTimer();
}

void AppTask::TimerDutyCycle(app_event_t event)
{
    static uint32_t backup_blinkOnTimeMS, backup_blinkOffTimeMS;

    if (event & APP_EVENT_SYS_PROVISIONED) {
        GetAppTask().mBlinkOnTimeMS = 800, GetAppTask().mBlinkOffTimeMS = 200;
    }
    else if (event & APP_EVENT_SYS_BLE_CONN) {
        GetAppTask().mBlinkOnTimeMS = 200, GetAppTask().mBlinkOffTimeMS = 200;
    }
    else if (event & APP_EVENT_SYS_BLE_ADV){
        GetAppTask().mBlinkOnTimeMS = 200, GetAppTask().mBlinkOffTimeMS = 800;
    }
    else if (event & APP_EVENT_BTN_FACTORY_RESET) {
        GetAppTask().mBlinkOnTimeMS = 500, GetAppTask().mBlinkOffTimeMS = 500;
        return;
    }
    else if (event & APP_EVENT_BTN_FACTORY_RESET_CANCEL) {
        GetAppTask().mBlinkOnTimeMS = backup_blinkOnTimeMS, GetAppTask().mBlinkOffTimeMS = backup_blinkOffTimeMS;
    }

    backup_blinkOnTimeMS = GetAppTask().mBlinkOnTimeMS, backup_blinkOffTimeMS = GetAppTask().mBlinkOffTimeMS;
}

hosal_gpio_dev_t gpio_key = {
    .port = LED_BTN_RESET,
    .config = INPUT_PULL_UP,
    .priv = NULL
};

void AppTask::ButtonInit(void)
{
    GetAppTask().buttonPressedTimeout = 0;

    hosal_gpio_init(&gpio_key);
    hosal_gpio_irq_set(&gpio_key, HOSAL_IRQ_TRIG_NEG_PULSE, GetAppTask().ButtonEventHandler, NULL);
}

bool AppTask::ButtonPressed(void)
{
    uint8_t val = 1;

    hosal_gpio_input_get(&gpio_key, &val);
    return val == 0;
}

void AppTask::ButtonEventHandler(void * arg)
{
    if (ButtonPressed()) {
        GetAppTask().PostEvent(APP_EVENT_BTN_FACTORY_RESET);
        GetAppTask().buttonPressedTimeout = chip::System::SystemClock().GetMonotonicMilliseconds64().count() + FACTORY_RESET_TRIGGER_TIMEOUT - 100;
    }
}
