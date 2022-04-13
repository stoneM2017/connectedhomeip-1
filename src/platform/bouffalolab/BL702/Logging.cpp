/* See Project CHIP LICENSE file for licensing information. */

#include <platform/logging/LogV.h>

#include <lib/core/CHIPConfig.h>
#include <lib/support/logging/Constants.h>

#include <stdio.h>

extern "C" {
#include <blog.h>
}

namespace chip {
namespace Logging {
namespace Platform {


#define log_format(N, M, ...) do {  LOG_LOCK_LOCK;\
                                    __utils_printf("[%10u][%s] " M,\
                                    (xPortIsInsideInterrupt())?(xTaskGetTickCountFromISR()):(xTaskGetTickCount()),\
                                    N, ##__VA_ARGS__);\
                                    LOG_LOCK_UNLOCK;\
                                    } while(0==1)

#define logout_detail(M, ...)    log_format("DETAIL", M, ##__VA_ARGS__)
#define logout_error(M, ...)   log_format("ERROR", M, ##__VA_ARGS__)
#define logout_process(M, ...)  log_format("PROCESS", M, ##__VA_ARGS__)

void LogV(const char * module, uint8_t category, const char * msg, va_list v)
{
    char formattedMsg[CHIP_CONFIG_LOG_MESSAGE_MAX_SIZE];
    vsnprintf(formattedMsg, sizeof(formattedMsg), msg, v);

    switch (category)
    {
    case kLogCategory_Error:
        logout_error("[%s] %s\r\n", module, formattedMsg);
        break;
    case kLogCategory_Progress:
    default:
        logout_process("[%s] %s\r\n", module, formattedMsg);
        break;
    case kLogCategory_Detail:
        logout_detail("[%s] %s\r\n", module, formattedMsg);

        break;
    }
}

} // namespace Platform
} // namespace Logging
} // namespace chip
