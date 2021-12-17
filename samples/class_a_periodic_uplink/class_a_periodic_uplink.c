/*
 * File      : lorawan class a periodic uplink
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-12-10      forest-rain   the first version 
 */

#include <rthw.h>
#include <rtdevice.h>
#include <stdlib.h>

#include "lora_modem.h"
#include "class_a_periodic_uplink.h" 

#define LOG_TAG     "lora.modem.app.class_a"    
#define LOG_LVL     LOG_LVL_DBG  
#include "lora_modem_debug.h"

struct lora_modem_common_configure app_class_a_common_conf = 
{
    .device_type = 0, /* class a */
    .otaa = 1,        /* otaa */
    .confirm = 0,     /* unconfirm */
    .hot_start = 0,   /* cold start */
};

rt_thread_t app_class_a_thread = NULL;
rt_sem_t periodic_uplink_sem;
int32_t periodic_uplink_wait;

static uint8_t deveui[8]={0};
static uint32_t tx_counter = 0;
static uint8_t tx_period_sec = 15; /* second */
static uint8_t user_data_len = 32;

app_packet_t app_packet = 
{
    .cmd     = 0x00,    // cmd id
    .up_fcnt = 0x00,    // fcnt
    .payload = {0x01,0x02,0x03},
};

#if LORA_MODEM_DRIVER_USING_AT_CLIENT_URC_ENABLE == 1
static void urc_uplink_log_func(struct at_client *client,const char *data, rt_size_t size);
static void urc_downlink_log_func(struct at_client *client,const char *data, rt_size_t size);
static void urc_join_log_func(struct at_client *client,const char *data, rt_size_t size);

static struct at_urc urc_table[] = 
{
    {"+JOIN INFO:", "\r\n", urc_join_log_func},
    {"[UP] Cfm_Stat:", "\r\n", urc_uplink_log_func},
    {"[DN] Ind_Stat:", "\r\n", urc_downlink_log_func},
};

static void urc_join_log_func(struct at_client *client,const char *data, rt_size_t size)
{
    LOG_D("%s",data);
}

static void urc_uplink_log_func(struct at_client *client,const char *data, rt_size_t size)
{
    LOG_D("%s",data);
}

static void urc_downlink_log_func(struct at_client *client,const char *data, rt_size_t size)
{
    LOG_D("%s",data);
}
#endif

void user_recv_callback(uint8_t *data, uint8_t len)
{
    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"len: %d\n", len);
    if(len)
    {
        //LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"%s",data);
    }
    
    /* light command */
    if( data[6] == 0x01 )
    {
        uint8_t off = 1;
        if(data[7] == 1)
        {
            off = 0;
        } 

#ifdef LED_RX_PIN
        rt_pin_write(LED_RX_PIN,off_level);
#endif        
    }
}

void app_class_a_params_get(void)
{
    AT_CMD_EXEC("AT+DEVEUI?",150);
    AT_RESP_PARSE_LINE_ARGS("%*[^:]: %02X %02X %02X %02X %02X %02X %02X %02X",
                                                                                 &deveui[0],
                                                                                 &deveui[1],
                                                                                 &deveui[2],
                                                                                 &deveui[3],
                                                                                 &deveui[4],
                                                                                 &deveui[5],
                                                                                 &deveui[6],
                                                                                 &deveui[7]);    
    
    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"DevEUI: %02X%02X%02X%02X%02X%02X%02X%02X",deveui[0],deveui[1],deveui[2],deveui[3],deveui[4],deveui[5],deveui[6],deveui[7]);
     
    uint8_t appeui[8]={0};
    AT_CMD_EXEC("AT+APPEUI?",150);
    AT_RESP_PARSE_LINE_ARGS("%*[^:]: %02X %02X %02X %02X %02X %02X %02X %02X",
                                                                                 &appeui[0],
                                                                                 &appeui[1],
                                                                                 &appeui[2],
                                                                                 &appeui[3],
                                                                                 &appeui[4],
                                                                                 &appeui[5],
                                                                                 &appeui[6],
                                                                                 &appeui[7]);    
    
    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"AppEUI: %02X%02X%02X%02X%02X%02X%02X%02X",appeui[0],appeui[1],appeui[2],appeui[3],appeui[4],appeui[5],appeui[6],appeui[7]);

    uint32_t baud_rate;
    AT_CMD_EXEC("AT+BAUD?",150);
    /* baud default of transparent is 9600 */
    if(AT_RESP_PARSE_LINE_ARGS_BY_KW("+BAUD:", "+BAUD:%d", &baud_rate)>0)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"Transparant Mode Baudrate:%d",baud_rate);
    }
    else
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"Baudrate read error");
    }
}

void app_class_a_params_config(void)
{
    rt_err_t result = RT_EOK;

    /* set lora modem common params */
    lwm_common_params_set(&app_class_a_common_conf);

    /* check lora modem at cmd connect ok */
    if(AT_CMD_EXEC("AT",50) < 0)
    {                    
        result = RT_ERROR;
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"AT Connect fail");
        goto __exit;
    }   
    else
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"AT Connect success");
    }
       
    /* turn on debug log */
    //AT_CMD_EXEC("AT+DEBUG=1",150);

__exit:

    if (result == RT_EOK)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"LoRa Modem at initialize success!");
    }
    else
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LoRa Modem at initialize failed (%d)!", result);
    }
}

/*!
 * @ brief  app_class_a_thread_entry 
 * @ para   None
 * @ return None
 */
void app_class_a_thread_entry(void* parameter)
{       
    /* hard reset lora modem */
    lwm_hard_reset();
    
    /* init lora modem,register user  */
    lwm_init(LORA_MODEM_UART_DEVICE,255,user_recv_callback);
    
#if LORA_MODEM_DRIVER_USING_AT_CLIENT_URC_ENABLE == 1
    /* register at urc for command mode */
    lwm_at_urc_register(urc_table,sizeof(urc_table) / sizeof(urc_table[0]));
#endif    
    
    /* set lora modem params depend on applications */
    app_class_a_params_config();
    
    /* display key params for debug */
    app_class_a_params_get();
    
    /* join timeout after 120 second */
    if(lwm_netconn(120) == RT_EOK)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"lorawan network connected,begin to periodic(%d sec) uplink data\r\n",tx_period_sec);
        periodic_uplink_wait = tx_period_sec * RT_TICK_PER_SECOND;
    }
    else
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"lorawan network connect failed,check DevEUI¡¢AppEUI¡¢AppKey..\r\n");
        periodic_uplink_wait = RT_WAITING_FOREVER;
    }
    
    while(1)
    {
        if(rt_sem_take(periodic_uplink_sem, periodic_uplink_wait) != RT_EOK)
        {     
            uint8_t send_result;
                                    
            tx_counter++;
            LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"[%02X%02X%02X%02X%02X%02X%02X%02X]:Class%c,%s Start,%s,tx period:%ds,Send Packet Counter:%d,size:%d\r\n",
                                                          deveui[0],deveui[1],deveui[2],deveui[3],deveui[4],deveui[5],deveui[6],deveui[7], \
                                                          "ABC"[lwm_obj_get()->config.device_type & 0x03],
                                                          ((lwm_obj_get()->config.hot_start == 1)?"Hot":"Cold"),
                                                          ((lwm_obj_get()->config.confirm == 1)?"cfm":"uncfm"),
                                                          tx_period_sec,
                                                          tx_counter,
                                                          user_data_len);

            {
                app_packet.cmd     = 0x00;
                app_packet.up_fcnt = tx_counter; // 1~4
                
                send_result = lwm_data_send((uint8_t *)&app_packet.cmd, user_data_len, LORA_MODEM_SEND_TIMEOUT_MAX);
                if( send_result == LORA_MODEM_STATUS_COMM_LW_TRX_OK )
                {
                    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"LoRaWAN TRX OK\n");
                }
                else if( send_result == LORA_MODEM_STATUS_SYS_BUSY )
                {
                    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_INFO,"LoRa Modem BUSY now, Send Break!\n");
                }
                else if( send_result == LORA_MODEM_STATUS_COMM_UART_RESP_FAULT )
                {
                    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LoRa Modem Uart No Response\n");
                }
            }
        }        
    }
}

void app_class_a_thread_init(void)
{
    if( app_class_a_thread == RT_NULL )
    {
        app_class_a_thread = rt_thread_create("periodic-up",
                                            app_class_a_thread_entry, 
                                            RT_NULL,
                                            8192, 
                                            RT_THREAD_PRIORITY_MAX/2-3,
                                            10);
        if (app_class_a_thread != RT_NULL)
            rt_thread_startup(app_class_a_thread);
        
        periodic_uplink_sem = rt_sem_create("periodic-sem", 0, RT_IPC_FLAG_FIFO);
        if (periodic_uplink_sem == RT_NULL)
        {
            LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"periodic_uplink_sem semaphore create failed!");
        }
    }
}

#if LORA_MODEM_DRIVER_USING_TEST_SHELL_EANBLE == 1

/* for finish\msh */
#define CMD_APP_CLASS_A_PROBE_INDEX       0 // lora modem probe
#define CMD_APP_CLASS_A_suspend_INDEX     1 // lora modem class_a app suspend
#define CMD_APP_CLASS_A_RESUME_INDEX      2 // lora modem class_a app resume

const char* lora_modem_classa_help_info[] = 
{
    [CMD_APP_CLASS_A_PROBE_INDEX]         = "lwma probe    - lora modem class_a probe",
    [CMD_APP_CLASS_A_suspend_INDEX]       = "lwma suspend  - class_a app suspend",   
    [CMD_APP_CLASS_A_RESUME_INDEX]        = "lwma resume   - class_a app resume",   
};

/* lora modem class_a app Test Shell */
static int lwma(int argc, char *argv[])
{
    size_t i = 0;
    
    if (argc < 2)
    {   
        /* parameter error */
        rt_kprintf("LoRa Modem(%s) Usage:\n",LORA_MODEM_SW_VERSION);
        for (i = 0; i < sizeof(lora_modem_classa_help_info) / sizeof(char*); i++) 
        {
            rt_kprintf("%s\n", lora_modem_classa_help_info[i]);
        }
        rt_kprintf("\n");
    } 
    else 
    {
        const char *cmd0 = argv[1];
				
        if(lwm_init(LORA_MODEM_UART_DEVICE,255,user_recv_callback) != RT_EOK)
        {
            rt_kprintf("LoRa Modem Init Fail");
            return 0;
        }
        
        if (!rt_strcmp(cmd0, "probe")) 
        {   
            if(lwm_obj_get() != RT_NULL)
            {
                rt_kprintf("LoRa Modem(%s) Probe ok!\n",lwm_obj_get()->device->parent.name);
            }
            else
            {
                rt_kprintf("LoRa Modem(%s) Probe failed!\n",lwm_obj_get()->device->parent.name);
            }
        }
        else if (!rt_strcmp(cmd0, "suspend")) 
        {
            periodic_uplink_wait = RT_WAITING_FOREVER;
            rt_sem_release(periodic_uplink_sem);
            rt_kprintf("LoRa Modem(%s) Periodic Uplink suspend!\n",lwm_obj_get()->device->parent.name);
        }
        else if (!rt_strcmp(cmd0, "resume")) 
        {
            periodic_uplink_wait = tx_period_sec * RT_TICK_PER_SECOND;
            rt_sem_release(periodic_uplink_sem);
            rt_kprintf("LoRa Modem(%s) Periodic Uplink Resume!\n",lwm_obj_get()->device->parent.name);
        }
        else if (!rt_strcmp(cmd0, "tx")) 
        {
            if( argc >= 3 )
            {
                /* payload_len for setup tx payload length */
                tx_period_sec = atol(argv[2]);
                
                if(tx_period_sec)
                {
                    periodic_uplink_wait = tx_period_sec * RT_TICK_PER_SECOND;
                }
                else
                {
                    periodic_uplink_wait = RT_WAITING_FOREVER;
                }
                
                if( argc >= 4 )
                {
                    /* data length */
                   user_data_len = atoi(argv[3]);
                }
            }
        }
    }
    return 1;
}    
MSH_CMD_EXPORT(lwma, lora modem class_a test shell);
#endif
