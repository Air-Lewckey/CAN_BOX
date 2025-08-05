/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_app.h
  * @brief          : CAN application layer header file
  * @author         : Alientek Team
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * This header file defines interface functions, data structures and constant definitions for CAN application layer
  * Provides complete CAN communication application layer API
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

/* Application layer return value definitions ----------------------------------------------------------*/
#define CAN_APP_OK              0       // Operation successful
#define CAN_APP_ERROR           1       // Operation failed
#define CAN_APP_TIMEOUT         2       // Operation timeout
#define CAN_APP_BUSY            3       // System busy
#define CAN_APP_NOT_INIT        4       // Not initialized

/* CAN application layer message ID definitions -------------------------------------------------------*/
#define CAN_MSG_HEARTBEAT       0x100   // Heartbeat message ID
#define CAN_MSG_DATA            0x200   // Data message ID
#define CAN_MSG_STATUS          0x300   // Status message ID
#define CAN_MSG_COMMAND         0x400   // Command message ID
#define CAN_MSG_RESPONSE        0x500   // Response message ID
#define CAN_MSG_ERROR           0x600   // Error message ID
#define CAN_MSG_DEBUG           0x700   // Debug message ID

/* CAN application layer message type definitions -----------------------------------------------------*/
#define CAN_MSG_TYPE_HEARTBEAT  0x01    // Heartbeat message type
#define CAN_MSG_TYPE_DATA       0x02    // Data message type
#define CAN_MSG_TYPE_STATUS     0x03    // Status message type
#define CAN_MSG_TYPE_COMMAND    0x04    // Command message type
#define CAN_MSG_TYPE_RESPONSE   0x05    // Response message type
#define CAN_MSG_TYPE_ERROR      0x06    // Error message type
#define CAN_MSG_TYPE_DEBUG      0x07    // Debug message type

/* CAN application layer command definitions ---------------------------------------------------------*/
#define CAN_CMD_RESET           0x01    // Reset command
#define CAN_CMD_START           0x02    // Start command
#define CAN_CMD_STOP            0x03    // Stop command
#define CAN_CMD_GET_STATUS      0x04    // Get status command
#define CAN_CMD_SET_PARAM       0x05    // Set parameter command
#define CAN_CMD_GET_PARAM       0x06    // Get parameter command
#define CAN_CMD_SELF_TEST       0x07    // Self test command
#define CAN_CMD_FIRMWARE_VER    0x08    // Get firmware version command

/* CAN application layer status definitions ---------------------------------------------------------*/
#define CAN_STATUS_IDLE         0x00    // Idle status
#define CAN_STATUS_RUNNING      0x01    // Running status
#define CAN_STATUS_ERROR        0x02    // Error status
#define CAN_STATUS_BUSY         0x03    // Busy status
#define CAN_STATUS_INIT         0x04    // Initialization status
#define CAN_STATUS_SLEEP        0x05    // Sleep status

/* Data structure definitions --------------------------------------------------------------*/

/**
  * @brief  CAN queue message structure
  */
typedef struct {
    MCP2515_CANMessage_t message;       // CAN message
    uint32_t timestamp;                 // Timestamp
    uint8_t priority;                   // Priority (0-7, 0 highest)
    uint8_t retry_count;                // Retry count
} CAN_QueueMessage_t;

/**
  * @brief  CAN application statistics structure
  */
typedef struct {
    uint32_t tx_count;                  // Transmit message count
    uint32_t rx_count;                  // Receive message count
    uint32_t error_count;               // Error count
    uint32_t queue_full_count;          // Queue full count
    uint32_t timeout_count;             // Timeout count
    uint8_t initialized;                // Initialization flag
    uint8_t current_status;             // Current status
    uint8_t last_error;                 // Last error code
} CAN_App_Stats_t;

/**
  * @brief  CAN application configuration structure
  */
typedef struct {
    uint8_t baudrate;                   // Baudrate setting
    uint8_t mode;                       // Working mode
    uint8_t auto_retry;                 // Auto retry enable
    uint8_t max_retry;                  // Maximum retry count
    uint32_t timeout_ms;                // Timeout time (ms)
    uint8_t filter_enable;              // Filter enable
    uint32_t filter_id;                 // Filter ID
    uint32_t filter_mask;               // Filter mask
} CAN_App_Config_t;

/**
  * @brief  CAN heartbeat message structure
  */
typedef struct {
    uint8_t header[2];                  // Message header (0xAA, 0x55)
    uint32_t counter;                   // Counter
    uint16_t timestamp;                 // Timestamp
} CAN_HeartbeatMsg_t;

/**
  * @brief  CAN data message structure
  */
typedef struct {
    uint8_t header[2];                  // Message header (0x12, 0x34)
    uint8_t data_type;                  // Data type
    uint8_t data_length;                // Data length
    uint8_t data[4];                    // Data content
} CAN_DataMsg_t;

/**
  * @brief  CAN status message structure
  */
typedef struct {
    uint8_t status;                     // System status
    uint8_t error_code;                 // Error code
    uint16_t voltage;                   // Voltage value (mV)
    uint16_t temperature;               // Temperature value (0.1°C)
    uint16_t reserved;                  // Reserved field
} CAN_StatusMsg_t;

/**
  * @brief  CAN command message structure
  */
typedef struct {
    uint8_t command;                    // Command code
    uint8_t param_count;                // Parameter count
    uint8_t params[6];                  // Parameter data
} CAN_CommandMsg_t;

/**
  * @brief  CAN response message structure
  */
typedef struct {
    uint8_t command;                    // Corresponding command code
    uint8_t result;                     // Execution result
    uint8_t data_length;                // Return data length
    uint8_t data[5];                    // Return data
} CAN_ResponseMsg_t;

/* Function declarations ------------------------------------------------------------------*/

/* Initialization and configuration functions */
uint8_t CAN_App_Init(void);
uint8_t CAN_App_DeInit(void);
uint8_t CAN_App_Config(CAN_App_Config_t *config);
uint8_t CAN_App_GetConfig(CAN_App_Config_t *config);

/* Main task functions */
void CAN_SendTask_Main(void *argument);
void CAN_ReceiveTask_Main(void *argument);

/* Message sending functions */
uint8_t CAN_App_SendMessage(uint32_t id, uint8_t *data, uint8_t length, uint8_t extended);
uint8_t CAN_App_SendRemoteFrame(uint32_t id, uint8_t dlc, uint8_t extended);
uint8_t CAN_App_SendHeartbeat(void);
uint8_t CAN_App_SendStatus(uint8_t status, uint8_t error_code);
uint8_t CAN_App_SendCommand(uint8_t command, uint8_t *params, uint8_t param_count);
uint8_t CAN_App_SendResponse(uint8_t command, uint8_t result, uint8_t *data, uint8_t length);

/* Message receiving functions */
uint8_t CAN_App_ReceiveMessage(MCP2515_CANMessage_t *message, uint32_t timeout);
uint8_t CAN_App_CheckReceive(void);

/* Filter and mask settings */
uint8_t CAN_App_SetFilter(uint32_t filter_id, uint32_t mask, uint8_t extended);
uint8_t CAN_App_SetMultipleFilters(uint32_t *filter_ids, uint32_t *masks, uint8_t count, uint8_t extended);
uint8_t CAN_App_DisableFilter(void);

/* Status and statistics functions */
void CAN_App_GetStats(CAN_App_Stats_t *stats);
void CAN_App_ClearStats(void);
void CAN_App_PrintStatus(void);
uint8_t CAN_App_GetStatus(void);
uint8_t CAN_App_GetLastError(void);

/* Control functions */
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