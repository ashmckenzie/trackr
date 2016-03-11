/*---------------------------------------------------------------------------*/
/*  advert.h                                                                 */
/*  Copyright (c) 2015 Robin Callender. All Rights Reserved.                 */
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>

#include "nrf51.h"
#include "nrf_soc.h"
#include "softdevice_handler.h"
#include "app_scheduler.h"
#include "ble_advdata.h"
#include "ble_gap.h"
#include "app_timer.h"
#include "app_timer_appsh.h"

#include "config.h"
#include "advert.h"
#include "connect.h"
#include "eddystone.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/

static const ble_gap_adv_params_t   m_adv_params_connectable = {
    .type         = BLE_GAP_ADV_TYPE_ADV_IND,
    .p_peer_addr  = NULL,
    .fp           = 0,
    .p_whitelist  = NULL,
    .interval     = APP_ADV_INTERVAL,
    .timeout      = APP_ADV_TIMEOUT,
    .channel_mask = {0,0,0},
};

static const ble_gap_adv_params_t   m_adv_params_nonconnectable = {
    .type         = BLE_GAP_ADV_TYPE_ADV_NONCONN_IND,
    .p_peer_addr  = NULL,
    .fp           = 0,
    .p_whitelist  = NULL,
    .interval     = APP_ADV_INTERVAL,
    .timeout      = 0,
    .channel_mask = {0,0,0},
};

/*---------------------------------------------------------------------------*/
/*  Function for starting advertising: allow connections.                    */
/*---------------------------------------------------------------------------*/
void advertising_start_connectable(void)
{
    APP_ERROR_CHECK( sd_ble_gap_adv_start(&m_adv_params_connectable) );

    APP_ERROR_CHECK( bsp_indication_set(BSP_INDICATE_ADVERTISING) );
}

/*---------------------------------------------------------------------------*/
/*  Function for starting advertising: disallow connections.                 */
/*---------------------------------------------------------------------------*/
void advertising_start_nonconnectable(void)
{
    APP_ERROR_CHECK( sd_ble_gap_adv_start(&m_adv_params_nonconnectable) );
}

/*---------------------------------------------------------------------------*/
/*  Function for initializing the Advertising functionality.                 */
/*---------------------------------------------------------------------------*/
void advertising_init(void)
{
    /* Build (refresh) all Eddystone frames. */
    eddystone_init();
}
