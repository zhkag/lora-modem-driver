/*!
 * \file      lora-modem-debug.h
 *
 * \brief     control all LoRa Modem Driver debug features.
 *
 * \copyright SPDX-License-Identifier: Apache-2.0
 *
 * \author    Forest-Rain
 */
 
#ifndef __LORA_MODEM_DEBUG_H__
#define __LORA_MODEM_DEBUG_H__

#ifdef RT_USING_ULOG
#include <rtdbg.h>
#include <ulog.h> 
#else

#endif

/* Using this macro to control all LoRa Modem Driver debug features. */
#ifdef LORA_MODEM_DRIVER_USING_LORA_MODEM_DEBUG

/* Turn on some of these (set to non-zero) to debug LoRa Modem */

/* application */
#ifndef LWM_DBG_APP
#define LWM_DBG_APP                          0
#endif

/* core */
#ifndef LWM_DBG_CORE
#define LWM_DBG_CORE                    0
#endif

/* data hex dump */
#ifndef LWM_DBG_DATA_HEX_DUMP
#define LWM_DBG_DATA_HEX_DUMP                    0
#endif

#if defined RT_USING_ULOG
#define LORA_MODEM_DEBUG_LOG(type, level, ...)                                \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
    {                                                                         \
        ulog_output(level, LOG_TAG, RT_TRUE, __VA_ARGS__);                    \
    }                                                                         \
}                                                                             \
while (0)

#define LORA_MODEM_DEBUG_LOG_RAW(type, ...)                                   \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
    {                                                                         \
        ulog_raw(__VA_ARGS__);                                                \
    }                                                                         \
}                                                                             \
while (0)

#define LORA_MODEM_DEBUG_LOG_HEXDUMP(type, buf, size)                         \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
    {                                                                         \
        ulog_hexdump(LOG_TAG, 16, buf, size);                                 \
    }                                                                         \
}                                                                             \
while (0)

#else

#define LORA_MODEM_DEBUG_LOG(type, level, ...)                                \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
    {                                                                         \
        rt_kprintf(__VA_ARGS__);                                              \
        rt_kprintf("\r\n");                                                   \
    }                                                                         \
}                                                                             \
while (0)

#define LORA_MODEM_DEBUG_LOG_RAW(type, ...)                                   \
do                                                                            \
{                                                                             \
    if (type)                                                                 \
    {                                                                         \
        rt_kprintf(__VA_ARGS__);                                              \
        rt_kprintf("\r\n");                                                   \
    }                                                                         \
}    

#define LORA_MODEM_DEBUG_LOG_HEXDUMP(type, buf, size)                      
                                                                          
#endif

#else /* LORA_MODEM_DEBUG */

#define LORA_MODEM_DEBUG_LOG(type, level, ...)
#define LORA_MODEM_DEBUG_LOG_RAW(type, ...)  
#define LORA_MODEM_DEBUG_LOG_HEXDUMP(type, buf, size)  

#endif /* LORA_MODEM_DEBUG */

#endif /* __LORA_MODEM_DEBUG_H__ */
