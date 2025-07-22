/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_simple_demo.h
  * @brief          : 最简单的CAN1发送Demo头文件
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件定义了最简单的CAN1发送Demo的接口
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_SIMPLE_DEMO_H
#define __CAN_SIMPLE_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  CAN简单Demo统计信息结构体
  */
typedef struct {
    uint32_t heartbeat_count;   // 心跳消息计数
    uint32_t data_count;        // 数据消息计数
    uint32_t status_count;      // 状态消息计数
    uint32_t control_count;     // 控制消息计数
    uint32_t debug_count;       // 调试消息计数
    uint32_t total_count;       // 总消息计数
    uint8_t initialized;        // 初始化状态
} CAN_SimpleDemo_Stats_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  CAN简单Demo初始化
  * @param  None
  * @retval HAL_OK: 成功, HAL_ERROR: 失败
  */
HAL_StatusTypeDef CAN_SimpleDemo_Init(void);

/**
  * @brief  CAN简单Demo主任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_SimpleDemo_Task(void *argument);

/**
  * @brief  获取Demo统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN_SimpleDemo_GetStats(CAN_SimpleDemo_Stats_t *stats);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_SIMPLE_DEMO_H */