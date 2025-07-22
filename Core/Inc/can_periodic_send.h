/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_periodic_send.h
  * @brief          : STM32 CAN1周期性发送任务头文件
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件定义了STM32自带CAN1的周期性发送功能接口
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_PERIODIC_SEND_H
#define __CAN_PERIODIC_SEND_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  CAN周期性发送统计信息结构体
  */
typedef struct {
    uint32_t send_count;        // 发送计数
    uint32_t last_send_time;    // 上次发送时间
    uint32_t next_send_time;    // 下次发送时间
    uint8_t initialized;        // 初始化标志
} CAN_PeriodicSend_Stats_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  CAN周期性发送任务初始化
  * @param  None
  * @retval HAL_OK: 成功, HAL_ERROR: 失败
  */
HAL_StatusTypeDef CAN_PeriodicSend_Init(void);

/**
  * @brief  CAN周期性发送任务主函数
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_PeriodicSend_Task(void *argument);

/**
  * @brief  获取周期性发送统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN_PeriodicSend_GetStats(CAN_PeriodicSend_Stats_t *stats);

/**
  * @brief  重置周期性发送统计信息
  * @retval None
  */
void CAN_PeriodicSend_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_PERIODIC_SEND_H */