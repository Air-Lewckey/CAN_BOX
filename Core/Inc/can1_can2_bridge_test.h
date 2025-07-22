/**
  ******************************************************************************
  * @file    can1_can2_bridge_test.h
  * @brief   CAN1和CAN2总线桥接测试模块头文件
  *          验证CAN1为CAN2提供ACK应答的功能
  ******************************************************************************
  * @attention
  *
  * 使用说明：
  * 1. 确保CAN1和CAN2的总线已经物理连接
  * 2. 调用CAN1_CAN2_BridgeTest_Init()初始化桥接测试
  * 3. 创建任务运行CAN1_CAN2_BridgeTest_Task()进行自动测试
  * 4. 在HAL_CAN_RxFifo0MsgPendingCallback中调用处理函数
  * 5. 观察CAN2发送成功率的提升（应该接近100%）
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN1_CAN2_BRIDGE_TEST_H
#define __CAN1_CAN2_BRIDGE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* 导出的函数 ----------------------------------------------------------------*/

/**
  * @brief  CAN1-CAN2桥接测试初始化
  * @note   配置CAN1和CAN2为正常模式，CAN1提供ACK应答
  * @retval HAL_OK: 初始化成功
  *         HAL_ERROR: 初始化失败
  */
HAL_StatusTypeDef CAN1_CAN2_BridgeTest_Init(void);

/**
  * @brief  CAN1-CAN2桥接测试任务
  * @note   自动从CAN2发送消息，CAN1提供ACK应答，适用于FreeRTOS任务
  * @param  argument: 任务参数（未使用）
  * @retval None
  */
void CAN1_CAN2_BridgeTest_Task(void *argument);

/**
  * @brief  从CAN2发送桥接测试消息
  * @note   CAN2发送消息，期望CAN1提供ACK应答
  * @param  id: CAN消息ID（标准帧）
  * @param  data: 数据指针
  * @param  length: 数据长度（0-8字节）
  * @retval HAL_OK: 发送成功
  *         HAL_ERROR: 发送失败
  */
HAL_StatusTypeDef CAN1_CAN2_BridgeTest_SendFromCAN2(uint32_t id, uint8_t *data, uint8_t length);

/**
  * @brief  处理CAN1接收到的桥接测试消息
  * @note   在CAN1接收中断回调中调用此函数处理桥接测试消息
  * @param  rx_header: 接收消息头指针
  * @param  rx_data: 接收数据指针
  * @retval None
  */
void CAN1_CAN2_BridgeTest_ProcessCAN1Reception(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);

/**
  * @brief  打印CAN1-CAN2桥接测试统计信息
  * @note   显示CAN2发送、CAN1接收计数和成功率
  * @retval None
  */
void CAN1_CAN2_BridgeTest_PrintStats(void);

/**
  * @brief  获取桥接测试统计信息
  * @note   获取详细的统计数据用于分析
  * @param  can2_tx: CAN2发送计数指针（可为NULL）
  * @param  can2_success: CAN2成功计数指针（可为NULL）
  * @param  can1_rx: CAN1接收计数指针（可为NULL）
  * @retval None
  */
void CAN1_CAN2_BridgeTest_GetStats(uint32_t *can2_tx, uint32_t *can2_success, uint32_t *can1_rx);

/* 桥接测试配置宏 */
#define CAN1_CAN2_BRIDGE_TEST_ID_BASE     0x300   /**< 桥接测试消息ID基址 */
#define CAN1_CAN2_BRIDGE_TEST_ID_RANGE    10      /**< 测试ID范围 */
#define CAN1_CAN2_BRIDGE_TEST_INTERVAL    2000    /**< 测试消息发送间隔(ms) */
#define CAN1_CAN2_BRIDGE_PATTERN_1        0xBB    /**< 桥接测试数据模式1 */
#define CAN1_CAN2_BRIDGE_PATTERN_2        0xCC    /**< 桥接测试数据模式2 */

/* 桥接测试状态定义 */
typedef enum {
    BRIDGE_TEST_SUCCESS = 0,    /**< 测试成功 */
    BRIDGE_TEST_CAN1_ERROR,     /**< CAN1错误 */
    BRIDGE_TEST_CAN2_ERROR,     /**< CAN2错误 */
    BRIDGE_TEST_CONFIG_ERROR,   /**< 配置错误 */
    BRIDGE_TEST_TIMEOUT_ERROR   /**< 超时错误 */
} BridgeTestStatus_t;

/* 桥接测试统计结构体 */
typedef struct {
    uint32_t can2_tx_total;     /**< CAN2发送总数 */
    uint32_t can2_tx_success;   /**< CAN2发送成功数 */
    uint32_t can2_tx_errors;    /**< CAN2发送错误数 */
    uint32_t can1_rx_total;     /**< CAN1接收总数 */
    float success_rate;         /**< 成功率 */
    float ack_rate;             /**< ACK应答率 */
} BridgeTestStats_t;

#ifdef __cplusplus
}
#endif

#endif /* __CAN1_CAN2_BRIDGE_TEST_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/