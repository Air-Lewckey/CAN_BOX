/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_app.h
  * @brief          : CAN应用层头文件
  * @author         : 正点原子团队
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本头文件定义了CAN应用层的接口函数、数据结构和常量定义
  * 提供了完整的CAN通信应用层API
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __CAN_APP_H
#define __CAN_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "mcp2515.h"
#include "cmsis_os.h"

/* 应用层返回值定义 ----------------------------------------------------------*/
#define CAN_APP_OK              0       // 操作成功
#define CAN_APP_ERROR           1       // 操作失败
#define CAN_APP_TIMEOUT         2       // 操作超时
#define CAN_APP_BUSY            3       // 系统忙
#define CAN_APP_NOT_INIT        4       // 未初始化

/* CAN应用层消息ID定义 -------------------------------------------------------*/
#define CAN_MSG_HEARTBEAT       0x100   // 心跳消息ID
#define CAN_MSG_DATA            0x200   // 数据消息ID
#define CAN_MSG_STATUS          0x300   // 状态消息ID
#define CAN_MSG_COMMAND         0x400   // 命令消息ID
#define CAN_MSG_RESPONSE        0x500   // 响应消息ID
#define CAN_MSG_ERROR           0x600   // 错误消息ID
#define CAN_MSG_DEBUG           0x700   // 调试消息ID

/* CAN应用层消息类型定义 -----------------------------------------------------*/
#define CAN_MSG_TYPE_HEARTBEAT  0x01    // 心跳消息类型
#define CAN_MSG_TYPE_DATA       0x02    // 数据消息类型
#define CAN_MSG_TYPE_STATUS     0x03    // 状态消息类型
#define CAN_MSG_TYPE_COMMAND    0x04    // 命令消息类型
#define CAN_MSG_TYPE_RESPONSE   0x05    // 响应消息类型
#define CAN_MSG_TYPE_ERROR      0x06    // 错误消息类型
#define CAN_MSG_TYPE_DEBUG      0x07    // 调试消息类型

/* CAN应用层命令定义 ---------------------------------------------------------*/
#define CAN_CMD_RESET           0x01    // 复位命令
#define CAN_CMD_START           0x02    // 启动命令
#define CAN_CMD_STOP            0x03    // 停止命令
#define CAN_CMD_GET_STATUS      0x04    // 获取状态命令
#define CAN_CMD_SET_PARAM       0x05    // 设置参数命令
#define CAN_CMD_GET_PARAM       0x06    // 获取参数命令
#define CAN_CMD_SELF_TEST       0x07    // 自检命令
#define CAN_CMD_FIRMWARE_VER    0x08    // 获取固件版本命令

/* CAN应用层状态定义 ---------------------------------------------------------*/
#define CAN_STATUS_IDLE         0x00    // 空闲状态
#define CAN_STATUS_RUNNING      0x01    // 运行状态
#define CAN_STATUS_ERROR        0x02    // 错误状态
#define CAN_STATUS_BUSY         0x03    // 忙状态
#define CAN_STATUS_INIT         0x04    // 初始化状态
#define CAN_STATUS_SLEEP        0x05    // 睡眠状态

/* 数据结构定义 --------------------------------------------------------------*/

/**
  * @brief  CAN队列消息结构体
  */
typedef struct {
    MCP2515_CANMessage_t message;       // CAN消息
    uint32_t timestamp;                 // 时间戳
    uint8_t priority;                   // 优先级 (0-7, 0最高)
    uint8_t retry_count;                // 重试次数
} CAN_QueueMessage_t;

/**
  * @brief  CAN应用统计信息结构体
  */
typedef struct {
    uint32_t tx_count;                  // 发送消息计数
    uint32_t rx_count;                  // 接收消息计数
    uint32_t error_count;               // 错误计数
    uint32_t queue_full_count;          // 队列满计数
    uint32_t timeout_count;             // 超时计数
    uint8_t initialized;                // 初始化标志
    uint8_t current_status;             // 当前状态
    uint8_t last_error;                 // 最后错误代码
} CAN_App_Stats_t;

/**
  * @brief  CAN应用配置结构体
  */
typedef struct {
    uint8_t baudrate;                   // 波特率设置
    uint8_t mode;                       // 工作模式
    uint8_t auto_retry;                 // 自动重试使能
    uint8_t max_retry;                  // 最大重试次数
    uint32_t timeout_ms;                // 超时时间(ms)
    uint8_t filter_enable;              // 过滤器使能
    uint32_t filter_id;                 // 过滤器ID
    uint32_t filter_mask;               // 过滤器掩码
} CAN_App_Config_t;

/**
  * @brief  CAN心跳消息结构体
  */
typedef struct {
    uint8_t header[2];                  // 消息头 (0xAA, 0x55)
    uint32_t counter;                   // 计数器
    uint16_t timestamp;                 // 时间戳
} CAN_HeartbeatMsg_t;

/**
  * @brief  CAN数据消息结构体
  */
typedef struct {
    uint8_t header[2];                  // 消息头 (0x12, 0x34)
    uint8_t data_type;                  // 数据类型
    uint8_t data_length;                // 数据长度
    uint8_t data[4];                    // 数据内容
} CAN_DataMsg_t;

/**
  * @brief  CAN状态消息结构体
  */
typedef struct {
    uint8_t status;                     // 系统状态
    uint8_t error_code;                 // 错误代码
    uint16_t voltage;                   // 电压值 (mV)
    uint16_t temperature;               // 温度值 (0.1°C)
    uint16_t reserved;                  // 保留字段
} CAN_StatusMsg_t;

/**
  * @brief  CAN命令消息结构体
  */
typedef struct {
    uint8_t command;                    // 命令代码
    uint8_t param_count;                // 参数个数
    uint8_t params[6];                  // 参数数据
} CAN_CommandMsg_t;

/**
  * @brief  CAN响应消息结构体
  */
typedef struct {
    uint8_t command;                    // 对应的命令代码
    uint8_t result;                     // 执行结果
    uint8_t data_length;                // 返回数据长度
    uint8_t data[5];                    // 返回数据
} CAN_ResponseMsg_t;

/* 函数声明 ------------------------------------------------------------------*/

/* 初始化和配置函数 */
uint8_t CAN_App_Init(void);
uint8_t CAN_App_DeInit(void);
uint8_t CAN_App_Config(CAN_App_Config_t *config);
uint8_t CAN_App_GetConfig(CAN_App_Config_t *config);

/* 任务主函数 */
void CAN_SendTask_Main(void *argument);
void CAN_ReceiveTask_Main(void *argument);

/* 消息发送函数 */
uint8_t CAN_App_SendMessage(uint32_t id, uint8_t *data, uint8_t length, uint8_t extended);
uint8_t CAN_App_SendRemoteFrame(uint32_t id, uint8_t dlc, uint8_t extended);
uint8_t CAN_App_SendHeartbeat(void);
uint8_t CAN_App_SendStatus(uint8_t status, uint8_t error_code);
uint8_t CAN_App_SendCommand(uint8_t command, uint8_t *params, uint8_t param_count);
uint8_t CAN_App_SendResponse(uint8_t command, uint8_t result, uint8_t *data, uint8_t length);

/* 消息接收函数 */
uint8_t CAN_App_ReceiveMessage(MCP2515_CANMessage_t *message, uint32_t timeout);
uint8_t CAN_App_CheckReceive(void);

/* 过滤器和掩码设置 */
uint8_t CAN_App_SetFilter(uint32_t filter_id, uint32_t mask, uint8_t extended);
uint8_t CAN_App_SetMultipleFilters(uint32_t *filter_ids, uint32_t *masks, uint8_t count, uint8_t extended);
uint8_t CAN_App_DisableFilter(void);

/* 状态和统计函数 */
void CAN_App_GetStats(CAN_App_Stats_t *stats);
void CAN_App_ClearStats(void);
void CAN_App_PrintStatus(void);
uint8_t CAN_App_GetStatus(void);
uint8_t CAN_App_GetLastError(void);

/* 控制函数 */
uint8_t CAN_App_Start(void);
uint8_t CAN_App_Stop(void);
uint8_t CAN_App_Reset(void);
uint8_t CAN_App_Sleep(void);
uint8_t CAN_App_Wakeup(void);

/* 测试和调试函数 */
uint8_t CAN_App_SelfTest(void);
uint8_t CAN_App_LoopbackTest(void);
uint8_t CAN_App_SendTestMessage(uint32_t id, uint8_t *test_data, uint8_t length);
void CAN_App_PrintMessage(const char *prefix, MCP2515_CANMessage_t *message);
void CAN_App_EnableDebug(uint8_t enable);

/* 中断回调函数 */
void CAN_App_IRQ_Callback(void);
void CAN_EXTI_Callback(uint16_t GPIO_Pin);
void CAN_App_RxCallback(MCP2515_CANMessage_t *message);
void CAN_App_TxCallback(uint32_t message_id);
void CAN_App_ErrorCallback(uint8_t error_code);

/* 工具函数 */
uint32_t CAN_App_GetTimestamp(void);
void CAN_App_Delay(uint32_t ms);
uint8_t CAN_App_ValidateMessage(MCP2515_CANMessage_t *message);
uint8_t CAN_App_CalculateChecksum(uint8_t *data, uint8_t length);

/* 消息构造函数 */
void CAN_App_BuildHeartbeatMsg(CAN_HeartbeatMsg_t *msg, uint32_t counter);
void CAN_App_BuildDataMsg(CAN_DataMsg_t *msg, uint8_t type, uint8_t *data, uint8_t length);
void CAN_App_BuildStatusMsg(CAN_StatusMsg_t *msg, uint8_t status, uint8_t error, uint16_t voltage, uint16_t temp);
void CAN_App_BuildCommandMsg(CAN_CommandMsg_t *msg, uint8_t command, uint8_t *params, uint8_t count);
void CAN_App_BuildResponseMsg(CAN_ResponseMsg_t *msg, uint8_t command, uint8_t result, uint8_t *data, uint8_t length);

/* 消息解析函数 */
uint8_t CAN_App_ParseHeartbeatMsg(MCP2515_CANMessage_t *can_msg, CAN_HeartbeatMsg_t *heartbeat);
uint8_t CAN_App_ParseDataMsg(MCP2515_CANMessage_t *can_msg, CAN_DataMsg_t *data_msg);
uint8_t CAN_App_ParseStatusMsg(MCP2515_CANMessage_t *can_msg, CAN_StatusMsg_t *status_msg);
uint8_t CAN_App_ParseCommandMsg(MCP2515_CANMessage_t *can_msg, CAN_CommandMsg_t *command_msg);
uint8_t CAN_App_ParseResponseMsg(MCP2515_CANMessage_t *can_msg, CAN_ResponseMsg_t *response_msg);

/* 队列管理函数 */
uint8_t CAN_App_QueuePut(CAN_QueueMessage_t *queue_msg, uint32_t timeout);
uint8_t CAN_App_QueueGet(CAN_QueueMessage_t *queue_msg, uint32_t timeout);
uint8_t CAN_App_QueueGetCount(void);
uint8_t CAN_App_QueueGetSpace(void);
void CAN_App_QueueFlush(void);

/* 错误处理函数 */
void CAN_App_HandleError(uint8_t error_code);
const char* CAN_App_GetErrorString(uint8_t error_code);
void CAN_App_LogError(uint8_t error_code, const char *description);

/* 版本信息 */
#define CAN_APP_VERSION_MAJOR   1
#define CAN_APP_VERSION_MINOR   0
#define CAN_APP_VERSION_PATCH   0

const char* CAN_App_GetVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_APP_H */