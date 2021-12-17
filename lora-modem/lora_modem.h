/*
 * File      : lora-modem-driver.h
 * This file is driver for LoRa\LoRaWAN Modem part,base on RT-Thread RTOS.
 *  now support to lorawan module:
 *         - LSD4WN-2N717M91(CN470S\CN470)
 *         - LSD4WN-2L717M90(CLAA)
 *         - LSD4WN-2L817M90(EU868)
 *         - LSD4WN-2K217M90(KR923)
 * Change Logs:
 * Date           Author            Notes
 * 2021-12-15                      the first version
 */
#ifndef __LORA_MODEM_DRIVER_H__
#define __LORA_MODEM_DRIVER_H__

#include <rtthread.h>
#include <at.h>

/*!
 * LoRa Modem software version number
 */
#define LORA_MODEM_SW_VERSION                 "0.5.0"

#ifndef PIN_LOW
#define PIN_LOW                 0x00
#endif
#ifndef PIN_HIGH
#define PIN_HIGH                0x01
#endif

#define LORA_MODEM_USING_LORAWAN_MAC_JOIN_ACCEPT2_MS       6000
#define LORA_MODEM_AT_CMD_CONNECT_WAIT_TIMEOUT             20
#define LORA_MODEM_DRIVER_USING_POWER_CONTROL_ENABLE       1
#define LORA_MODEM_DRIVER_USING_TEST_SHELL_EANBLE          1
#define LORA_MODEM_DRIVER_USING_BUSY_PIN_INTERRUPT_ENABLE  0
#define LORA_MODEM_SEND_TIMEOUT_MAX                        0xFFFFFFFF

#ifdef RT_USING_PIN
#define LORA_MODEM_WAKE_PIN_MODE_OUTPUT()     rt_pin_mode(LORA_MODEM_WAKE_PIN, PIN_MODE_OUTPUT)
#define LORA_MODEM_WAKE_PIN_MODE_INPUT()      rt_pin_mode(LORA_MODEM_WAKE_PIN, PIN_MODE_INPUT)
#define LORA_MODEM_MODE_PIN_MODE_OUTPUT()     rt_pin_mode(LORA_MODEM_MODE_PIN, PIN_MODE_OUTPUT)
#define LORA_MODEM_NRST_PIN_MODE_OUTPUT()     rt_pin_mode(LORA_MODEM_NRST_PIN, PIN_MODE_OUTPUT)
#define LORA_MODEM_NRST_PIN_MODE_INPUT()      rt_pin_mode(LORA_MODEM_NRST_PIN, PIN_MODE_INPUT)
#define LORA_MODEM_STAT_PIN_MODE_INPUT()      rt_pin_mode(LORA_MODEM_STAT_PIN, PIN_MODE_INPUT)
#define LORA_MODEM_BUSY_PIN_MODE_INPUT()      rt_pin_mode(LORA_MODEM_BUSY_PIN, PIN_MODE_INPUT)
#define LORA_MODEM_P0_PIN_MODE_INPUT()        rt_pin_mode(LORA_MODEM_P0_PIN, PIN_MODE_INPUT)
#define LORA_MODEM_P1_PIN_MODE_INPUT()        rt_pin_mode(LORA_MODEM_P1_PIN, PIN_MODE_INPUT)
#define LORA_MODEM_P2_PIN_MODE_INPUT()        rt_pin_mode(LORA_MODEM_P2_PIN, PIN_MODE_INPUT)

#define LORA_MODEM_NRST_PIN_WRITE_HIGH()      rt_pin_write(LORA_MODEM_NRST_PIN, PIN_HIGH)
#define LORA_MODEM_NRST_PIN_WRITE_LOW()       rt_pin_write(LORA_MODEM_NRST_PIN, PIN_LOW)
#define LORA_MODEM_WAKE_PIN_WRITE_HIGH()      rt_pin_write(LORA_MODEM_WAKE_PIN, PIN_HIGH)
#define LORA_MODEM_WAKE_PIN_WRITE_LOW()       rt_pin_write(LORA_MODEM_WAKE_PIN, PIN_LOW)
#define LORA_MODEM_MODE_PIN_WRITE_HIGH()      rt_pin_write(LORA_MODEM_MODE_PIN, PIN_HIGH)
#define LORA_MODEM_MODE_PIN_WRITE_LOW()       rt_pin_write(LORA_MODEM_MODE_PIN, PIN_LOW)

#define LORA_MODEM_BUSY_PIN_READ()            rt_pin_read(LORA_MODEM_BUSY_PIN)
#define LORA_MODEM_STAT_PIN_READ()            rt_pin_read(LORA_MODEM_STAT_PIN)

#else

#endif

#define AT_CMD_SEND(cmd, timeout)                                                              \
    do                                                                                                          \
    {                                                                                                           \
        if (at_exec_cmd(at_resp_set_info(lwm_obj_get()->at_resp, 128, 0, rt_tick_from_millisecond(timeout)), cmd) < 0)    \
        {                                                                                                       \
            goto __exit;                                                                                        \
        }                                                                                                       \
    } while(0);

#define AT_CMD_EXEC(cmd, timeout, ...)                      at_exec_cmd(at_resp_set_info(lwm_obj_get()->at_resp, 128, 0, rt_tick_from_millisecond(timeout)), cmd)
#define AT_RESP_PARSE_LINE_ARGS(resp_expr, ...)             at_resp_parse_line_args(lwm_obj_get()->at_resp, 2, resp_expr, ##__VA_ARGS__)
#define AT_RESP_PARSE_LINE_ARGS_BY_KW(cmd, resp_expr, ...)  at_resp_parse_line_args_by_kw(lwm_obj_get()->at_resp, cmd, resp_expr, ##__VA_ARGS__)
       
#define AT_CMD_EXEC_WICH_CHECK(cmd, timeout)                                                              \
    do                                                                                                          \
    {                                                                                                           \
        if (at_exec_cmd(at_resp_set_info(lwm_obj_get()->at_resp, 128, 0, rt_tick_from_millisecond(timeout)), cmd) < 0)    \
        {                                                                                                       \
            goto __exit;                                                                                        \
        }                                                                                                       \
    } while(0);
    
typedef enum
{
    LORA_MODEM_OPMODE_DEEP_SLEEP,
    LORA_MODEM_OPMODE_TRANSPARENT,
    LORA_MODEM_OPMODE_COMMAND,
    
    LORA_MODEM_OPMODE_MAX,
}lora_modem_opmode_t;

typedef enum lora_modem_status_tag
{
    LORA_MODEM_STATUS_COMM_UNINITIALIZED = 0x00, 
    LORA_MODEM_STATUS_COMM_OK,
    LORA_MODEM_STATUS_COMM_START_SEND,
    LORA_MODEM_STATUS_COMM_UART_RESP_FAULT,
    LORA_MODEM_STATUS_COMM_LW_TRX_OK,       
    LORA_MODEM_STATUS_COMM_LW_TRX_FAIL,      
    LORA_MODEM_STATUS_COMM_LW_TRX_FAULT,     /* if happened, maybe a serious question */
}lora_modem_status_t;

typedef enum lora_modem_status_sys_tag
{
    LORA_MODEM_STATUS_SYS_UNINITIALIZED = 0x00, 
    LORA_MODEM_STATUS_SYS_OK,
    LORA_MODEM_STATUS_SYS_START_SEND,
    LORA_MODEM_STATUS_SYS_BUSY,
    LORA_MODEM_STATUS_SYS_BUSY_FAULT,
    LORA_MODEM_STATUS_SYS_SLEEP_FAULT,
    LORA_MODEM_STATUS_SYS_WAKEUP_FAULT,
}lora_modem_status_sys_t;

struct lora_modem_common_configure
{
    uint32_t device_type             :2;
    uint32_t otaa                    :1;
    uint32_t confirm                 :1;
    uint32_t hot_start               :1;
    uint32_t dr                      :3;
    uint32_t reserved                :24;
};

struct lora_modem_status
{
    uint32_t init                    :1;
    uint32_t net_joined              :1;
    uint32_t sys_result              :4;
    uint32_t comm_result             :8;
    uint32_t rssi                    :8;
    uint32_t snr                     :8;
    uint32_t reserved                :2;
};

typedef void (*recv_callback_t)(uint8_t *data, uint8_t len);

struct lora_modem
{
    rt_device_t device;
    rt_sem_t rx_notice;
    rt_mutex_t lock;
    rt_sem_t resp_notice;
    rt_int32_t rx_done_wait;
    
    struct lora_modem_common_configure config;
    struct lora_modem_status status;
    lora_modem_opmode_t cur_opmode;
    
    /* the current received one line data buffer */
    uint8_t *recv_buf;
    /* The length of the currently received one line data */
    uint8_t recv_len;
    
    /* The maximum supported receive data length */
    uint8_t recv_bufsz;
    
    rt_err_t (*at_client_rx_ind)(rt_device_t dev, rt_size_t size);
    at_response_t at_resp;
    
    recv_callback_t recv_callback;
    
    rt_thread_t recv_thread;
};
typedef struct lora_modem *lora_modem_t;

extern int16_t lwm_init(const char *dev_name, uint8_t recv_bufsz, recv_callback_t recv_callback);
extern int16_t lwm_at_urc_register(struct at_urc *at_urc_table,size_t size);
extern int16_t lwm_data_send(const uint8_t *buf,uint8_t size,uint32_t timeout);
extern int16_t lwm_netconn(uint8_t timeout);
extern int16_t lwm_common_params_set(struct lora_modem_common_configure *conf);

extern void lwm_opmode_set(lora_modem_opmode_t opmode);
extern void lwm_hard_reset(void);
extern lora_modem_t lwm_obj_get(void);

#endif
