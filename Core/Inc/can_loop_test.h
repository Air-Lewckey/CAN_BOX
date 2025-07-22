/**
  ******************************************************************************
  * @file    can_loop_test.h
  * @brief   双CAN节点循环通信测试头文件
  * @author  正点原子技术专家
  * @version V1.0
  * @date    2024-12-19
  ******************************************************************************
  * @attention
  *
  * 本文件定义双CAN节点循环通信测试的接口和数据结构
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_LOOP_TEST_H
#define __CAN_LOOP_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "mcp2515.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  CAN循环测试统计信息结构体
  */
typedef struct
{
    uint32_t total_loops;           // 总循环次数
    uint32_t successful_loops;      // 成功循环次数
    uint32_t timeout_count;         // 超时次数
    float success_rate;             // 成功率（百分比）
    uint32_t last_send_time;        // 最后发送时间
    uint32_t last_receive_time;     // 最后接收时间
    uint8_t waiting_for_response;   // 等待响应标志
} CAN_LoopTest_Stats_t;

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化CAN循环测试
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_LoopTest_Init(void);

/**
  * @brief  CAN循环测试主任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_LoopTest_Task(void *argument);

/**
  * @brief  处理STM32 CAN接收到的循环消息
  * @param  header: 接收头
  * @param  data: 接收数据
  * @retval None
  */
void CAN_LoopTest_ProcessSTM32Message(CAN_RxHeaderTypeDef* header, uint8_t* data);

/**
  * @brief  处理MCP2515接收到的循环消息
  * @param  message: CAN消息
  * @retval None
  */
void CAN_LoopTest_ProcessMCP2515Message(MCP2515_CANMessage_t* message);

/**
  * @brief  获取循环测试统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN_LoopTest_GetStats(CAN_LoopTest_Stats_t* stats);

/**
  * @brief  重置循环测试统计信息
  * @retval None
  */
void CAN_LoopTest_ResetStats(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_LOOP_TEST_H */