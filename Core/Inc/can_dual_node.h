/**
  ******************************************************************************
  * @file    can_dual_node.h
  * @brief   STM32F407 + WCMCU-230 双CAN节点通信头文件
  * @author  正点原子技术专家
  * @version V1.0
  * @date    2024-12-19
  ******************************************************************************
  * @attention
  *
  * 本文件实现STM32F407内置CAN控制器与WCMCU-230模块的双节点通信
  * 支持心跳消息、数据请求/响应、状态监控等功能
  *
  ******************************************************************************
  */

#ifndef __CAN_DUAL_NODE_H
#define __CAN_DUAL_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief CAN双节点通信统计结构体
  */
typedef struct {
    uint32_t tx_count;          // 发送消息计数
    uint32_t rx_count;          // 接收消息计数
    uint32_t error_count;       // 错误计数
    uint32_t heartbeat_count;   // 心跳计数
    uint32_t data_req_count;    // 数据请求计数
    uint32_t data_resp_count;   // 数据响应计数
    uint32_t start_time;        // 开始时间
    uint32_t last_rx_time;      // 最后接收时间
} CAN_DualNode_Stats_t;

/**
  * @brief CAN消息类型枚举
  */
typedef enum {
    CAN_MSG_HEARTBEAT = 0,      // 心跳消息
    CAN_MSG_DATA_REQUEST,       // 数据请求
    CAN_MSG_DATA_RESPONSE,      // 数据响应
    CAN_MSG_STATUS,             // 状态消息
    CAN_MSG_CONTROL,            // 控制消息
    CAN_MSG_ERROR,              // 错误消息
    CAN_MSG_ACK,                // ACK应答消息
    CAN_MSG_UNKNOWN             // 未知消息
} CAN_MessageType_t;

/**
  * @brief CAN节点状态枚举
  */
typedef enum {
    CAN_NODE_OFFLINE = 0,       // 离线
    CAN_NODE_ONLINE,            // 在线
    CAN_NODE_ERROR,             // 错误
    CAN_NODE_TIMEOUT            // 超时
} CAN_NodeStatus_t;

/* Exported constants --------------------------------------------------------*/

/* CAN消息ID定义 */
#define CAN_STM32_TO_WCMCU_ID       0x123   // STM32发送给WCMCU-230
#define CAN_WCMCU_TO_STM32_ID       0x456   // WCMCU-230发送给STM32
#define CAN_HEARTBEAT_ID            0x100   // 心跳消息
#define CAN_DATA_REQUEST_ID         0x200   // 数据请求
#define CAN_DATA_RESPONSE_ID        0x300   // 数据响应
#define CAN_STATUS_ID               0x400   // 状态消息
#define CAN_CONTROL_ID              0x500   // 控制消息
#define CAN_ERROR_ID                0x600   // 错误消息
#define CAN_ACK_ID                  0x700   // ACK应答消息

/* 通信周期定义(ms) */
#define CAN_HEARTBEAT_PERIOD        1000    // 心跳周期
#define CAN_DATA_REQUEST_PERIOD     3000    // 数据请求周期
#define CAN_STATUS_PERIOD           2000    // 状态消息周期
#define CAN_TIMEOUT_PERIOD          5000    // 超时判断周期

/* 消息数据长度定义 */
#define CAN_HEARTBEAT_LEN           4       // 心跳消息长度
#define CAN_DATA_REQUEST_LEN        2       // 数据请求长度
#define CAN_DATA_RESPONSE_LEN       8       // 数据响应长度
#define CAN_STATUS_LEN              6       // 状态消息长度
#define CAN_CONTROL_LEN             4       // 控制消息长度
#define CAN_ACK_LEN                 4       // ACK应答消息长度

/* 消息标识符定义 */
#define CAN_HEARTBEAT_MAGIC         0xAA55  // 心跳魔数
#define CAN_DATA_REQ_MAGIC          0x1234  // 数据请求魔数
#define CAN_STATUS_MAGIC            0x5678  // 状态魔数
#define CAN_CONTROL_MAGIC           0x9ABC  // 控制魔数
#define CAN_ACK_MAGIC               0xACE0  // ACK应答魔数

/* Exported macro ------------------------------------------------------------*/

/**
  * @brief 获取当前时间戳
  */
#define CAN_GET_TIMESTAMP()         HAL_GetTick()

/**
  * @brief 检查超时
  */
#define CAN_IS_TIMEOUT(start, period) ((CAN_GET_TIMESTAMP() - (start)) >= (period))

/**
  * @brief 打印调试信息 (临时禁用以避免中文乱码)
  */
#define CAN_DEBUG_PRINTF(fmt, ...)  // printf("[CAN_DUAL] " fmt, ##__VA_ARGS__)

/* Exported functions prototypes ---------------------------------------------*/

/* 初始化和配置函数 */
HAL_StatusTypeDef CAN_DualNode_Init(void);
HAL_StatusTypeDef CAN_DualNode_DeInit(void);
HAL_StatusTypeDef CAN_DualNode_Start(void);
HAL_StatusTypeDef CAN_DualNode_Stop(void);

/* 消息发送函数 */
HAL_StatusTypeDef CAN_SendToWCMCU(uint32_t id, uint8_t* data, uint8_t len);
HAL_StatusTypeDef CAN_SendHeartbeat(void);
HAL_StatusTypeDef CAN_SendDataRequest(uint8_t req_type, uint8_t req_param);
HAL_StatusTypeDef CAN_SendDataResponse(uint8_t* data, uint8_t len);
HAL_StatusTypeDef CAN_SendStatusMessage(void);
HAL_StatusTypeDef CAN_SendControlCommand(uint16_t cmd, uint16_t param);
HAL_StatusTypeDef CAN_SendErrorMessage(uint8_t error_code, uint8_t error_data);
HAL_StatusTypeDef CAN_SendAckMessage(uint32_t original_id, uint8_t ack_code);

/* 消息处理函数 */
void CAN_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data);
CAN_MessageType_t CAN_GetMessageType(uint32_t id);
void CAN_ProcessHeartbeat(uint8_t* data, uint8_t len);
void CAN_ProcessDataRequest(uint8_t* data, uint8_t len);
void CAN_ProcessDataResponse(uint8_t* data, uint8_t len);
void CAN_ProcessStatusMessage(uint8_t* data, uint8_t len);
void CAN_ProcessControlCommand(uint8_t* data, uint8_t len);
void CAN_ProcessErrorMessage(uint8_t* data, uint8_t len);
void CAN_ProcessAckMessage(uint8_t* data, uint8_t len);

/* 状态监控函数 */
CAN_NodeStatus_t CAN_GetWCMCUStatus(void);
void CAN_UpdateNodeStatus(void);
void CAN_CheckTimeout(void);

/* 统计和诊断函数 */
void CAN_ResetStats(void);
CAN_DualNode_Stats_t* CAN_GetStats(void);
void CAN_PrintStats(void);
void CAN_PrintNodeStatus(void);
float CAN_GetSuccessRate(void);
uint32_t CAN_GetBusLoad(void);

/* 任务和周期函数 */
void CAN_DualNodeTask(void const * argument);
void CAN_PeriodicSend(void);
void CAN_PeriodicCheck(void);

/* 中断回调函数 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan);

/* 工具函数 */
void CAN_PrintMessage(const char* prefix, uint32_t id, uint8_t* data, uint8_t len);
uint16_t CAN_CalculateChecksum(uint8_t* data, uint8_t len);
bool CAN_VerifyChecksum(uint8_t* data, uint8_t len, uint16_t checksum);
void CAN_FormatTimestamp(uint32_t timestamp, char* buffer, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_DUAL_NODE_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/