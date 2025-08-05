/**
  ******************************************************************************
  * @file    can_dual_node.c
  * @brief   STM32F407 + WCMCU-230 dual CAN node communication implementation
  * @author  Alientek Technical Expert
  * @version V1.0
  * @date    2024-12-19
  ******************************************************************************
  * @attention
  *
  * This file implements dual node communication between STM32F407 built-in CAN controller and WCMCU-230 module
 * Includes complete message sending, receiving, processing and status monitoring functions
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can_dual_node.h"
#include "can.h"
#include "mcp2515.h"
#include "can_loop_test.h"
#include "can2_demo.h"
#include "can2_loopback_test.h"
#include "can1_can2_bridge_test.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* External variables --------------------------------------------------------*/
// CAN handle external declaration (waiting for IOC to regenerate can.c file)
extern CAN_HandleTypeDef hcan1;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* CAN handle and message structures */
static CAN_TxHeaderTypeDef TxHeader;
static CAN_RxHeaderTypeDef RxHeader;
static uint8_t TxData[8];
static uint8_t RxData[8];
static uint32_t TxMailbox;

/* Statistics */
static CAN_DualNode_Stats_t can_stats = {0};

/* Node status */
static CAN_NodeStatus_t wcmcu_status = CAN_NODE_OFFLINE;
static uint32_t last_heartbeat_time = 0;
static uint32_t last_send_time = 0;

/* Periodic task timestamps */
static uint32_t last_heartbeat_send = 0;
static uint32_t last_data_request = 0;
static uint32_t last_status_send = 0;

/* Message counters */
static uint32_t heartbeat_counter = 0;
static uint32_t data_request_counter = 0;
static uint32_t status_counter = 0;

/* Private function prototypes -----------------------------------------------*/
static void CAN_ConfigFilter(void);
static void CAN_UpdateTxStats(void);
static void CAN_UpdateRxStats(void);
static void CAN_UpdateErrorStats(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  Initialize CAN dual node communication
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_DualNode_Init(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    // CAN_DEBUG_PRINTF("Starting CAN dual node communication initialization...\r\n");
    
    // Configure CAN filter
    CAN_ConfigFilter();
    
    // Start CAN
    status = HAL_CAN_Start(&hcan1);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN start failed: %d\r\n", status);
        return status;
    }
    
    // Activate receive interrupt
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN receive interrupt activation failed: %d\r\n", status);
        return status;
    }
    
    // Activate transmit complete interrupt
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_TX_MAILBOX_EMPTY);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN transmit interrupt activation failed: %d\r\n", status);
        return status;
    }
    
    // Activate error interrupt
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN error interrupt activation failed: %d\r\n", status);
        return status;
    }
    
    // Initialize statistics
    CAN_ResetStats();
    
    // Set initial state
    wcmcu_status = CAN_NODE_OFFLINE;
    
    // CAN_DEBUG_PRINTF("CAN dual node communication initialization completed\r\n");
    // CAN_DEBUG_PRINTF("Supported message types:\r\n");
    // CAN_DEBUG_PRINTF("  - Heartbeat message (ID: 0x%03X)\r\n", CAN_HEARTBEAT_ID);
    // CAN_DEBUG_PRINTF("  - Data request (ID: 0x%03X)\r\n", CAN_DATA_REQUEST_ID);
    // CAN_DEBUG_PRINTF("  - Data response (ID: 0x%03X)\r\n", CAN_DATA_RESPONSE_ID);
    // CAN_DEBUG_PRINTF("  - Status message (ID: 0x%03X)\r\n", CAN_STATUS_ID);
    // CAN_DEBUG_PRINTF("  - Control command (ID: 0x%03X)\r\n", CAN_CONTROL_ID);
    
    return HAL_OK;
}

/**
  * @brief  Deinitialize CAN dual node communication
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_DualNode_DeInit(void)
{
    HAL_StatusTypeDef status;
    
    // Stop CAN
    status = HAL_CAN_Stop(&hcan1);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN stop failed: %d\r\n", status);
        return status;
    }
    
    // Print final statistics
    // CAN_PrintStats();
    
    // CAN_DEBUG_PRINTF("CAN dual node communication stopped\r\n");
    return HAL_OK;
}

/**
  * @brief  Send CAN message to WCMCU-230
  * @param  id: Message ID
  * @param  data: Data pointer
  * @param  len: Data length
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_SendToWCMCU(uint32_t id, uint8_t* data, uint8_t len)
{
    HAL_StatusTypeDef status;
    
    // Check parameters
    if (data == NULL || len > 8)
    {
        // CAN_DEBUG_PRINTF("Send parameter error\r\n");
        return HAL_ERROR;
    }
    
    // Configure transmit header
    TxHeader.StdId = id;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = len;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // Copy data
    memcpy(TxData, data, len);
    
    // Send message
    status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    
    if (status == HAL_OK)
    {
        // Print CAN1 transmit log
        printf("[CAN1-TX] ID:0x%03X, DLC:%d, Data:", (unsigned int)id, len);
        for (int i = 0; i < len && i < 8; i++) {
            printf("%02X ", data[i]);
        }
        printf("\r\n");
        
        CAN_UpdateTxStats();
        last_send_time = CAN_GET_TIMESTAMP();
    }
    else
    {
        CAN_UpdateErrorStats();
        // CAN_DEBUG_PRINTF("CAN message send failed: %d\r\n", status);
    }
    
    return status;
}

/**
  * @brief  Send heartbeat message
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_SendHeartbeat(void)
{
    uint8_t heartbeat_data[CAN_HEARTBEAT_LEN];
    uint32_t timestamp = CAN_GET_TIMESTAMP();
    
    // Construct heartbeat data
    heartbeat_data[0] = (CAN_HEARTBEAT_MAGIC >> 8) & 0xFF;  // Magic number high byte
    heartbeat_data[1] = CAN_HEARTBEAT_MAGIC & 0xFF;         // Magic number low byte
    heartbeat_data[2] = (heartbeat_counter >> 8) & 0xFF;    // Counter high byte
    heartbeat_data[3] = heartbeat_counter & 0xFF;           // Counter low byte
    
    // Add timestamp information to heartbeat data (if extra space available)
    (void)timestamp;  // Mark variable as used to avoid compilation warning
    
    heartbeat_counter++;
    can_stats.heartbeat_count++;
    
    return CAN_SendToWCMCU(CAN_HEARTBEAT_ID, heartbeat_data, CAN_HEARTBEAT_LEN);
}

/**
  * @brief  Send data request
  * @param  req_type: Request type
  * @param  req_param: Request parameter
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_SendDataRequest(uint8_t req_type, uint8_t req_param)
{
    uint8_t request_data[CAN_DATA_REQUEST_LEN];
    
    // Construct request data
    request_data[0] = req_type;   // Request type
    request_data[1] = req_param;  // Request parameter
    
    data_request_counter++;
    can_stats.data_req_count++;
    
    return CAN_SendToWCMCU(CAN_DATA_REQUEST_ID, request_data, CAN_DATA_REQUEST_LEN);
}

/**
  * @brief  Send data response
  * @param  data: Response data
  * @param  len: Data length
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_SendDataResponse(uint8_t* data, uint8_t len)
{
    uint8_t response_data[CAN_DATA_RESPONSE_LEN];
    
    // Check length
    if (len > CAN_DATA_RESPONSE_LEN)
    {
        len = CAN_DATA_RESPONSE_LEN;
    }
    
    // Copy data
    memcpy(response_data, data, len);
    
    // If data is less than 8 bytes, fill with 0
    if (len < CAN_DATA_RESPONSE_LEN)
    {
        memset(&response_data[len], 0, CAN_DATA_RESPONSE_LEN - len);
    }
    
    can_stats.data_resp_count++;
    
    return CAN_SendToWCMCU(CAN_DATA_RESPONSE_ID, response_data, CAN_DATA_RESPONSE_LEN);
}

/**
  * @brief  Send status message
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_SendStatusMessage(void)
{
    uint8_t status_data[CAN_STATUS_LEN];
    uint32_t timestamp = CAN_GET_TIMESTAMP();
    
    // Construct status data
    status_data[0] = (CAN_STATUS_MAGIC >> 8) & 0xFF;        // Magic number high byte
    status_data[1] = CAN_STATUS_MAGIC & 0xFF;               // Magic number low byte
    status_data[2] = (uint8_t)wcmcu_status;                 // Node status
    status_data[3] = (status_counter >> 8) & 0xFF;          // Status counter high byte
    status_data[4] = status_counter & 0xFF;                 // Status counter low byte
    status_data[5] = (timestamp / 1000) & 0xFF;             // Runtime (seconds)
    
    status_counter++;
    
    return CAN_SendToWCMCU(CAN_STATUS_ID, status_data, CAN_STATUS_LEN);
}

/**
  * @brief  Send control command
  * @param  cmd: Control command
  * @param  param: Command parameter
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_SendControlCommand(uint16_t cmd, uint16_t param)
{
    uint8_t control_data[CAN_CONTROL_LEN];
    
    // Construct control data
    control_data[0] = (CAN_CONTROL_MAGIC >> 8) & 0xFF;  // Magic number high byte
    control_data[1] = CAN_CONTROL_MAGIC & 0xFF;         // Magic number low byte
    control_data[2] = (cmd >> 8) & 0xFF;                // Command high byte
    control_data[3] = cmd & 0xFF;                       // Command low byte
    
    return CAN_SendToWCMCU(CAN_CONTROL_ID, control_data, CAN_CONTROL_LEN);
}

/**
  * @brief  Send error message
  * @param  error_code: Error code
  * @param  error_data: Error data
  * @retval HAL status
  */
HAL_StatusTypeDef CAN_SendErrorMessage(uint8_t error_code, uint8_t error_data)
{
    uint8_t error_msg[4];
    uint32_t timestamp = CAN_GET_TIMESTAMP();
    
    // Construct error message
    error_msg[0] = 0xEE;                    // Error identifier
    error_msg[1] = error_code;              // Error code
    error_msg[2] = error_data;              // Error data
    error_msg[3] = (timestamp / 100) & 0xFF; // Timestamp
    
    return CAN_SendToWCMCU(CAN_ERROR_ID, error_msg, 4);
}

/**
  * @brief  Process received CAN message
  * @param  header: Receive header
  * @param  data: Receive data
  * @retval None
  */
void CAN_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data)
{
    CAN_MessageType_t msg_type;
    
    // Update receive statistics
    CAN_UpdateRxStats();
    can_stats.last_rx_time = CAN_GET_TIMESTAMP();
    
    // Get message type
    msg_type = CAN_GetMessageType(header->StdId);
    
    // Process according to message type
    switch(msg_type)
    {
        case CAN_MSG_HEARTBEAT:
            CAN_ProcessHeartbeat(data, header->DLC);
            break;
            
        case CAN_MSG_DATA_REQUEST:
            CAN_ProcessDataRequest(data, header->DLC);
            break;
            
        case CAN_MSG_DATA_RESPONSE:
            CAN_ProcessDataResponse(data, header->DLC);
            break;
            
        case CAN_MSG_STATUS:
            CAN_ProcessStatusMessage(data, header->DLC);
            break;
            
        case CAN_MSG_CONTROL:
            CAN_ProcessControlCommand(data, header->DLC);
            break;
            
        case CAN_MSG_ERROR:
            CAN_ProcessErrorMessage(data, header->DLC);
            break;
            
        default:
            // CAN_DEBUG_PRINTF("Received unknown message type: ID=0x%03X\r\n", (unsigned int)header->StdId);
            break;
    }
    
    // Update node status
    if (msg_type != CAN_MSG_UNKNOWN)
    {
        wcmcu_status = CAN_NODE_ONLINE;
        last_heartbeat_time = CAN_GET_TIMESTAMP();
    }
}

/**
  * @brief  Get message type
  * @param  id: Message ID
  * @retval Message type
  */
CAN_MessageType_t CAN_GetMessageType(uint32_t id)
{
    switch(id)
    {
        case CAN_HEARTBEAT_ID:
            return CAN_MSG_HEARTBEAT;
        case CAN_DATA_REQUEST_ID:
            return CAN_MSG_DATA_REQUEST;
        case CAN_DATA_RESPONSE_ID:
            return CAN_MSG_DATA_RESPONSE;
        case CAN_STATUS_ID:
            return CAN_MSG_STATUS;
        case CAN_CONTROL_ID:
            return CAN_MSG_CONTROL;
        case CAN_ERROR_ID:
            return CAN_MSG_ERROR;
        case CAN_WCMCU_TO_STM32_ID:
            return CAN_MSG_DATA_RESPONSE;  // Data sent by WCMCU is treated as response
        default:
            return CAN_MSG_UNKNOWN;
    }
}

/**
  * @brief  Process heartbeat message
  * @param  data: Message data
  * @param  len: Data length
  * @retval None
  */
void CAN_ProcessHeartbeat(uint8_t* data, uint8_t len)
{
    if (len >= 4)
    {
        uint16_t magic = (data[0] << 8) | data[1];
        uint16_t counter = (data[2] << 8) | data[3];
        
        if (magic == CAN_HEARTBEAT_MAGIC)
        {
            // CAN_DEBUG_PRINTF("Received WCMCU heartbeat: counter=%d\r\n", counter);
            wcmcu_status = CAN_NODE_ONLINE;
        }
        else
        {
            // CAN_DEBUG_PRINTF("Heartbeat message magic number error: 0x%04X\r\n", magic);
        }
    }
}

/**
  * @brief  Process data request
  * @param  data: Message data
  * @param  len: Data length
  * @retval None
  */
void CAN_ProcessDataRequest(uint8_t* data, uint8_t len)
{
    if (len >= 2)
    {
        uint8_t req_type = data[0];
        uint8_t req_param = data[1];
        
        // CAN_DEBUG_PRINTF("Received data request: type=%d, param=%d\r\n", req_type, req_param);
        
        // Send response according to request type
        uint8_t response_data[8];
        switch(req_type)
        {
            case 0x01:  // Request system status
                response_data[0] = 0x01;  // Response type
                response_data[1] = (uint8_t)wcmcu_status;
                response_data[2] = (can_stats.tx_count >> 8) & 0xFF;
                response_data[3] = can_stats.tx_count & 0xFF;
                response_data[4] = (can_stats.rx_count >> 8) & 0xFF;
                response_data[5] = can_stats.rx_count & 0xFF;
                response_data[6] = (can_stats.error_count >> 8) & 0xFF;
                response_data[7] = can_stats.error_count & 0xFF;
                break;
                
            case 0x02:  // Request timestamp
                {
                    uint32_t timestamp = CAN_GET_TIMESTAMP();
                    response_data[0] = 0x02;  // Response type
                    response_data[1] = (timestamp >> 24) & 0xFF;
                    response_data[2] = (timestamp >> 16) & 0xFF;
                    response_data[3] = (timestamp >> 8) & 0xFF;
                    response_data[4] = timestamp & 0xFF;
                    response_data[5] = 0x00;
                    response_data[6] = 0x00;
                    response_data[7] = 0x00;
                }
                break;
                
            default:
                response_data[0] = 0xFF;  // Unknown request
                response_data[1] = req_type;
                memset(&response_data[2], 0, 6);
                break;
        }
        
        CAN_SendDataResponse(response_data, 8);
    }
}

/**
  * @brief  Process data response
  * @param  data: 消息数据
  * @param  len: Data length
  * @retval None
  */
void CAN_ProcessDataResponse(uint8_t* data, uint8_t len)
{
    // CAN_DEBUG_PRINTF("Received WCMCU data response: ");
    // for(int i = 0; i < len; i++)
    // {
    //     printf("%02X ", data[i]);
    // }
    // printf("\r\n");
    
    // Parse response data
    if (len >= 1)
    {
        uint8_t resp_type = data[0];
        switch(resp_type)
        {
            case 0x01:  // System status response
                if (len >= 8)
                {
                    uint8_t node_status = data[1];
                    uint16_t tx_count = (data[2] << 8) | data[3];
                    uint16_t rx_count = (data[4] << 8) | data[5];
                    uint16_t error_count = (data[6] << 8) | data[7];
                    
                    // CAN_DEBUG_PRINTF("WCMCU status: %d, TX:%d, RX:%d, ERR:%d\r\n", 
                    //                node_status, tx_count, rx_count, error_count);
                }
                break;
                
            case 0x02:  // Timestamp response
                if (len >= 5)
                {
                    uint32_t wcmcu_time = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
                    // CAN_DEBUG_PRINTF("WCMCU timestamp: %lu ms\r\n", wcmcu_time);
                }
                break;
                
            default:
                // CAN_DEBUG_PRINTF("Unknown response type: %d\r\n", resp_type);
                break;
        }
    }
}

/**
  * @brief  Process status message
  * @param  data: 消息数据
  * @param  len: Data length
  * @retval None
  */
void CAN_ProcessStatusMessage(uint8_t* data, uint8_t len)
{
    if (len >= 6)
    {
        uint16_t magic = (data[0] << 8) | data[1];
        uint8_t status = data[2];
        uint16_t counter = (data[3] << 8) | data[4];
        uint8_t runtime = data[5];
        
        if (magic == CAN_STATUS_MAGIC)
        {
            // CAN_DEBUG_PRINTF("WCMCU status: %d, counter: %d, runtime: %d seconds\r\n", 
            //                status, counter, runtime);
        }
    }
}

/**
  * @brief  Process control command
  * @param  data: 消息数据
  * @param  len: 数据长度
  * @retval None
  */
void CAN_ProcessControlCommand(uint8_t* data, uint8_t len)
{
    if (len >= 4)
    {
        uint16_t magic = (data[0] << 8) | data[1];
        uint16_t cmd = (data[2] << 8) | data[3];
        
        if (magic == CAN_CONTROL_MAGIC)
        {
            // CAN_DEBUG_PRINTF("Received control command: 0x%04X\r\n", cmd);
            
            // Process control command
            switch(cmd)
            {
                case 0x0001:  // Reset statistics
                    CAN_ResetStats();
                    // CAN_DEBUG_PRINTF("Statistics have been reset\r\n");
                    break;
                    
                case 0x0002:  // Print statistics
                    CAN_PrintStats();
                    break;
                    
                case 0x0003:  // Print status
                    CAN_PrintNodeStatus();
                    break;
                    
                default:
                    // CAN_DEBUG_PRINTF("Unknown control command: 0x%04X\r\n", cmd);
                    break;
            }
        }
    }
}

/**
  * @brief  Process error message
  * @param  data: 消息数据
  * @param  len: 数据长度
  * @retval None
  */
void CAN_ProcessErrorMessage(uint8_t* data, uint8_t len)
{
    if (len >= 3)
    {
        uint8_t error_code = data[1];
        uint8_t error_data = data[2];
        
        // CAN_DEBUG_PRINTF("Received error message: code=0x%02X, data=0x%02X\r\n", 
        //                error_code, error_data);
        
        // Update node status
        wcmcu_status = CAN_NODE_ERROR;
    }
}

/**
  * @brief  Get WCMCU node status
  * @retval Node status
  */
CAN_NodeStatus_t CAN_GetWCMCUStatus(void)
{
    return wcmcu_status;
}

/**
  * @brief  Update node status
  * @retval None
  */
void CAN_UpdateNodeStatus(void)
{
    uint32_t current_time = CAN_GET_TIMESTAMP();
    
    // Check timeout
    if (wcmcu_status == CAN_NODE_ONLINE)
    {
        if (CAN_IS_TIMEOUT(last_heartbeat_time, CAN_TIMEOUT_PERIOD))
        {
            wcmcu_status = CAN_NODE_TIMEOUT;
            // CAN_DEBUG_PRINTF("WCMCU node timeout\r\n");
        }
    }
}

/**
  * @brief  Check timeout
  * @retval None
  */
void CAN_CheckTimeout(void)
{
    CAN_UpdateNodeStatus();
}

/**
  * @brief  Reset statistics
  * @retval None
  */
void CAN_ResetStats(void)
{
    memset(&can_stats, 0, sizeof(can_stats));
    can_stats.start_time = CAN_GET_TIMESTAMP();
    
    heartbeat_counter = 0;
    data_request_counter = 0;
    status_counter = 0;
    
    // CAN_DEBUG_PRINTF("Statistics have been reset\r\n");
}

/**
  * @brief  Get statistics
  * @retval Statistics pointer
  */
CAN_DualNode_Stats_t* CAN_GetStats(void)
{
    return &can_stats;
}

/**
  * @brief  Print statistics
  * @retval None
  */
void CAN_PrintStats(void)
{
    uint32_t elapsed = CAN_GET_TIMESTAMP() - can_stats.start_time;
    float success_rate = CAN_GetSuccessRate();
    
    // printf("\r\n=== CAN Dual Node Communication Statistics ===\r\n");
    // printf("Runtime: %lu ms (%.1f seconds)\r\n", elapsed, elapsed / 1000.0f);
    // printf("Sent messages: %lu\r\n", can_stats.tx_count);
    // printf("Received messages: %lu\r\n", can_stats.rx_count);
    // printf("Error count: %lu\r\n", can_stats.error_count);
    // printf("Heartbeat messages: %lu\r\n", can_stats.heartbeat_count);
    // printf("Data requests: %lu\r\n", can_stats.data_req_count);
    // printf("Data responses: %lu\r\n", can_stats.data_resp_count);
    // printf("Communication success rate: %.2f%%\r\n", success_rate);
    // printf("Last receive time: %lu ms\r\n", can_stats.last_rx_time);
    // printf("========================\r\n\r\n");
}

/**
  * @brief  Print node status
  * @retval None
  */
void CAN_PrintNodeStatus(void)
{
    const char* status_str[] = {"Offline", "Online", "Error", "Timeout"};
    
    // printf("\r\n=== Node Status Information ===\r\n");
    // printf("WCMCU-230 Status: %s\r\n", status_str[wcmcu_status]);
    // printf("Last heartbeat time: %lu ms\r\n", last_heartbeat_time);
    // printf("Last send time: %lu ms\r\n", last_send_time);
    // printf("Current time: %lu ms\r\n", CAN_GET_TIMESTAMP());
    // printf("==================\r\n\r\n");
}

/**
  * @brief  Get communication success rate
  * @retval Success rate (percentage)
  */
float CAN_GetSuccessRate(void)
{
    if (can_stats.tx_count == 0)
    {
        return 0.0f;
    }
    
    return (float)can_stats.rx_count / can_stats.tx_count * 100.0f;
}

/**
  * @brief  Get bus load
  * @retval Bus load (percentage)
  */
uint32_t CAN_GetBusLoad(void)
{
    uint32_t elapsed = CAN_GET_TIMESTAMP() - can_stats.start_time;
    if (elapsed == 0)
    {
        return 0;
    }
    
    // Simple estimation: approximately 1ms transmission time per message
    uint32_t total_msg_time = (can_stats.tx_count + can_stats.rx_count) * 1;
    return (total_msg_time * 100) / elapsed;
}

/**
  * @brief  CAN dual node communication task
  * @param  argument: Task parameter
  * @retval None
  */
void CAN_DualNodeTask(void const * argument)
{
    // Initialize CAN dual node communication
    if (CAN_DualNode_Init() != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN dual node communication initialization failed\r\n");
        return;
    }
    
    // CAN_DEBUG_PRINTF("CAN dual node communication task started\r\n");
    
    for(;;)
    {
        // Periodic send
        CAN_PeriodicSend();
        
        // Periodic check
        CAN_PeriodicCheck();
        
        // Task delay
        osDelay(100);  // 100ms period
    }
}

/**
  * @brief  Periodic send
  * @retval None
  */
void CAN_PeriodicSend(void)
{
    uint32_t current_time = CAN_GET_TIMESTAMP();
    
    // Send heartbeat message
    if (CAN_IS_TIMEOUT(last_heartbeat_send, CAN_HEARTBEAT_PERIOD))
    {
        CAN_SendHeartbeat();
        last_heartbeat_send = current_time;
    }
    
    // Send data request
    if (CAN_IS_TIMEOUT(last_data_request, CAN_DATA_REQUEST_PERIOD))
    {
        static uint8_t req_type = 1;
        CAN_SendDataRequest(req_type, 0x00);
        req_type = (req_type == 1) ? 2 : 1;  // Alternate request different types
        last_data_request = current_time;
    }
    
    // Send status message
    if (CAN_IS_TIMEOUT(last_status_send, CAN_STATUS_PERIOD))
    {
        CAN_SendStatusMessage();
        last_status_send = current_time;
    }
}

/**
  * @brief  Periodic check
  * @retval None
  */
void CAN_PeriodicCheck(void)
{
    static uint32_t last_stats_print = 0;
    uint32_t current_time = CAN_GET_TIMESTAMP();
    
    // Check timeout
    CAN_CheckTimeout();
    
    // Print statistics periodically (every 30 seconds)
    if (CAN_IS_TIMEOUT(last_stats_print, 30000))
    {
        CAN_PrintStats();
        CAN_PrintNodeStatus();
        last_stats_print = current_time;
    }
}

/**
  * @brief  Print message
  * @param  prefix: Prefix string
  * @param  id: Message ID
  * @param  data: Data pointer
  * @param  len: 数据长度
  * @retval None
  */
void CAN_PrintMessage(const char* prefix, uint32_t id, uint8_t* data, uint8_t len)
{
    printf("[%s] ID:0x%03X Len:%d Data:", prefix, (unsigned int)id, len);
    for(int i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
}

/**
  * @brief  Calculate checksum
  * @param  data: Data pointer
  * @param  len: Data length
  * @retval Checksum
  */
uint16_t CAN_CalculateChecksum(uint8_t* data, uint8_t len)
{
    uint16_t checksum = 0;
    for(int i = 0; i < len; i++)
    {
        checksum += data[i];
    }
    return checksum;
}

/**
  * @brief  Verify checksum
  * @param  data: Data pointer
  * @param  len: Data length
  * @param  checksum: Expected checksum
  * @retval Verification result
  */
bool CAN_VerifyChecksum(uint8_t* data, uint8_t len, uint16_t checksum)
{
    return (CAN_CalculateChecksum(data, len) == checksum);
}

/**
  * @brief  Format timestamp
  * @param  timestamp: Timestamp
  * @param  buffer: Output buffer
  * @param  size: Buffer size
  * @retval None
  */
void CAN_FormatTimestamp(uint32_t timestamp, char* buffer, size_t size)
{
    uint32_t seconds = timestamp / 1000;
    uint32_t milliseconds = timestamp % 1000;
    uint32_t minutes = seconds / 60;
    seconds = seconds % 60;
    uint32_t hours = minutes / 60;
    minutes = minutes % 60;
    
    snprintf(buffer, size, "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, milliseconds);
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Configure CAN filter
  * @retval None
  */
static void CAN_ConfigFilter(void)
{
    CAN_FilterTypeDef sFilterConfig;
    
    // Configure filter to accept all messages
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;
    
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief  Update transmit statistics
  * @retval None
  */
static void CAN_UpdateTxStats(void)
{
    can_stats.tx_count++;
}

/**
  * @brief  Update receive statistics
  * @retval None
  */
static void CAN_UpdateRxStats(void)
{
    can_stats.rx_count++;
}

/**
  * @brief  Update error statistics
  * @retval None
  */
static void CAN_UpdateErrorStats(void)
{
    can_stats.error_count++;
}

/* Interrupt Callbacks -------------------------------------------------------*/

/**
  * @brief  CAN receive FIFO0 message pending callback
  * @param  hcan: CAN handle pointer
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            // Print CAN1 received message
            printf("[CAN1-RX] ID:0x%03X, DLC:%d, Data:", (unsigned int)RxHeader.StdId, RxHeader.DLC);
            for (int i = 0; i < RxHeader.DLC && i < 8; i++) {
                if (i == RxHeader.DLC - 1 || i == 7) {
                    printf("%02X", RxData[i]);  // 最后一个字节不加空格
                } else {
                    printf("%02X ", RxData[i]); // 其他字节后面加空格
                }
            }
            printf("\r\n");
            
            // Process dual node communication message
            CAN_ProcessReceivedMessage(&RxHeader, RxData);
            
            // Process loop test message
            CAN_LoopTest_ProcessSTM32Message(&RxHeader, RxData);
            
            // CAN1 bridge test message processing is disabled
            // CAN1_CAN2_BridgeTest_ProcessCAN1Reception(&RxHeader, RxData);
        }
    }
    // CAN2相关处理已禁用 - 只使用CAN1
    // else if (hcan->Instance == CAN2)
    // {
    //     printf("[CAN2-IRQ] Interrupt triggered at %lu\r\n", HAL_GetTick());
    //     if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
    //     {
    //         printf("[CAN2-IRQ] Message received successfully\r\n");
    //         // CAN2 only acts as receiver, prints all messages on CAN bus
    //         CAN2_Demo_ProcessReceivedMessage(&RxHeader, RxData);
    //         
    //         // Loopback test is disabled - CAN2 only receives
    //         // CAN2_LoopbackTest_ProcessMessage(&RxHeader, RxData);
    //     }
    //     else
    //     {
    //         printf("[CAN2-IRQ] Failed to get message from FIFO0\r\n");
    //     }
    // }
}

/**
  * @brief  CAN transmit mailbox 0 complete callback
  * @param  hcan: CAN handle pointer
  * @retval None
  */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        // Transmission complete handling
    }
}

/**
  * @brief  CAN transmit mailbox 1 complete callback
  * @param  hcan: CAN handle pointer
  * @retval None
  */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        // Transmission complete handling
    }
}

/**
  * @brief  CAN transmit mailbox 2 complete callback
  * @param  hcan: CAN句柄指针
  * @retval None
  */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        // Transmission complete handling
    }
}

/**
  * @brief  CAN error callback
  * @param  hcan: CAN句柄指针
  * @retval None
  */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        uint32_t error_code = HAL_CAN_GetError(hcan);
        // CAN_DEBUG_PRINTF("CAN Error: 0x%08lX\r\n", error_code);
        CAN_UpdateErrorStats();
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/