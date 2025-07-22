/**
  ******************************************************************************
  * @file    can2_loopback_test.h
  * @brief   CAN2环回测试模块头文件
  *          提供CAN2内部环回测试功能，用于验证CAN控制器基本功能
  ******************************************************************************
  * @attention
  *
  * 使用说明：
  * 1. 调用CAN2_LoopbackTest_Init()初始化环回模式
  * 2. 创建任务运行CAN2_LoopbackTest_Task()进行自动测试
  * 3. 或手动调用CAN2_LoopbackTest_SendMessage()发送测试消息
  * 4. 在HAL_CAN_RxFifo0MsgPendingCallback中调用CAN2_LoopbackTest_ProcessMessage处理接收
  * 5. 测试完成后调用CAN2_LoopbackTest_RestoreNormalMode()恢复正常模式
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN2_LOOPBACK_TEST_H
#define __CAN2_LOOPBACK_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* 导出的函数 ----------------------------------------------------------------*/

/**
  * @brief  CAN2环回测试初始化
  * @note   将CAN2配置为内部环回模式，不需要外部总线连接
  * @retval HAL_OK: 初始化成功
  *         HAL_ERROR: 初始化失败
  */
HAL_StatusTypeDef CAN2_LoopbackTest_Init(void);

/**
  * @brief  CAN2环回测试任务
  * @note   自动发送测试消息并统计结果，适用于FreeRTOS任务
  * @param  argument: 任务参数（未使用）
  * @retval None
  */
void CAN2_LoopbackTest_Task(void *argument);

/**
  * @brief  CAN2环回测试发送消息
  * @note   在环回模式下发送测试消息
  * @param  id: CAN消息ID（标准帧）
  * @param  data: 数据指针
  * @param  length: 数据长度（0-8字节）
  * @retval HAL_OK: 发送成功
  *         HAL_ERROR: 发送失败
  */
HAL_StatusTypeDef CAN2_LoopbackTest_SendMessage(uint32_t id, uint8_t *data, uint8_t length);

/**
  * @brief  处理CAN2环回测试接收消息
  * @note   在CAN接收中断回调中调用此函数处理环回消息
  * @param  rx_header: 接收消息头指针
  * @param  rx_data: 接收数据指针
  * @retval None
  */
void CAN2_LoopbackTest_ProcessMessage(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);

/**
  * @brief  打印CAN2环回测试统计信息
  * @note   显示发送、接收计数和成功率
  * @retval None
  */
void CAN2_LoopbackTest_PrintStats(void);

/**
  * @brief  恢复CAN2正常模式
  * @note   测试完成后调用此函数恢复CAN2为正常工作模式
  * @retval HAL_OK: 恢复成功
  *         HAL_ERROR: 恢复失败
  */
HAL_StatusTypeDef CAN2_LoopbackTest_RestoreNormalMode(void);

/* 环回测试配置宏 */
#define CAN2_LOOPBACK_TEST_ID_BASE    0x100   /**< 环回测试消息ID基址 */
#define CAN2_LOOPBACK_TEST_INTERVAL   1000    /**< 测试消息发送间隔(ms) */
#define CAN2_LOOPBACK_DATA_PATTERN_1  0xAA    /**< 测试数据模式1 */
#define CAN2_LOOPBACK_DATA_PATTERN_2  0x55    /**< 测试数据模式2 */

#ifdef __cplusplus
}
#endif

#endif /* __CAN2_LOOPBACK_TEST_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/