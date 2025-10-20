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
#include "cmsis_os.h"
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
    
    // Configure CAN filter
    CAN_ConfigFilter();
    
    // Start CAN
    status = HAL_CAN_Start(&hcan1);
    if (status != HAL_OK)
    {
        return status;
    }
    
    // Activate receive interrupt
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK)
    {
        return status;
    }
    
    // Activate transmit complete interrupt
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_TX_MAILBOX_EMPTY);
    if (status != HAL_OK)
    {
        return status;
    }
    
    // Activate error interrupt
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE);
    if (status != HAL_OK)
    {
        return status;
    }
    
    // Initialize statistics
    CAN_ResetStats();
    
    // Set initial state
    wcmcu_status = CAN_NODE_OFFLINE;
    
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
        return status;
    }
    
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
        // Print CAN1 transmit log with new format
        printf("[TX] ID:0x%03X, Data:", (unsigned int)id);
        for (int i = 0; i < len && i < 8; i++) {
            if (i == len - 1) {
                printf("%02X", data[i]);  // 最后一个字节不加空格
            } else {
                printf("%02X ", data[i]); // 其他字节后面加空格
            }
        }
        printf(" [END]\r\n");
        
        CAN_UpdateTxStats();
        last_send_time = CAN_GET_TIMESTAMP();
    }
    else
    {
        CAN_UpdateErrorStats();
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
            // 发送ACK应答 - Send ACK response
            CAN_SendAckMessage(header->StdId, 0x01);
            break;
            
        case CAN_MSG_DATA_REQUEST:
            CAN_ProcessDataRequest(data, header->DLC);
            // 发送ACK应答 - Send ACK response
            CAN_SendAckMessage(header->StdId, 0x02);
            break;
            
        case CAN_MSG_DATA_RESPONSE:
            CAN_ProcessDataResponse(data, header->DLC);
            // 发送ACK应答 - Send ACK response
            CAN_SendAckMessage(header->StdId, 0x03);
            break;
            
        case CAN_MSG_STATUS:
            CAN_ProcessStatusMessage(data, header->DLC);
            // 发送ACK应答 - Send ACK response
            CAN_SendAckMessage(header->StdId, 0x04);
            break;
            
        case CAN_MSG_CONTROL:
            CAN_ProcessControlCommand(data, header->DLC);
            // 发送ACK应答 - Send ACK response
            CAN_SendAckMessage(header->StdId, 0x05);
            break;
            
        case CAN_MSG_ERROR:
            CAN_ProcessErrorMessage(data, header->DLC);
            // 发送ACK应答 - Send ACK response
            CAN_SendAckMessage(header->StdId, 0x06);
            break;
            
        case CAN_MSG_ACK:
            CAN_ProcessAckMessage(data, header->DLC);
            // ACK消息本身不需要再次应答 - ACK messages don't need further ACK
            break;
            
        default:
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
        case CAN_ACK_ID:
            return CAN_MSG_ACK;
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
        
        // 标记变量已使用，避免编译警告
        (void)counter;
        
        if (magic == CAN_HEARTBEAT_MAGIC)
        {
            wcmcu_status = CAN_NODE_ONLINE;
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
        
        // 标记变量已使用，避免编译警告
        (void)req_param;
        
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
                    
                    // 标记变量已使用，避免编译警告
                    (void)node_status;
                    (void)tx_count;
                    (void)rx_count;
                    (void)error_count;
                }
                break;
                
            case 0x02:  // Timestamp response
                if (len >= 5)
                {
                    uint32_t wcmcu_time = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
                    
                    // 标记变量已使用，避免编译警告
                    (void)wcmcu_time;
                }
                break;
                
            default:
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
        
        // 标记变量已使用，避免编译警告
        (void)status;
        (void)counter;
        (void)runtime;
        
        if (magic == CAN_STATUS_MAGIC)
        {
            // Status message processed
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
            // Process control command
            switch(cmd)
            {
                case 0x0001:  // Reset statistics
                    CAN_ResetStats();
                    break;
                    
                case 0x0002:  // Print statistics
                    CAN_PrintStats();
                    break;
                    
                case 0x0003:  // Print status
                    CAN_PrintNodeStatus();
                    break;
                    
                default:
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
        
        // 标记变量已使用，避免编译警告
        (void)error_code;
        (void)error_data;
        
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
    
    // 标记变量已使用，避免编译警告
    (void)current_time;
    
    // Check timeout
    if (wcmcu_status == CAN_NODE_ONLINE)
    {
        if (CAN_IS_TIMEOUT(last_heartbeat_time, CAN_TIMEOUT_PERIOD))
        {
            wcmcu_status = CAN_NODE_TIMEOUT;
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
    
    // 标记变量已使用，避免编译警告
    (void)elapsed;
    (void)success_rate;
}

/**
  * @brief  Print node status
  * @retval None
  */
void CAN_PrintNodeStatus(void)
{
    const char* status_str[] = {"Offline", "Online", "Error", "Timeout"};
    
    // 标记变量已使用，避免编译警告
    (void)status_str;
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
        return;
    }
    
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
    printf("[%s] ID:0x%03X, Data:", prefix, (unsigned int)id);
    for(int i = 0; i < len; i++)
    {
        if (i == len - 1) {
            printf("%02X", data[i]);  // 最后一个字节不加空格
        } else {
            printf("%02X ", data[i]); // 其他字节后面加空格
        }
    }
    printf(" [END]\r\n");
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
            // Print CAN1 received message with new format
            printf("[RX] ID:0x%03X, Data:", (unsigned int)RxHeader.StdId);
            for (int i = 0; i < RxHeader.DLC && i < 8; i++) {
                if (i == RxHeader.DLC - 1 || i == 7) {
                    printf("%02X", RxData[i]);  // 最后一个字节不加空格
                } else {
                    printf("%02X ", RxData[i]); // 其他字节后面加空格
                }
            }
            printf(" [END]\r\n");
            
            // Process dual node communication message
            CAN_ProcessReceivedMessage(&RxHeader, RxData);
            
            // 已移除循环测试模块，保留双节点业务处理
            
            // CAN1桥接测试模块已移除
            
            // 调用CAN测试盒API的接收处理函数
            extern void CAN_TestBox_ProcessRxMessage(CAN_HandleTypeDef *hcan, CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);
            CAN_TestBox_ProcessRxMessage(hcan, &RxHeader, RxData);
        }
    }
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
        
        // 标记变量已使用，避免编译警告
        (void)error_code;
        
        CAN_UpdateErrorStats();
        
        // 调用CAN测试盒API的错误处理函数
        extern void CAN_TestBox_ProcessError(CAN_HandleTypeDef *hcan);
        CAN_TestBox_ProcessError(hcan);
    }
}

/**
  * @brief  发送ACK应答消息
  * @param  original_id: 原始消息ID
  * @param  ack_code: ACK代码
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendAckMessage(uint32_t original_id, uint8_t ack_code)
{
    uint8_t ack_data[CAN_ACK_LEN];
    
    // 构造ACK消息数据 - Construct ACK message data
    ack_data[0] = (CAN_ACK_MAGIC >> 8) & 0xFF;  // 魔数高字节 - Magic number high byte
    ack_data[1] = CAN_ACK_MAGIC & 0xFF;         // 魔数低字节 - Magic number low byte
    ack_data[2] = ack_code;                     // ACK代码 - ACK code
    ack_data[3] = (original_id & 0xFF);         // 原始消息ID低字节 - Original message ID low byte
    
    // 发送ACK消息 - Send ACK message
    HAL_StatusTypeDef status = CAN_SendToWCMCU(CAN_ACK_ID, ack_data, CAN_ACK_LEN);
    
    if (status == HAL_OK)
    {
        printf("[CAN_ACK] Sent ACK for ID=0x%03X, code=0x%02X\r\n", 
               (unsigned int)original_id, ack_code);
    }
    else
    {
        printf("[CAN_ACK] Failed to send ACK for ID=0x%03X\r\n", 
               (unsigned int)original_id);
    }
    
    return status;
}

/**
  * @brief  处理ACK应答消息
  * @param  data: 消息数据
  * @param  len: 数据长度
  * @retval None
  */
void CAN_ProcessAckMessage(uint8_t* data, uint8_t len)
{
    if (len >= CAN_ACK_LEN)
    {
        uint16_t magic = (data[0] << 8) | data[1];
        uint8_t ack_code = data[2];
        uint8_t original_id_low = data[3];
        
        if (magic == CAN_ACK_MAGIC)
        {
            printf("[CAN_ACK] Received ACK: code=0x%02X, original_ID_low=0x%02X\r\n", 
                   ack_code, original_id_low);
            
            // 根据ACK代码处理不同类型的确认 - Process different types of ACK based on code
            switch(ack_code)
            {
                case 0x01:
                    printf("[CAN_ACK] Heartbeat message acknowledged\r\n");
                    break;
                case 0x02:
                    printf("[CAN_ACK] Data request acknowledged\r\n");
                    break;
                case 0x03:
                    printf("[CAN_ACK] Data response acknowledged\r\n");
                    break;
                case 0x04:
                    printf("[CAN_ACK] Status message acknowledged\r\n");
                    break;
                case 0x05:
                    printf("[CAN_ACK] Control command acknowledged\r\n");
                    break;
                case 0x06:
                    printf("[CAN_ACK] Error message acknowledged\r\n");
                    break;
                default:
                    printf("[CAN_ACK] Unknown ACK code: 0x%02X\r\n", ack_code);
                    break;
            }
        }
        else
        {
            printf("[CAN_ACK] ACK message magic number error: 0x%04X\r\n", magic);
        }
    }
    else
    {
        printf("[CAN_ACK] ACK message length error: %d bytes\r\n", len);
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/