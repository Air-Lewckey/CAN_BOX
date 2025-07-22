/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mcp2515_test_demo.h
  * @brief          : MCP2515接收Demo头文件
  * @author         : Assistant
  * @version        : V2.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本头文件定义了MCP2515接收Demo的接口和数据结构
  * 专门用于接收CAN报文并通过串口打印详细信息
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MCP2515_TEST_DEMO_H
#define __MCP2515_TEST_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  MCP2515接收Demo统计信息结构体
  */
typedef struct {
    uint32_t total_received;     // 总接收计数
    uint32_t receive_errors;     // 接收错误计数
    uint32_t last_rx_time;       // 最后接收时间戳 (ms)
    uint8_t  init_status;        // 初始化状态 (1=已初始化, 0=未初始化)
} MCP2515_TestDemo_Stats_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  初始化MCP2515接收Demo
  * @param  None
  * @retval HAL_OK: 成功, HAL_ERROR: 失败
  */
HAL_StatusTypeDef MCP2515_TestDemo_Init(void);

/**
  * @brief  MCP2515接收Demo主任务
  * @param  argument: 任务参数
  * @retval None
  */
void MCP2515_TestDemo_Task(void *argument);

/**
  * @brief  获取MCP2515接收统计信息
  * @param  None
  * @retval 统计信息结构体指针
  */
MCP2515_TestDemo_Stats_t* MCP2515_TestDemo_GetStats(void);

/**
  * @brief  运行MCP2515接收诊断
  * @param  None
  * @retval None
  */
void MCP2515_TestDemo_RunDiagnostic(void);

/**
  * @brief  运行MCP2515接收问题的全面诊断
  * @param  None
  * @retval None
  */
void MCP2515_RunReceptionDiagnostic(void);

#ifdef __cplusplus
}
#endif

#endif /* __MCP2515_TEST_DEMO_H */