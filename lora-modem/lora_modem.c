/*
 * File      : lora-modem-driver.c
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
#include <stdlib.h>
#include "rtdevice.h"
#include "rthw.h"
#include "lora_modem.h"

#define LOG_TAG     "lora.modem" 
#define LOG_LEVEL     LOG_LVL_DBG 
#include "lora_modem_debug.h"           

#define LORA_MODEM_LOCK_NAME            "lm_c"
#define LORA_MODEM_SEM_NAME             "lm_s"
#define LORA_MODEM_SEM_RXDONE_NAME      "lm_rd"
#define LORA_MODEM_RESP_NAME            "lm_r"
#define LORA_MODEM_THREAD_NAME          "lm_t"

static struct lora_modem lora_modem_obj = { 0 };

static char* lora_modem_mode_string[LORA_MODEM_OPMODE_MAX] = {"SLEEP","TRAN","CMD"};

#if LORA_MODEM_DRIVER_USING_BUSY_PIN_INTERRUPT_ENABLE == 1
static void lwm_busy_pin_interrupt_callback(void *args)
{
    rt_pin_irq_enable(LORA_MODEM_BUSY_PIN, PIN_IRQ_SUSPEND);

    rt_sem_release(lora_modem_obj.resp_notice);
}
#endif

/**
 * @brief initlize lora modem gpio
 * 
 */
void lwm_io_init(void)
{ 
    LORA_MODEM_NRST_PIN_MODE_INPUT();
    
    /* wake-pin */
    LORA_MODEM_WAKE_PIN_MODE_OUTPUT(); 
    /* default set lora-modem to deep mode */
    LORA_MODEM_WAKE_PIN_WRITE_LOW();
    
    /* mode-pin */
    LORA_MODEM_MODE_PIN_MODE_OUTPUT(); 
   
    {   
        /* set lora modem to command mode��config parameters,eg:APPEUI��APPKEY...�� */
        LORA_MODEM_MODE_PIN_WRITE_HIGH();
    }

    /* stat-pin */
    LORA_MODEM_STAT_PIN_MODE_INPUT(); 
    
    /* busy-pin */
    LORA_MODEM_BUSY_PIN_MODE_INPUT(); 
    
#if LORA_MODEM_DRIVER_USING_BUSY_PIN_INTERRUPT_ENABLE == 1
    rt_pin_attach_irq(LORA_MODEM_BUSY_PIN, PIN_IRQ_MODE_RISING,lwm_busy_pin_interrupt_callback,(void*)"busy irq");
    rt_pin_irq_enable(LORA_MODEM_BUSY_PIN, PIN_IRQ_ENABLE); 
    rt_pin_irq_enable(LORA_MODEM_BUSY_PIN, PIN_IRQ_SUSPEND);
#endif

#ifdef LW470E_P0_PIN     
    LORA_MODEM_P0_PIN_MODE_INPUT(); 
#endif

#ifdef LW470E_P1_PIN     
    LORA_MODEM_P1_PIN_MODE_INPUT(); 
#endif

#ifdef LW470E_P2_PIN     
    LORA_MODEM_P2_PIN_MODE_INPUT(); 
#endif

#ifdef LW470E_P3_PIN     
    LORA_MODEM_P3_GPIO_INIT_INPUT(); 
#endif
}

static rt_err_t lwm_rx_ind(rt_device_t dev, rt_size_t size)
{
    if (lora_modem_obj.device == dev && size > 0)
    {
        lora_modem_obj.recv_len = size;

        rt_sem_release(lora_modem_obj.rx_notice);
    }

    return RT_EOK;
}

static void lwm_recv_thread(lora_modem_t modem)
{
    rt_err_t sem_result;
    uint8_t recv_len = 0;
    uint8_t read_len = 0;
    lora_modem_obj.rx_done_wait = RT_WAITING_FOREVER;
    
    while(1)
    {
        /* wait for received data */
        sem_result = rt_sem_take(lora_modem_obj.rx_notice, lora_modem_obj.rx_done_wait);
        
        if(lora_modem_obj.cur_opmode == LORA_MODEM_OPMODE_TRANSPARENT && lora_modem_obj.status.net_joined)
        {
            if(sem_result == -RT_ETIMEOUT)
            {   /* a frame receive timeout */
                
                lora_modem_obj.rx_done_wait = RT_WAITING_FOREVER;
                
                lora_modem_obj.recv_len = recv_len;
                recv_len = 0;
                LORA_MODEM_DEBUG_LOG_HEXDUMP(LWM_DBG_DATA_HEX_DUMP,lora_modem_obj.recv_buf,lora_modem_obj.recv_len);
                lora_modem_obj.recv_callback(lora_modem_obj.recv_buf, lora_modem_obj.recv_len);
                lora_modem_obj.recv_len = 0;
            }
            else
            {
                /* setup a frame receive timeout */
                lora_modem_obj.rx_done_wait = rt_tick_from_millisecond(200);

                if( recv_len > lora_modem_obj.recv_bufsz )
                {
                    /* over maximum size */
                    continue;
                }
                else
                {
                    /* read bytes from uart，if not, wait timeout */
                    read_len = rt_device_read(lora_modem_obj.device, 0, lora_modem_obj.recv_buf+recv_len, lora_modem_obj.recv_len);
                    if (read_len > 0)
                    {
                         recv_len += read_len;
                    }
                }
            }
        }
    }
}

/* initialize the lora modem object */
static int lwm_para_init(lora_modem_t modem)
{   
    int result = RT_EOK;
    char name[RT_NAME_MAX];
    
    modem->status.net_joined = 0;
    modem->status.comm_result = 0;
    modem->recv_len = 0;
    
    modem->recv_buf = (uint8_t *) rt_calloc(1, modem->recv_bufsz);
    if (modem->recv_buf == RT_NULL)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LWM initialize failed! No memory for receive buffer.");
        result = -RT_ENOMEM;
        goto __exit;
    }
    
    modem->at_resp = at_create_resp(128, 0, rt_tick_from_millisecond(300));
    if (!modem->at_resp)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"No memory for at response structure!");
        result = -RT_ENOMEM;
        goto __exit;
    }
    
    rt_snprintf(name, RT_NAME_MAX, "%s", LORA_MODEM_LOCK_NAME);
    modem->lock = rt_mutex_create(name, RT_IPC_FLAG_FIFO);
    if (modem->lock == RT_NULL)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LWM initialize failed! lora_modem_recv_lock create failed!");
        result = -RT_ENOMEM;
        goto __exit;
    }

    rt_snprintf(name, RT_NAME_MAX, "%s", LORA_MODEM_SEM_NAME);
    modem->rx_notice = rt_sem_create(name, 0, RT_IPC_FLAG_FIFO);
    if (modem->rx_notice == RT_NULL)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LWM initialize failed! lora_modem_notice semaphore create failed!");
        result = -RT_ENOMEM;
        goto __exit;
    }
    
    rt_snprintf(name, RT_NAME_MAX, "%s", LORA_MODEM_RESP_NAME);
    modem->resp_notice = rt_sem_create(name, 0, RT_IPC_FLAG_FIFO);
    if (modem->resp_notice == RT_NULL)
    {
        LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LWM initialize failed! lora_modem_resp semaphore create failed!");
        result = -RT_ENOMEM;
        goto __exit;
    }
    
    rt_snprintf(name, RT_NAME_MAX, "%s", LORA_MODEM_THREAD_NAME);
    modem->recv_thread = rt_thread_create(name,
                                     (void (*)(void *parameter))lwm_recv_thread,
                                     modem,
                                     1024 + 512,
                                     RT_THREAD_PRIORITY_MAX / 3 - 1,
                                     5);
    if (modem->recv_thread == RT_NULL)
    {
        result = -RT_ENOMEM;
        goto __exit;
    }

__exit:
    if (result != RT_EOK)
    {
        if (modem->lock)
        {
            rt_mutex_delete(modem->lock);
        }

        if (modem->rx_notice)
        {
            rt_sem_delete(modem->rx_notice);
        }

        if (modem->resp_notice)
        {
            rt_sem_delete(modem->resp_notice);
        }
        
        if (modem->at_resp)
        {
            at_delete_resp(modem->at_resp);
        }

        if (modem->device)
        {
            rt_device_close(modem->device);
        }
        
        rt_memset(modem, 0x00, sizeof(struct lora_modem));
    }

    return result;
}

/**
 * @brief initlize lora modem device,register a receive callback
 * 
 * @param dev_name 
 * @param recv_bufsz 
 * @param recv_callback 
 * @return int16_t 
 */
int16_t lwm_init(const char *dev_name, uint8_t recv_bufsz, recv_callback_t recv_callback)
{
    int16_t result = RT_EOK;
    
    RT_ASSERT(dev_name);
    RT_ASSERT(recv_bufsz > 0);
    
    if(!lora_modem_obj.status.init)
    {
        lwm_io_init();

        lora_modem_obj.recv_callback = recv_callback;
        lora_modem_obj.recv_bufsz = recv_bufsz;
        
        result = lwm_para_init(&lora_modem_obj);
        
        /* find and open device */
        lora_modem_obj.device = rt_device_find(dev_name);
        if(lora_modem_obj.device)
        {
            /* register lwm_rx_ind for transparent mode */
            rt_device_set_rx_indicate(lora_modem_obj.device, lwm_rx_ind);
            
            /* init at client for command mode */
            at_client_init(dev_name,recv_bufsz);
            
            /* copy at client rx ind for command mode */
            lora_modem_obj.at_client_rx_ind = at_client_get_first()->device->rx_indicate;
            
            /* start recv thread for transparent mode */
            rt_thread_startup(lora_modem_obj.recv_thread);
        }
        else
        {
            result = -RT_ERROR;
        }
        
        lora_modem_obj.status.init = 1;
    }
    
    return result;
}

/**
 * @brief register at urc table for command mode
 * 
 * @param at_urc_table 
 * @param size 
 * @return int16_t 
 */
int16_t lwm_at_urc_register(struct at_urc *at_urc_table,size_t size)
{
    int16_t result = RT_EOK;
#if LORA_MODEM_DRIVER_USING_AT_CLIENT_URC_ENABLE == 1
    if(at_urc_table != RT_NULL)
    {
        /* URC register */
        at_set_urc_table(at_urc_table,size);
    }
    else
    {
        result = -RT_ERROR;
    }
#else
    result = -RT_ERROR;     
#endif
    return result;
}

/**
 * @brief lora modem hard reset
 * 
 */
void lwm_hard_reset(void)
{  
    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"lora modem start to hard reset.\n");
    /* hard reset lora modem */
    LORA_MODEM_NRST_PIN_MODE_OUTPUT(); 
    LORA_MODEM_NRST_PIN_WRITE_LOW();
    rt_hw_us_delay(100);
    // lw470e pullup internal
    LORA_MODEM_NRST_PIN_MODE_INPUT();
    
    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"lora modem hard reset done.\n");
}
MSH_CMD_EXPORT(lwm_hard_reset, LoRa Modem Hard Reset);

static void _lwm_opmode_set(lora_modem_opmode_t opmode)
{  
    uint32_t start_time;
    uint8_t read_busy_pin;
    
    if(lora_modem_obj.cur_opmode == opmode)
    {
        return;
    }
    
    if( opmode == LORA_MODEM_OPMODE_DEEP_SLEEP )
    {  
        /* set lora modem transparent mode */
        LORA_MODEM_MODE_PIN_WRITE_LOW();
            
        /* set lora modem sleep */
        LORA_MODEM_WAKE_PIN_WRITE_LOW();
    }
    else 
    {
        if(lora_modem_obj.cur_opmode == LORA_MODEM_OPMODE_DEEP_SLEEP)
        {
            /* wakeup lora modem first */        
            //LORA_MODEM_WAKE_PIN_MODE_OUTPUT(); 
            LORA_MODEM_WAKE_PIN_WRITE_HIGH();
            
            start_time = rt_tick_get();
            
            do
            {
                /* Check whether lora modem wakeup */
                rt_sem_take(lora_modem_obj.resp_notice, rt_tick_from_millisecond(15));
             
                if (rt_tick_get() - start_time > rt_tick_from_millisecond(150))
                {
                    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LWM WAKEUP Fault->");
                    
                    lora_modem_obj.status.sys_result = LORA_MODEM_STATUS_SYS_WAKEUP_FAULT;
                    
                    goto __exit;
                }
            }
            while(LORA_MODEM_BUSY_PIN_READ() == PIN_LOW);
        }

        if(opmode == LORA_MODEM_OPMODE_TRANSPARENT)
        {
            /* switch to transparent mode */
            LORA_MODEM_MODE_PIN_WRITE_LOW();
#if LORA_MODEM_DRIVER_USING_AT_CLIENT_URC_ENABLE == 0
            /* register lwm_rx_ind for transparent mode */
            rt_device_set_rx_indicate(lora_modem_obj.device, lwm_rx_ind);
#endif            
        }
        else
        {
            /* register at_client_rx_ind for command mode */
            LORA_MODEM_MODE_PIN_WRITE_HIGH();
            
            rt_device_set_rx_indicate(lora_modem_obj.device, lora_modem_obj.at_client_rx_ind); 
        }
    }

    start_time = rt_tick_get();
    while(1)
    {
        /* wait for lora modem opmode setup ready */
        rt_sem_take(lora_modem_obj.resp_notice, rt_tick_from_millisecond(15));
        
        read_busy_pin = LORA_MODEM_BUSY_PIN_READ();
        
        /* check busy state for sleep mode and tranparent mode,command mode quit directly */
        if(((opmode == LORA_MODEM_OPMODE_DEEP_SLEEP) && (read_busy_pin == PIN_LOW)) ||\
            (opmode == LORA_MODEM_OPMODE_COMMAND) || \
           ((opmode == LORA_MODEM_OPMODE_TRANSPARENT) && \
            (lora_modem_obj.status.net_joined ? (read_busy_pin == PIN_HIGH) : (read_busy_pin == PIN_LOW))))
        {
            /* it is ok */
            lora_modem_obj.status.comm_result = LORA_MODEM_STATUS_COMM_OK;
            break;
        }
        
        if (rt_tick_get() - start_time > rt_tick_from_millisecond(300))
        {
            /* timeout happened */
            if(read_busy_pin == PIN_HIGH)
            {
                if(opmode == LORA_MODEM_OPMODE_DEEP_SLEEP)
                    lora_modem_obj.status.sys_result = LORA_MODEM_STATUS_SYS_SLEEP_FAULT;
            }
            else
            {   
                /* BUSY pin is low */
                if(opmode == LORA_MODEM_OPMODE_TRANSPARENT)
                    lora_modem_obj.status.sys_result = LORA_MODEM_STATUS_SYS_BUSY_FAULT;
            }
            
            LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_ERROR,"LWM %s Fault->",lora_modem_mode_string[opmode]);
            break;
        }
    }
    
    lora_modem_obj.cur_opmode = opmode;  
    
__exit: 
    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"LWM GOTO %s MODE(now:BUSY=%d,STAT=%d)",lora_modem_mode_string[opmode],LORA_MODEM_BUSY_PIN_READ(),LORA_MODEM_STAT_PIN_READ());
}

/*!
 * @ brief  lora modem opmode set 
 * @ para   opmode - lora modem opmode
 * @ return None
 */
void lwm_opmode_set(lora_modem_opmode_t opmode)
{
    rt_mutex_take(lora_modem_obj.lock, RT_WAITING_FOREVER);
    _lwm_opmode_set(opmode);
    rt_mutex_release(lora_modem_obj.lock);
}

/**
 * @brief  join lorawan network
 *
 * @param timeout second for timeout
 *
 * @return 0 : success
 *        -2 : timeout
 */
int16_t lwm_netconn(uint8_t timeout)
{
    rt_err_t result = RT_EOK;
    uint32_t start_time = 0;
    /* once join timeout */
    uint32_t once_join_network_timeout = rt_tick_from_millisecond(LORA_MODEM_USING_LORAWAN_MAC_JOIN_ACCEPT2_MS);

    if(lora_modem_obj.status.net_joined)
    {
        return RT_EOK;
    }
    
    if(lora_modem_obj.config.hot_start || lora_modem_obj.config.otaa == 0)
    {
        /* Check whether it is already connected */
        if((LORA_MODEM_STAT_PIN_READ() == PIN_HIGH) && (LORA_MODEM_BUSY_PIN_READ() == PIN_HIGH))
        {
            lora_modem_obj.status.net_joined = 1;
            return RT_EOK;
        }
    }

    /* set lora modem to transparent mode then start to join */
    _lwm_opmode_set(LORA_MODEM_OPMODE_TRANSPARENT);
    
    rt_mutex_take(lora_modem_obj.lock, RT_WAITING_FOREVER);

    start_time = rt_tick_get();

    while (1)
    {
        /* wait lora modem once join done */
        rt_sem_take(lora_modem_obj.resp_notice, once_join_network_timeout);
        
        /* Check whether it is already connected */
        if((LORA_MODEM_STAT_PIN_READ() == PIN_LOW) && (LORA_MODEM_BUSY_PIN_READ() == PIN_LOW))
        {
            LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"LWM network connecting(%d sec).", (rt_tick_get() - start_time)/ RT_TICK_PER_SECOND);
        }
        else
        {
            lora_modem_obj.status.net_joined = 1;
            break;
        }
        
        /* Check whether join timeout */
        if (rt_tick_get() - start_time > (timeout * RT_TICK_PER_SECOND))
        {
            LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"LWM Network Connect Timeout(%d sec).",timeout);
            result = -RT_ETIMEOUT;
            /* goto sleep after join failed*/
            _lwm_opmode_set(LORA_MODEM_OPMODE_DEEP_SLEEP);
            break;
        }
    }

    rt_mutex_release(lora_modem_obj.lock);

    return result;
}

/**
 * @brief Send data to lorawan server
 *
 * @param modem current LWM object
 * @param buf   send data buffer
 * @param size  send fixed data size
 *
 * @return >0: send data size
 *         =0: send failed
 */
int16_t lwm_data_send(const uint8_t *buf,uint8_t size,uint32_t timeout)
{
    int16_t result = LORA_MODEM_STATUS_COMM_OK;
    uint32_t start_time = 0;
 
    if(timeout == LORA_MODEM_SEND_TIMEOUT_MAX)
    {
        /* 60 second is enough for most cases, depend on confirm nbtrials、datarate、pending and so on*/
        timeout = 60;
    }
    
    rt_mutex_take(lora_modem_obj.lock, RT_WAITING_FOREVER);

#if LORA_MODEM_DRIVER_USING_POWER_CONTROL_ENABLE == 1
    /* wakeup lora modem then keep on transparent mode */
    _lwm_opmode_set(LORA_MODEM_OPMODE_TRANSPARENT);
#endif
    lora_modem_obj.status.comm_result = LORA_MODEM_STATUS_COMM_START_SEND;
    
    /* check lora modem busy state */
    if(LORA_MODEM_BUSY_PIN_READ() == PIN_LOW)
    {
        /* lora modem may be busy to send auto ack */
        rt_thread_mdelay(10);
        if(LORA_MODEM_BUSY_PIN_READ() == PIN_LOW)
        {
            result = lora_modem_obj.status.sys_result = LORA_MODEM_STATUS_SYS_BUSY;
            goto __exit;
        }
    }

    start_time = rt_tick_get();
    
    size = rt_device_write(lora_modem_obj.device, 0, buf, size);

    /* wait a while for lora modem's uart get a valid frame done */
    rt_sem_take(lora_modem_obj.resp_notice, rt_tick_from_millisecond(15));
    
    if(LORA_MODEM_BUSY_PIN_READ() == PIN_HIGH)
    {
        /* lora modem's uart don't response */
        result = lora_modem_obj.status.comm_result = LORA_MODEM_STATUS_COMM_UART_RESP_FAULT;
        goto __exit;
    }
   
    while (1)
    {
#if LORA_MODEM_DRIVER_USING_BUSY_PIN_INTERRUPT_ENABLE == 1        
        // todo
#else        
        rt_sem_take(lora_modem_obj.resp_notice, rt_tick_from_millisecond(500));
#endif        
        /* Check whether lora modem txdone */
        if(LORA_MODEM_BUSY_PIN_READ() == PIN_HIGH)
        {
            /* check communicate result,valid for uplink confirm frame */
            if(LORA_MODEM_STAT_PIN_READ() == PIN_HIGH)
            {
                lora_modem_obj.status.comm_result = LORA_MODEM_STATUS_COMM_LW_TRX_OK;
            }
            else
            {
                lora_modem_obj.status.comm_result = LORA_MODEM_STATUS_COMM_LW_TRX_FAIL;
            }
            break;
        }
        
        /* Check whether send fault,todo: here maybe missed auto-ack-frame if timeout is not enough */
        if (rt_tick_get() - start_time > (timeout * RT_TICK_PER_SECOND))
        {
            lora_modem_obj.status.comm_result = LORA_MODEM_STATUS_COMM_LW_TRX_FAULT;
            break;
        }
    }
    result = lora_modem_obj.status.comm_result;

__exit:

    LORA_MODEM_DEBUG_LOG(LWM_DBG_CORE, LOG_LVL_DBG,"LWM Send Result:%d,BUSY:%d,STAT:%d",lora_modem_obj.status.comm_result,LORA_MODEM_BUSY_PIN_READ(),LORA_MODEM_STAT_PIN_READ());
    
#if LORA_MODEM_DRIVER_USING_POWER_CONTROL_ENABLE == 1
    /* set lora modem go to dsleep */
    _lwm_opmode_set(LORA_MODEM_OPMODE_DEEP_SLEEP); 
#endif    
    
    rt_mutex_release(lora_modem_obj.lock);

    return result;
}

int16_t lwm_common_params_set(struct lora_modem_common_configure *conf)
{
    int16_t result = RT_EOK;

    rt_mutex_take(lora_modem_obj.lock, RT_WAITING_FOREVER);

    _lwm_opmode_set(LORA_MODEM_OPMODE_COMMAND);

    lora_modem_obj.config = *conf;
    
    if(lora_modem_obj.config.otaa == 0 || lora_modem_obj.config.hot_start == 1)
    {
        /* factory default is otaa(1) ,cold start(0) */
        if(AT_CMD_EXEC("AT+OTAA=%d,%d",150,lora_modem_obj.config.otaa,lora_modem_obj.config.hot_start) <0 )
        {
            result = -RT_ERROR;
        }
    }
    
     if(lora_modem_obj.config.confirm == 1)
     {
          /* factory default is unconfirm(0)*/
         if(AT_CMD_EXEC("AT+CONFIRM=%d",150,lora_modem_obj.config.confirm) < 0)
         {
            result = -RT_ERROR;
         }
     }

     if(lora_modem_obj.config.device_type != 0)
     {
          /* factory default is class a(0)*/
         if(AT_CMD_EXEC("AT+CLASS=%d",150,lora_modem_obj.config.device_type) < 0)
         {
            result = -RT_ERROR;
         }
     }

     if(AT_CMD_EXEC("AT+SAVE",300) < 0)
     {
        result = -RT_ERROR;
     }
         
     rt_mutex_release(lora_modem_obj.lock);
     
     return result;
}

/**
 * @brief get lora modem object.
 *
 * @return lora modem object
 */
lora_modem_t lwm_obj_get(void)
{
    return &lora_modem_obj;
}

#if LORA_MODEM_DRIVER_USING_TEST_SHELL_EANBLE == 1

/* for finish\msh */
#define CMD_LORA_MODEM_PROBE_INDEX       0 // lora modem probe
#define CMD_LORA_MODEM_CMD_MODE_INDEX    1 // lora modem command mode
#define CMD_LORA_MODEM_TRAN_MODE_INDEX   2 // lora modem transperent mode
#define CMD_LORA_MODEM_PINS_READ_INDEX   3 // lora modem pins read
#define CMD_LORA_MODEM_PARAMS_CFG_INDEX  4 // lora modem common paras config

const char* lora_modem_help_info[] = 
{
    [CMD_LORA_MODEM_PROBE_INDEX]         = "lwm probe    - lora modem probe",
    [CMD_LORA_MODEM_CMD_MODE_INDEX]      = "lwm cmd      - switch to command mode",
    [CMD_LORA_MODEM_TRAN_MODE_INDEX]     = "lwm tran     - switch to transparent mode",   
    [CMD_LORA_MODEM_PINS_READ_INDEX]     = "lwm read     - busy & stat pin status read",   
    [CMD_LORA_MODEM_PARAMS_CFG_INDEX]    = "lwm cfg      - common params config", 
};

/* lora modem Test Shell */
static int lwm(int argc, char *argv[])
{
    size_t i = 0;
    
    if (argc < 2)
    {   
        /* parameter error */
        rt_kprintf("LoRa Modem(%s) Usage:\n",LORA_MODEM_SW_VERSION);
        for (i = 0; i < sizeof(lora_modem_help_info) / sizeof(char*); i++) 
        {
            rt_kprintf("%s\n", lora_modem_help_info[i]);
        }
        rt_kprintf("\n");
    } 
    else 
    {
        const char *cmd0 = argv[1];
				
        if(!lwm_obj_get()->status.init)
        {
            rt_kprintf("LoRa Modem Not Init");
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
        else if (!rt_strcmp(cmd0, "cmd")) 
        {
            lwm_opmode_set(LORA_MODEM_OPMODE_COMMAND);
        }
        else if (!rt_strcmp(cmd0, "tran")) 
        {
            lwm_opmode_set(LORA_MODEM_OPMODE_TRANSPARENT);
        }
        else if (!rt_strcmp(cmd0, "read")) 
        {
            rt_kprintf("LWM BUSY:%d,STAT:%d\n",LORA_MODEM_BUSY_PIN_READ(),LORA_MODEM_STAT_PIN_READ());
        }
        else if (!rt_strcmp(cmd0, "cfg")) 
        {
            struct lora_modem_common_configure common_conf = lora_modem_obj.config;

            if(argc >= 3)
            {
                /* device class type */
                common_conf.device_type = atoi(argv[2]);
                if(argc >= 4)
                {
                    common_conf.otaa = atoi(argv[3]);
                    
                    if(argc >= 5)
                    {
                        common_conf.confirm = atoi(argv[4]);

                        if(argc >= 6)
                        {
                            common_conf.hot_start = atoi(argv[5]);
                        }
                    }
                }
                lwm_common_params_set(&common_conf);
            }
        }
    }
    return 1;
}    
MSH_CMD_EXPORT(lwm, lora modem test shell);

#endif
