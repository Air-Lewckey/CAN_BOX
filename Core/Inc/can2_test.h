/**
 * @file can2_test.h
 * @brief CAN2增强测试功能头文件
 * @author AI Assistant
 * @date 2024
 */

#ifndef __CAN2_TEST_H
#define __CAN2_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
// CAN2测试模式定义
#define CAN2_TEST_MODE_AUTO     0  // 自动测试模式
#define CAN2_TEST_MODE_MANUAL   1  // 手动测试模式
#define CAN2_TEST_MODE_BURST    2  // 突发测试模式

// CAN2配置类型定义
#define CAN2_CONFIG_BAUDRATE    0x01  // 波特率配置
#define CAN2_CONFIG_FILTER      0x02  // 过滤器配置
#define CAN2_CONFIG_MODE        0x03  // 工作模式配置
#define CAN2_CONFIG_TIMEOUT     0x04  // 超时配置

// CAN2错误类型定义
#define CAN2_ERROR_COMM         0x01  // 通信错误
#define CAN2_ERROR_TIMEOUT      0x02  // 超时错误
#define CAN2_ERROR_OVERRUN      0x03  // 数据溢出错误
#define CAN2_ERROR_FORMAT       0x04  // 格式错误

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief CAN2测试初始化
 * @return HAL状态
 */
HAL_StatusTypeDef CAN2_Test_Init(void);

/**
 * @brief CAN2快速测试 - 发送一组测试消息
 */
void CAN2_Test_QuickTest(void);

/**
 * @brief CAN2突发测试 - 高频率发送消息
 * @param count 发送消息数量
 */
void CAN2_Test_BurstMode(uint16_t count);

/**
 * @brief CAN2配置测试 - 发送配置消息
 * @param config_type 配置类型
 * @param value 配置值
 */
void CAN2_Test_SendConfig(uint8_t config_type, uint32_t value);

/**
 * @brief CAN2错误模拟测试
 */
void CAN2_Test_ErrorSimulation(void);

/**
 * @brief CAN2测试任务（主任务函数）
 */
void CAN2_Test_Task(void);

/**
 * @brief CAN2综合测试任务
 */
void CAN2_Test_ComprehensiveTask(void);

/**
 * @brief 获取CAN2测试统计信息
 */
void CAN2_Test_PrintStats(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN2_TEST_H */