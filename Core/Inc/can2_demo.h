/**
  ******************************************************************************
  * @file    can2_demo.h
  * @brief   CAN2静默监听模块头文件
  *          提供CAN2的初始化、消息接收和统计功能（纯监听模式）
  ******************************************************************************
  * @attention
  *
  * STM32F407ZGT6 CAN2静默监听演示
  * 模式：CAN_MODE_SILENT（只接收，不发送任何信号包括ACK）
  * 波特率：500Kbps
  * 引脚：PB12(CAN2_RX), PB13(CAN2_TX - 未使用)
  *
  ******************************************************************************
  */

#ifndef __CAN2_DEMO_H__
#define __CAN2_DEMO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include <stdint.h>

/* CAN2消息ID定义 */
#define CAN2_HEARTBEAT_ID    0x200  // CAN2心跳消息ID
#define CAN2_DATA_ID         0x201  // CAN2数据消息ID
#define CAN2_STATUS_ID       0x202  // CAN2状态消息ID
#define CAN2_CONTROL_ID      0x203  // CAN2控制消息ID
#define CAN2_DEBUG_ID        0x204  // CAN2调试消息ID
#define CAN2_RESPONSE_ID     0x205  // CAN2响应消息ID
#define CAN2_ERROR_ID        0x206  // CAN2错误消息ID
#define CAN2_TEST_ID         0x207  // CAN2测试消息ID
#define CAN2_CONFIG_ID       0x208  // CAN2配置消息ID

/* CAN2统计信息结构体 */
typedef struct {
    uint32_t heartbeat_count;    // 心跳消息计数
    uint32_t data_count;         // 数据消息计数
    uint32_t status_count;       // 状态消息计数
    uint32_t control_count;      // 控制消息计数
    uint32_t debug_count;        // 调试消息计数
    uint32_t response_count;     // 响应消息计数
    uint32_t error_count;        // 错误消息计数
    uint32_t test_count;         // 测试消息计数
    uint32_t config_count;       // 配置消息计数
    uint32_t total_sent;         // 总发送计数
    uint32_t total_received;     // 总接收计数
    uint32_t send_errors;        // 发送错误计数
    uint32_t receive_errors;     // 接收错误计数
    uint32_t last_tx_time;       // 最后发送时间
    uint32_t last_rx_time;       // 最后接收时间
    uint8_t initialized;         // 初始化状态
} CAN2_Demo_Stats_t;

/* 函数声明 */
HAL_StatusTypeDef CAN2_Demo_Init(void);
void CAN2_Demo_Task(void *argument);
void CAN2_Demo_GetStats(CAN2_Demo_Stats_t *stats);
HAL_StatusTypeDef CAN2_Demo_SendMessage(uint32_t id, uint8_t *data, uint8_t length);
void CAN2_Demo_ProcessReceivedMessage(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);
void CAN2_Demo_RunDiagnostic(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN2_DEMO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/