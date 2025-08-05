/**
  ******************************************************************************
  * @file           : can_trigger_send.h
  * @brief          : 触发式CAN发送模块头文件
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件定义了触发式CAN发送模块的接口函数
  *
  ******************************************************************************
  */

#ifndef __CAN_TRIGGER_SEND_H
#define __CAN_TRIGGER_SEND_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
HAL_StatusTypeDef CAN_TriggerSend_Init(void);
void CAN_TriggerSend_Task(void *argument);
HAL_StatusTypeDef CAN_TriggerSend_SendMessage1(void);
HAL_StatusTypeDef CAN_TriggerSend_SendMessage2(void);
HAL_StatusTypeDef CAN_TriggerSend_SendMessage3(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_TRIGGER_SEND_H */