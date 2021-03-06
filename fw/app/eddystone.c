/*---------------------------------------------------------------------------*/
/*  eddystone.c                                                              */
/*  Copyright (c) 2015 Robin Callender. All Rights Reserved.                 */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "nrf51.h"
#include "ble_gap.h"
#include "nrf_error.h"

#include "config.h"
#include "eddystone.h"
#include "ble_eddy.h"
#include "battery.h"
#include "temperature.h"
#include "dbglog.h"

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define FICR_DEVICEADDR          ((uint8_t*) &NRF_FICR->DEVICEADDR[0])

#define EDDYSTONE_UID_TYPE       0x00
#define EDDYSTONE_URL_TYPE       0x10
#define EDDYSTONE_TLM_TYPE       0x20

#define SERVICE_DATA_OFFSET      0x07

#define TLM_VERSION              0x00
 

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/

typedef struct {
    uint8_t adv_frame [BLE_GAP_ADV_MAX_SIZE];
    uint8_t adv_len;
} eddystone_frame_t;

typedef struct {
    uint8_t  flags_len;      // Length: Flags. CSS v5, Part A, 1.3
    uint8_t  flags_type;     // Flags data type value
    uint8_t  flags_data;     // Flags data
    uint8_t  svc_uuid_len;   // Length: Complete list of 16-bit Service UUIDs.
    uint8_t  svc_uuid_type;  // Complete list of 16-bit Service UUIDs data type value
    uint16_t svc_uuid_list;  // 16-bit Eddystone UUID
    uint8_t  svc_data_len;   // Length: Service Data.
    uint8_t  svc_data_type;  // Service Data data type value
    uint16_t svc_data_uuid;  // 16-bit Eddystone UUID
    uint8_t  frame_type;     // eddystone frame type
} __attribute__ ((packed)) eddystone_header_t;

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/

static eddystone_frame_t eddystone_frames [3];

static uint32_t adv_cnt = 0;
static uint32_t sec_cnt = 0;

static const eddystone_header_t  header = {
    .flags_len     = 0x02,
    .flags_type    = 0x01,
    .flags_data    = 0x06,
    .svc_uuid_len  = 0x03,
    .svc_uuid_type = 0x03,
    .svc_uuid_list = 0xFEAA,  // 0xAAFE  byte-swapped
    .svc_data_len  = 0x03,
    .svc_data_type = 0x16,
    .svc_data_uuid = 0xFEAA,  // 0xAAFE  byte-swapped
    .frame_type    = 0x00,
};

#define SVC_DATA_LEN_OFFSET  (offsetof(eddystone_header_t, svc_data_len) + 1)

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/

#define URL_PREFIX__http_www     0x00
#define URL_PREFIX__https_www    0x01
#define URL_PREFIX__http         0x02
#define URL_PREFIX__https        0x03

typedef struct {
    uint8_t     encoding;
    char      * prefix;
} url_prefix_t;

static const url_prefix_t  url_prefixes [] = {
    { .encoding = URL_PREFIX__http_www,  .prefix = "http://www"  },
    { .encoding = URL_PREFIX__https_www, .prefix = "https://www" },
    { .encoding = URL_PREFIX__http,      .prefix = "http://"     },
    { .encoding = URL_PREFIX__https,     .prefix = "https://"    },
};

#define URL_PREFIXES_COUNT (sizeof(url_prefixes)/sizeof(url_prefix_t))

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static uint32_t encode_url(uint8_t * encoded_advdata, uint8_t * len_advdata)
{
    uint8_t   i;
    uint8_t   prefix_len;

    char    * url     = (char*) eddy_url_str_get();
    uint8_t   url_len = eddy_url_len_get();
    char    * encoded = (char*)encoded_advdata;

    uint8_t   prefix = URL_PREFIX__http;

    if (*url == 0)     return NRF_ERROR_DATA_SIZE;
    if ( url_len == 0) return NRF_ERROR_DATA_SIZE;

    for (i=0; i < URL_PREFIXES_COUNT; i++) {

        prefix_len = strlen(url_prefixes[i].prefix);

        if (strncmp(url, url_prefixes[i].prefix, prefix_len) == 0) {

            prefix = url_prefixes[i].encoding;

            url     += prefix_len;
            url_len -= prefix_len;

            printf("url: \"%s\", url_len: %u, prefix: %d\n", 
                   url, (unsigned) url_len, prefix);
            break;
        }
    }
    
    if (*url == 0)    return NRF_ERROR_DATA_SIZE;
    if (url_len == 0) return NRF_ERROR_DATA_SIZE;

    if (url_len > URL_MAX_LENGTH -1) return NRF_ERROR_DATA_SIZE;

    *encoded++ = prefix;
    (*len_advdata)++;

    strncpy(encoded, url, url_len);
    *len_advdata += url_len;

    return NRF_SUCCESS;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static uint32_t eddystone_header(uint8_t * data, uint8_t frame_type, uint8_t * len) 
{
    if ((*len) + sizeof(eddystone_header_t) > BLE_GAP_ADV_MAX_SIZE) {
        return NRF_ERROR_DATA_SIZE;
    }

    memcpy(data, &header, sizeof(header));

    *len = sizeof(eddystone_header_t);
    
    ((eddystone_header_t*)data)->frame_type = frame_type;

    return NRF_SUCCESS;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static uint32_t eddystone_uint32(uint8_t * data, uint8_t * len, uint32_t val)
{
    if ((*len) + sizeof(uint32_t) > BLE_GAP_ADV_MAX_SIZE) {
        return NRF_ERROR_DATA_SIZE;
    }

    data[(*len)++] = (uint8_t) (val >> 24);
    data[(*len)++] = (uint8_t) (val >> 16);
    data[(*len)++] = (uint8_t) (val >>  8);
    data[(*len)++] = (uint8_t) (val >>  0);

    return NRF_SUCCESS;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static uint32_t eddystone_uint16(uint8_t * data, uint8_t * len, uint16_t val)
{
    if ((*len) + sizeof(uint16_t) > BLE_GAP_ADV_MAX_SIZE) {
        return NRF_ERROR_DATA_SIZE;
    }

    data[(*len)++] = (uint8_t) (val >> 8);
    data[(*len)++] = (uint8_t) (val >> 0);

    return NRF_SUCCESS;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void eddystone_set_adv_data(uint32_t frame_index)
{
    uint32_t err_code;

    uint8_t * encoded_advdata = eddystone_frames[frame_index].adv_frame;
    uint8_t   len_advdata     = eddystone_frames[frame_index].adv_len; 

    err_code = sd_ble_gap_adv_data_set(encoded_advdata, len_advdata, NULL, 0);
    APP_ERROR_CHECK( err_code );
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void build_tlm_frame_buffer(void)
{
    uint8_t * encoded_advdata =  eddystone_frames[EDDYSTONE_TLM].adv_frame;
    uint8_t * len_advdata     = &eddystone_frames[EDDYSTONE_TLM].adv_len;

    *len_advdata = 0;

    eddystone_header(encoded_advdata, EDDYSTONE_TLM_TYPE, len_advdata);

    encoded_advdata[(*len_advdata)++] = TLM_VERSION;

    /* Battery voltage, 1 mV/bit */
    eddystone_uint16(encoded_advdata, len_advdata, battery_level_get());

    /* Beacon temperature */
    eddystone_uint16(encoded_advdata, len_advdata, temperature_data_get());

    /* Advertising PDU count */
    eddystone_uint32(encoded_advdata, len_advdata, adv_cnt);

    /* Time since power-on or reboot */
    eddystone_uint32(encoded_advdata, len_advdata, sec_cnt);

    /* RFU field must be 0x00 */
    encoded_advdata[(*len_advdata)++] = 0x00;
    encoded_advdata[(*len_advdata)++] = 0x00;

    /* Update Service Data Length. */
    encoded_advdata[SERVICE_DATA_OFFSET] = (*len_advdata) - SVC_DATA_LEN_OFFSET;
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void build_url_frame_buffer(void)
{
    uint8_t * encoded_advdata =  eddystone_frames[EDDYSTONE_URL].adv_frame;
    uint8_t * len_advdata     = &eddystone_frames[EDDYSTONE_URL].adv_len;
    uint32_t  err_code;

    *len_advdata = 0;

    eddystone_header(encoded_advdata, EDDYSTONE_URL_TYPE, len_advdata);

    encoded_advdata[(*len_advdata)++] = APP_MEASURED_RSSI;

    err_code = encode_url(&encoded_advdata[(*len_advdata)], len_advdata);
    if (err_code != NRF_SUCCESS) {
        PUTS("encode_url failed");
    }

    /* Update Service Data Length. */
    encoded_advdata[SERVICE_DATA_OFFSET] = (*len_advdata) - SVC_DATA_LEN_OFFSET;

    dump_bytes(encoded_advdata, *len_advdata);
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
static void build_uid_frame_buffer(void)
{
    uint8_t * encoded_advdata =  eddystone_frames[EDDYSTONE_UID].adv_frame;
    uint8_t * len_advdata     = &eddystone_frames[EDDYSTONE_UID].adv_len;

    *len_advdata = 0;

    eddystone_header(encoded_advdata, EDDYSTONE_UID_TYPE, len_advdata);

    encoded_advdata[(*len_advdata)++] = APP_MEASURED_RSSI;

    /* Set Namespace */
    static const uint8_t namespace[] = UID_NAMESPACE;
    memcpy(&encoded_advdata[(*len_advdata)], &namespace, sizeof(namespace));
    *len_advdata += sizeof(namespace);

    /* Set Beacon Id (BID) to FICR Device Address */
    encoded_advdata[(*len_advdata)++] = FICR_DEVICEADDR[5];
    encoded_advdata[(*len_advdata)++] = FICR_DEVICEADDR[4];
    encoded_advdata[(*len_advdata)++] = FICR_DEVICEADDR[3];
    encoded_advdata[(*len_advdata)++] = FICR_DEVICEADDR[2];
    encoded_advdata[(*len_advdata)++] = FICR_DEVICEADDR[1];
    encoded_advdata[(*len_advdata)++] = FICR_DEVICEADDR[0];

    /* RFU field must be 0x00 */
    encoded_advdata[(*len_advdata)++] = 0x00;
    encoded_advdata[(*len_advdata)++] = 0x00;

    /* Update Service Data Length. */
    encoded_advdata[SERVICE_DATA_OFFSET] = (*len_advdata) - SVC_DATA_LEN_OFFSET;
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*---------------------------------------------------------------------------*/
void eddystone_init(void)
{
    memset(eddystone_frames, 0, sizeof(eddystone_frames));

    build_uid_frame_buffer();
    build_url_frame_buffer();
    build_tlm_frame_buffer();

    eddystone_set_adv_data(EDDYSTONE_UID);
}

/*---------------------------------------------------------------------------*/
/*  Crappy scheduler -- will re-implement later (sigh)                       */
/*---------------------------------------------------------------------------*/
void eddystone_scheduler(bool radio_is_active)
{
    static uint32_t iterations = 0;

    if (radio_is_active == false)
        return;

    iterations++;
    sec_cnt++;

    if ((iterations % 9) == 0) {
        build_tlm_frame_buffer();
        eddystone_set_adv_data(EDDYSTONE_TLM);
        adv_cnt++;
    }
    else if ((iterations % 5) == 0) {
        eddystone_set_adv_data(EDDYSTONE_URL);
        adv_cnt++;
    }
    else if ((iterations % 3) == 0) {
        eddystone_set_adv_data(EDDYSTONE_UID);
        adv_cnt++;
    }
}
