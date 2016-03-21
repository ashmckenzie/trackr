/*---------------------------------------------------------------------------*/
/*  main.c                                                                   */
/*  Copyright (c) 2015 Robin Callender. All Rights Reserved.                 */
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#include "nrf51.h"
#include "nrf_soc.h"
#include "ble_radio_notification.h"
#include "softdevice_handler.h"
#include "ble_advdata.h"
#include "app_timer.h"
#include "app_gpiote.h"
#include "app_error.h"
#include "app_scheduler.h"

#include "config.h"
#include "advert.h"
#include "connect.h"
#include "eddystone.h"
#include "uart.h"
#include "dbglog.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
#ifdef DBGLOG_SUPPORT
static const struct {
    uint16_t  error_code;
    char    * text;
} nrf_errors [] = {
    { NRF_ERROR_SVC_HANDLER_MISSING,    "SVC_HANDLER_MISSING" },
    { NRF_ERROR_SOFTDEVICE_NOT_ENABLED, "SD_NOT_ENABLED"      },
    { NRF_ERROR_INTERNAL,               "INTERNAL"            },
    { NRF_ERROR_NO_MEM,                 "NO_MEM"              },
    { NRF_ERROR_NOT_FOUND,              "NOT_FOUND"           },
    { NRF_ERROR_NOT_SUPPORTED,          "NOT_SUPPORTED"       },
    { NRF_ERROR_INVALID_PARAM,          "INVALID_PARAMETER"   },
    { NRF_ERROR_INVALID_STATE,          "INVALID_STATE"       },
    { NRF_ERROR_INVALID_LENGTH,         "INVALID_LENGTH"      },
    { NRF_ERROR_INVALID_FLAGS,          "INVALID_FLAGS"       },
    { NRF_ERROR_INVALID_DATA,           "INVALID_DATA"        },
    { NRF_ERROR_DATA_SIZE,              "DATA_SIZE"           },
    { NRF_ERROR_TIMEOUT,                "TIMEOUT"             },
    { NRF_ERROR_NULL,                   "NULL"                },
    { NRF_ERROR_FORBIDDEN,              "FORBIDDEN"           },
    { NRF_ERROR_INVALID_ADDR,           "INVALID_ADDR"        },
    { NRF_ERROR_BUSY,                   "BUSY"                },
};
#define NRF_ERRORS_COUNT (sizeof(nrf_errors)/sizeof(nrf_errors[0]))
#endif /* DBGLOG_SUPPORT */

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
void app_error_handler(uint32_t error_code, 
                       uint32_t line, 
                       const uint8_t * filename)
{
#ifdef DBGLOG_SUPPORT
    char * text = NULL;

    for (int i=0; i < NRF_ERRORS_COUNT; i++) {
        if (error_code == nrf_errors[i].error_code) {
            text = nrf_errors[i].text;
            break;
        }
    }

    char   buffer [10];

    if (text == NULL) {
        sprintf(buffer, "0x%x", (unsigned) error_code);
        text = (char*) &buffer;
    }

    PRINTF("app_error_handler: NRF_ERROR_%s 0x%x, at %s(%d)\n", 
           text, (unsigned)error_code, (char*)filename, (int)line);

#ifdef DEBUG
    __BKPT();
#endif

#endif /* DBGLOG_SUPPORT */

    __disable_irq();

    /* The system can only recover with a reset (reboot). */
    NVIC_SystemReset(); 
}

/*---------------------------------------------------------------------------*/
/*  Callback function for asserts in the SoftDevice.                         */
/*---------------------------------------------------------------------------*/
void assert_nrf_callback(uint16_t line_num, const uint8_t *p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void bsp_events(bsp_event_t event)
{
    static bool led_on = false;

    PRINTF("bsp_event: %d\n", (int) event);

    if (led_on) {
        led_on = false;
        bsp_indication_set(BSP_INDICATE_USER_STATE_OFF);
    }
    else {
        led_on = true;
        bsp_indication_set(BSP_INDICATE_USER_STATE_ON);
    }
}

//*---------------------------------------------------------------------------*/
/*  Function for initializing the BLE stack.                                 */
/*---------------------------------------------------------------------------*/
static void ble_stack_init(void)
{
    ble_gap_addr_t      addr;
    ble_enable_params_t ble_enable_params;

    /* Initialize the SoftDevice handler module. */
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, false);

    /* Enable BLE stack */ 
    memset(&ble_enable_params, 0, sizeof(ble_enable_params));
    
    ble_enable_params.gatts_enable_params.service_changed = IS_SRVC_CHANGED_CHARACT_PRESENT;

    APP_ERROR_CHECK( sd_ble_enable(&ble_enable_params) );

    sd_ble_gap_address_get(&addr);
    sd_ble_gap_address_set(BLE_GAP_ADDR_CYCLE_MODE_NONE, &addr);

    /* Subscribe for BLE events. */
    APP_ERROR_CHECK( softdevice_ble_evt_handler_set(ble_evt_dispatch) );

    /* Register with the SoftDevice handler module for BLE events. */
    APP_ERROR_CHECK( softdevice_sys_evt_handler_set(sys_evt_dispatch) );
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void timer_init(void)
{
    APP_TIMER_INIT(APP_TIMER_PRESCALER,
                   APP_TIMER_MAX_TIMERS,
                   APP_TIMER_OP_QUEUE_SIZE,
                   false);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void gpiote_init(void)
{
    APP_GPIOTE_INIT(APP_GPIOTE_MAX_USERS);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void button_and_led_init(void)
{
    uint32_t err_code;

    err_code = bsp_init((BSP_INIT_LED | BSP_INIT_BUTTON),
                        APP_TIMER_TICKS(100, APP_TIMER_PRESCALER),
                        bsp_events);
    APP_ERROR_CHECK(err_code);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void radio_init(void)
{
    uint32_t err_code;

    err_code = ble_radio_notification_init(NRF_APP_PRIORITY_LOW,
                                           NRF_RADIO_NOTIFICATION_DISTANCE_5500US,
                                           eddystone_scheduler);
    APP_ERROR_CHECK(err_code);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void scheduler_init(void)
{
    APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);
}

/*---------------------------------------------------------------------------*/
/*  Function for doing power management.                                     */
/*---------------------------------------------------------------------------*/
static void power_manage(void)
{
    APP_ERROR_CHECK( sd_app_evt_wait() );
}

/*---------------------------------------------------------------------------*/
/*  Function for application main entry.                                     */
/*---------------------------------------------------------------------------*/
int main(void)
{
    ble_stack_init();
    scheduler_init();

#if defined(DBGLOG_SUPPORT)
    uart_init();
#endif

    PRINTF("\n*** firmware built: %s %s ***\n\n", __DATE__, __TIME__);

    storage_init();
    timer_init();
    gpiote_init();
    button_and_led_init();
    radio_init();

    gap_params_init();
    services_init();
    eddystone_init();
    conn_params_init();
    sec_params_init();

    advertising_start_connectable();

    /* Enter main loop. */
    for (;;) {
        app_sched_execute();
        power_manage();
    }
}
