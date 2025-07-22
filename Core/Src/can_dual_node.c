/**
  ******************************************************************************
  * @file    can_dual_node.c
  * @brief   STM32F407 + WCMCU-230 双CAN节点通信实现
  * @author  正点原子技术专家
  * @version V1.0
  * @date    2024-12-19
  ******************************************************************************
  * @attention
  *
  * 本文件实现STM32F407内置CAN控制器与WCMCU-230模块的双节点通信
  * 包含完整的消息发送、接收、处理和状态监控功能
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
// CAN句柄外部声明（等待IOC重新生成can.c文件）
extern CAN_HandleTypeDef hcan1;

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* CAN句柄和消息结构体 */
static CAN_TxHeaderTypeDef TxHeader;
static CAN_RxHeaderTypeDef RxHeader;
static uint8_t TxData[8];
static uint8_t RxData[8];
static uint32_t TxMailbox;

/* 统计信息 */
static CAN_DualNode_Stats_t can_stats = {0};

/* 节点状态 */
static CAN_NodeStatus_t wcmcu_status = CAN_NODE_OFFLINE;
static uint32_t last_heartbeat_time = 0;
static uint32_t last_send_time = 0;

/* 周期性任务时间戳 */
static uint32_t last_heartbeat_send = 0;
static uint32_t last_data_request = 0;
static uint32_t last_status_send = 0;

/* 消息计数器 */
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
  * @brief  初始化CAN双节点通信
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_DualNode_Init(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    
    // CAN_DEBUG_PRINTF("开始初始化CAN双节点通信...\r\n");
    
    // 配置CAN过滤器
    CAN_ConfigFilter();
    
    // 启动CAN
    status = HAL_CAN_Start(&hcan1);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN启动失败: %d\r\n", status);
        return status;
    }
    
    // 激活接收中断
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN接收中断激活失败: %d\r\n", status);
        return status;
    }
    
    // 激活发送完成中断
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_TX_MAILBOX_EMPTY);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN发送中断激活失败: %d\r\n", status);
        return status;
    }
    
    // 激活错误中断
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_ERROR | CAN_IT_BUSOFF | CAN_IT_LAST_ERROR_CODE);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN错误中断激活失败: %d\r\n", status);
        return status;
    }
    
    // 初始化统计信息
    CAN_ResetStats();
    
    // 设置初始状态
    wcmcu_status = CAN_NODE_OFFLINE;
    
    // CAN_DEBUG_PRINTF("CAN双节点通信初始化完成\r\n");
    // CAN_DEBUG_PRINTF("支持的消息类型:\r\n");
    // CAN_DEBUG_PRINTF("  - 心跳消息 (ID: 0x%03X)\r\n", CAN_HEARTBEAT_ID);
    // CAN_DEBUG_PRINTF("  - 数据请求 (ID: 0x%03X)\r\n", CAN_DATA_REQUEST_ID);
    // CAN_DEBUG_PRINTF("  - 数据响应 (ID: 0x%03X)\r\n", CAN_DATA_RESPONSE_ID);
    // CAN_DEBUG_PRINTF("  - 状态消息 (ID: 0x%03X)\r\n", CAN_STATUS_ID);
    // CAN_DEBUG_PRINTF("  - 控制指令 (ID: 0x%03X)\r\n", CAN_CONTROL_ID);
    
    return HAL_OK;
}

/**
  * @brief  反初始化CAN双节点通信
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_DualNode_DeInit(void)
{
    HAL_StatusTypeDef status;
    
    // 停止CAN
    status = HAL_CAN_Stop(&hcan1);
    if (status != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN停止失败: %d\r\n", status);
        return status;
    }
    
    // 打印最终统计信息
    // CAN_PrintStats();
    
    // CAN_DEBUG_PRINTF("CAN双节点通信已停止\r\n");
    return HAL_OK;
}

/**
  * @brief  向WCMCU-230发送CAN消息
  * @param  id: 消息ID
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendToWCMCU(uint32_t id, uint8_t* data, uint8_t len)
{
    HAL_StatusTypeDef status;
    
    // 检查参数
    if (data == NULL || len > 8)
    {
        // CAN_DEBUG_PRINTF("发送参数错误\r\n");
        return HAL_ERROR;
    }
    
    // 配置发送头
    TxHeader.StdId = id;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = len;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 复制数据
    memcpy(TxData, data, len);
    
    // 发送消息
    status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    
    if (status == HAL_OK)
    {
        // 打印CAN1发送日志
        // printf("[CAN1-TX] ID:0x%03X, DLC:%d, Data:", (unsigned int)id, len);
        // for (int i = 0; i < len && i < 8; i++) {
        //     printf("%02X ", data[i]);
        // }
        // printf("\r\n");
        
        CAN_UpdateTxStats();
        last_send_time = CAN_GET_TIMESTAMP();
    }
    else
    {
        CAN_UpdateErrorStats();
        // CAN_DEBUG_PRINTF("CAN消息发送失败: %d\r\n", status);
    }
    
    return status;
}

/**
  * @brief  发送心跳消息
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendHeartbeat(void)
{
    uint8_t heartbeat_data[CAN_HEARTBEAT_LEN];
    uint32_t timestamp = CAN_GET_TIMESTAMP();
    
    // 构造心跳数据
    heartbeat_data[0] = (CAN_HEARTBEAT_MAGIC >> 8) & 0xFF;  // 魔数高字节
    heartbeat_data[1] = CAN_HEARTBEAT_MAGIC & 0xFF;         // 魔数低字节
    heartbeat_data[2] = (heartbeat_counter >> 8) & 0xFF;    // 计数器高字节
    heartbeat_data[3] = heartbeat_counter & 0xFF;           // 计数器低字节
    
    // 添加时间戳信息到心跳数据中（如果有额外空间）
    (void)timestamp;  // 标记变量已使用，避免编译警告
    
    heartbeat_counter++;
    can_stats.heartbeat_count++;
    
    return CAN_SendToWCMCU(CAN_HEARTBEAT_ID, heartbeat_data, CAN_HEARTBEAT_LEN);
}

/**
  * @brief  发送数据请求
  * @param  req_type: 请求类型
  * @param  req_param: 请求参数
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendDataRequest(uint8_t req_type, uint8_t req_param)
{
    uint8_t request_data[CAN_DATA_REQUEST_LEN];
    
    // 构造请求数据
    request_data[0] = req_type;   // 请求类型
    request_data[1] = req_param;  // 请求参数
    
    data_request_counter++;
    can_stats.data_req_count++;
    
    return CAN_SendToWCMCU(CAN_DATA_REQUEST_ID, request_data, CAN_DATA_REQUEST_LEN);
}

/**
  * @brief  发送数据响应
  * @param  data: 响应数据
  * @param  len: 数据长度
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendDataResponse(uint8_t* data, uint8_t len)
{
    uint8_t response_data[CAN_DATA_RESPONSE_LEN];
    
    // 检查长度
    if (len > CAN_DATA_RESPONSE_LEN)
    {
        len = CAN_DATA_RESPONSE_LEN;
    }
    
    // 复制数据
    memcpy(response_data, data, len);
    
    // 如果数据不足8字节，填充0
    if (len < CAN_DATA_RESPONSE_LEN)
    {
        memset(&response_data[len], 0, CAN_DATA_RESPONSE_LEN - len);
    }
    
    can_stats.data_resp_count++;
    
    return CAN_SendToWCMCU(CAN_DATA_RESPONSE_ID, response_data, CAN_DATA_RESPONSE_LEN);
}

/**
  * @brief  发送状态消息
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendStatusMessage(void)
{
    uint8_t status_data[CAN_STATUS_LEN];
    uint32_t timestamp = CAN_GET_TIMESTAMP();
    
    // 构造状态数据
    status_data[0] = (CAN_STATUS_MAGIC >> 8) & 0xFF;        // 魔数高字节
    status_data[1] = CAN_STATUS_MAGIC & 0xFF;               // 魔数低字节
    status_data[2] = (uint8_t)wcmcu_status;                 // 节点状态
    status_data[3] = (status_counter >> 8) & 0xFF;          // 状态计数器高字节
    status_data[4] = status_counter & 0xFF;                 // 状态计数器低字节
    status_data[5] = (timestamp / 1000) & 0xFF;             // 运行时间(秒)
    
    status_counter++;
    
    return CAN_SendToWCMCU(CAN_STATUS_ID, status_data, CAN_STATUS_LEN);
}

/**
  * @brief  发送控制指令
  * @param  cmd: 控制命令
  * @param  param: 命令参数
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendControlCommand(uint16_t cmd, uint16_t param)
{
    uint8_t control_data[CAN_CONTROL_LEN];
    
    // 构造控制数据
    control_data[0] = (CAN_CONTROL_MAGIC >> 8) & 0xFF;  // 魔数高字节
    control_data[1] = CAN_CONTROL_MAGIC & 0xFF;         // 魔数低字节
    control_data[2] = (cmd >> 8) & 0xFF;                // 命令高字节
    control_data[3] = cmd & 0xFF;                       // 命令低字节
    
    return CAN_SendToWCMCU(CAN_CONTROL_ID, control_data, CAN_CONTROL_LEN);
}

/**
  * @brief  发送错误消息
  * @param  error_code: 错误代码
  * @param  error_data: 错误数据
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_SendErrorMessage(uint8_t error_code, uint8_t error_data)
{
    uint8_t error_msg[4];
    uint32_t timestamp = CAN_GET_TIMESTAMP();
    
    // 构造错误消息
    error_msg[0] = 0xEE;                    // 错误标识
    error_msg[1] = error_code;              // 错误代码
    error_msg[2] = error_data;              // 错误数据
    error_msg[3] = (timestamp / 100) & 0xFF; // 时间戳
    
    return CAN_SendToWCMCU(CAN_ERROR_ID, error_msg, 4);
}

/**
  * @brief  处理接收到的CAN消息
  * @param  header: 接收头
  * @param  data: 接收数据
  * @retval None
  */
void CAN_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data)
{
    CAN_MessageType_t msg_type;
    
    // 更新接收统计
    CAN_UpdateRxStats();
    can_stats.last_rx_time = CAN_GET_TIMESTAMP();
    
    // 打印接收消息
    CAN_PrintMessage("接收", header->StdId, data, header->DLC);
    
    // 获取消息类型
    msg_type = CAN_GetMessageType(header->StdId);
    
    // 根据消息类型处理
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
            // CAN_DEBUG_PRINTF("收到未知消息类型: ID=0x%03X\r\n", (unsigned int)header->StdId);
            break;
    }
    
    // 更新节点状态
    if (msg_type != CAN_MSG_UNKNOWN)
    {
        wcmcu_status = CAN_NODE_ONLINE;
        last_heartbeat_time = CAN_GET_TIMESTAMP();
    }
}

/**
  * @brief  获取消息类型
  * @param  id: 消息ID
  * @retval 消息类型
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
            return CAN_MSG_DATA_RESPONSE;  // WCMCU发送的数据当作响应处理
        default:
            return CAN_MSG_UNKNOWN;
    }
}

/**
  * @brief  处理心跳消息
  * @param  data: 消息数据
  * @param  len: 数据长度
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
            // CAN_DEBUG_PRINTF("收到WCMCU心跳: 计数器=%d\r\n", counter);
            wcmcu_status = CAN_NODE_ONLINE;
        }
        else
        {
            // CAN_DEBUG_PRINTF("心跳消息魔数错误: 0x%04X\r\n", magic);
        }
    }
}

/**
  * @brief  处理数据请求
  * @param  data: 消息数据
  * @param  len: 数据长度
  * @retval None
  */
void CAN_ProcessDataRequest(uint8_t* data, uint8_t len)
{
    if (len >= 2)
    {
        uint8_t req_type = data[0];
        uint8_t req_param = data[1];
        
        // CAN_DEBUG_PRINTF("收到数据请求: 类型=%d, 参数=%d\r\n", req_type, req_param);
        
        // 根据请求类型发送响应
        uint8_t response_data[8];
        switch(req_type)
        {
            case 0x01:  // 请求系统状态
                response_data[0] = 0x01;  // 响应类型
                response_data[1] = (uint8_t)wcmcu_status;
                response_data[2] = (can_stats.tx_count >> 8) & 0xFF;
                response_data[3] = can_stats.tx_count & 0xFF;
                response_data[4] = (can_stats.rx_count >> 8) & 0xFF;
                response_data[5] = can_stats.rx_count & 0xFF;
                response_data[6] = (can_stats.error_count >> 8) & 0xFF;
                response_data[7] = can_stats.error_count & 0xFF;
                break;
                
            case 0x02:  // 请求时间戳
                {
                    uint32_t timestamp = CAN_GET_TIMESTAMP();
                    response_data[0] = 0x02;  // 响应类型
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
                response_data[0] = 0xFF;  // 未知请求
                response_data[1] = req_type;
                memset(&response_data[2], 0, 6);
                break;
        }
        
        CAN_SendDataResponse(response_data, 8);
    }
}

/**
  * @brief  处理数据响应
  * @param  data: 消息数据
  * @param  len: 数据长度
  * @retval None
  */
void CAN_ProcessDataResponse(uint8_t* data, uint8_t len)
{
    // CAN_DEBUG_PRINTF("收到WCMCU数据响应: ");
    // for(int i = 0; i < len; i++)
    // {
    //     printf("%02X ", data[i]);
    // }
    // printf("\r\n");
    
    // 解析响应数据
    if (len >= 1)
    {
        uint8_t resp_type = data[0];
        switch(resp_type)
        {
            case 0x01:  // 系统状态响应
                if (len >= 8)
                {
                    uint8_t node_status = data[1];
                    uint16_t tx_count = (data[2] << 8) | data[3];
                    uint16_t rx_count = (data[4] << 8) | data[5];
                    uint16_t error_count = (data[6] << 8) | data[7];
                    
                    // CAN_DEBUG_PRINTF("WCMCU状态: %d, TX:%d, RX:%d, ERR:%d\r\n", 
                    //                node_status, tx_count, rx_count, error_count);
                }
                break;
                
            case 0x02:  // 时间戳响应
                if (len >= 5)
                {
                    uint32_t wcmcu_time = (data[1] << 24) | (data[2] << 16) | (data[3] << 8) | data[4];
                    // CAN_DEBUG_PRINTF("WCMCU时间戳: %lu ms\r\n", wcmcu_time);
                }
                break;
                
            default:
                // CAN_DEBUG_PRINTF("未知响应类型: %d\r\n", resp_type);
                break;
        }
    }
}

/**
  * @brief  处理状态消息
  * @param  data: 消息数据
  * @param  len: 数据长度
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
            // CAN_DEBUG_PRINTF("WCMCU状态: %d, 计数器: %d, 运行时间: %d秒\r\n", 
            //                status, counter, runtime);
        }
    }
}

/**
  * @brief  处理控制指令
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
            // CAN_DEBUG_PRINTF("收到控制指令: 0x%04X\r\n", cmd);
            
            // 处理控制指令
            switch(cmd)
            {
                case 0x0001:  // 重置统计
                    CAN_ResetStats();
                    // CAN_DEBUG_PRINTF("统计信息已重置\r\n");
                    break;
                    
                case 0x0002:  // 打印统计
                    CAN_PrintStats();
                    break;
                    
                case 0x0003:  // 打印状态
                    CAN_PrintNodeStatus();
                    break;
                    
                default:
                    // CAN_DEBUG_PRINTF("未知控制指令: 0x%04X\r\n", cmd);
                    break;
            }
        }
    }
}

/**
  * @brief  处理错误消息
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
        
        // CAN_DEBUG_PRINTF("收到错误消息: 代码=0x%02X, 数据=0x%02X\r\n", 
        //                error_code, error_data);
        
        // 更新节点状态
        wcmcu_status = CAN_NODE_ERROR;
    }
}

/**
  * @brief  获取WCMCU节点状态
  * @retval 节点状态
  */
CAN_NodeStatus_t CAN_GetWCMCUStatus(void)
{
    return wcmcu_status;
}

/**
  * @brief  更新节点状态
  * @retval None
  */
void CAN_UpdateNodeStatus(void)
{
    uint32_t current_time = CAN_GET_TIMESTAMP();
    
    // 检查超时
    if (wcmcu_status == CAN_NODE_ONLINE)
    {
        if (CAN_IS_TIMEOUT(last_heartbeat_time, CAN_TIMEOUT_PERIOD))
        {
            wcmcu_status = CAN_NODE_TIMEOUT;
            // CAN_DEBUG_PRINTF("WCMCU节点超时\r\n");
        }
    }
}

/**
  * @brief  检查超时
  * @retval None
  */
void CAN_CheckTimeout(void)
{
    CAN_UpdateNodeStatus();
}

/**
  * @brief  重置统计信息
  * @retval None
  */
void CAN_ResetStats(void)
{
    memset(&can_stats, 0, sizeof(can_stats));
    can_stats.start_time = CAN_GET_TIMESTAMP();
    
    heartbeat_counter = 0;
    data_request_counter = 0;
    status_counter = 0;
    
    // CAN_DEBUG_PRINTF("统计信息已重置\r\n");
}

/**
  * @brief  获取统计信息
  * @retval 统计信息指针
  */
CAN_DualNode_Stats_t* CAN_GetStats(void)
{
    return &can_stats;
}

/**
  * @brief  打印统计信息
  * @retval None
  */
void CAN_PrintStats(void)
{
    uint32_t elapsed = CAN_GET_TIMESTAMP() - can_stats.start_time;
    float success_rate = CAN_GetSuccessRate();
    
    // printf("\r\n=== CAN双节点通信统计 ===\r\n");
    // printf("运行时间: %lu ms (%.1f 秒)\r\n", elapsed, elapsed / 1000.0f);
    // printf("发送消息: %lu\r\n", can_stats.tx_count);
    // printf("接收消息: %lu\r\n", can_stats.rx_count);
    // printf("错误次数: %lu\r\n", can_stats.error_count);
    // printf("心跳消息: %lu\r\n", can_stats.heartbeat_count);
    // printf("数据请求: %lu\r\n", can_stats.data_req_count);
    // printf("数据响应: %lu\r\n", can_stats.data_resp_count);
    // printf("通信成功率: %.2f%%\r\n", success_rate);
    // printf("最后接收时间: %lu ms\r\n", can_stats.last_rx_time);
    // printf("========================\r\n\r\n");
}

/**
  * @brief  打印节点状态
  * @retval None
  */
void CAN_PrintNodeStatus(void)
{
    const char* status_str[] = {"离线", "在线", "错误", "超时"};
    
    // printf("\r\n=== 节点状态信息 ===\r\n");
    // printf("WCMCU-230状态: %s\r\n", status_str[wcmcu_status]);
    // printf("最后心跳时间: %lu ms\r\n", last_heartbeat_time);
    // printf("最后发送时间: %lu ms\r\n", last_send_time);
    // printf("当前时间: %lu ms\r\n", CAN_GET_TIMESTAMP());
    // printf("==================\r\n\r\n");
}

/**
  * @brief  获取通信成功率
  * @retval 成功率(百分比)
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
  * @brief  获取总线负载
  * @retval 总线负载(百分比)
  */
uint32_t CAN_GetBusLoad(void)
{
    uint32_t elapsed = CAN_GET_TIMESTAMP() - can_stats.start_time;
    if (elapsed == 0)
    {
        return 0;
    }
    
    // 简单估算: 每个消息约1ms传输时间
    uint32_t total_msg_time = (can_stats.tx_count + can_stats.rx_count) * 1;
    return (total_msg_time * 100) / elapsed;
}

/**
  * @brief  CAN双节点通信任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_DualNodeTask(void const * argument)
{
    // 初始化CAN双节点通信
    if (CAN_DualNode_Init() != HAL_OK)
    {
        // CAN_DEBUG_PRINTF("CAN双节点通信初始化失败\r\n");
        return;
    }
    
    // CAN_DEBUG_PRINTF("CAN双节点通信任务启动\r\n");
    
    for(;;)
    {
        // 周期性发送
        CAN_PeriodicSend();
        
        // 周期性检查
        CAN_PeriodicCheck();
        
        // 任务延时
        osDelay(100);  // 100ms周期
    }
}

/**
  * @brief  周期性发送
  * @retval None
  */
void CAN_PeriodicSend(void)
{
    uint32_t current_time = CAN_GET_TIMESTAMP();
    
    // 发送心跳消息
    if (CAN_IS_TIMEOUT(last_heartbeat_send, CAN_HEARTBEAT_PERIOD))
    {
        CAN_SendHeartbeat();
        last_heartbeat_send = current_time;
    }
    
    // 发送数据请求
    if (CAN_IS_TIMEOUT(last_data_request, CAN_DATA_REQUEST_PERIOD))
    {
        static uint8_t req_type = 1;
        CAN_SendDataRequest(req_type, 0x00);
        req_type = (req_type == 1) ? 2 : 1;  // 交替请求不同类型
        last_data_request = current_time;
    }
    
    // 发送状态消息
    if (CAN_IS_TIMEOUT(last_status_send, CAN_STATUS_PERIOD))
    {
        CAN_SendStatusMessage();
        last_status_send = current_time;
    }
}

/**
  * @brief  周期性检查
  * @retval None
  */
void CAN_PeriodicCheck(void)
{
    static uint32_t last_stats_print = 0;
    uint32_t current_time = CAN_GET_TIMESTAMP();
    
    // 检查超时
    CAN_CheckTimeout();
    
    // 定期打印统计信息(每30秒)
    if (CAN_IS_TIMEOUT(last_stats_print, 30000))
    {
        CAN_PrintStats();
        CAN_PrintNodeStatus();
        last_stats_print = current_time;
    }
}

/**
  * @brief  打印消息
  * @param  prefix: 前缀字符串
  * @param  id: 消息ID
  * @param  data: 数据指针
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
  * @brief  计算校验和
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval 校验和
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
  * @brief  验证校验和
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @param  checksum: 期望的校验和
  * @retval 验证结果
  */
bool CAN_VerifyChecksum(uint8_t* data, uint8_t len, uint16_t checksum)
{
    return (CAN_CalculateChecksum(data, len) == checksum);
}

/**
  * @brief  格式化时间戳
  * @param  timestamp: 时间戳
  * @param  buffer: 输出缓冲区
  * @param  size: 缓冲区大小
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
  * @brief  配置CAN过滤器
  * @retval None
  */
static void CAN_ConfigFilter(void)
{
    CAN_FilterTypeDef sFilterConfig;
    
    // 配置过滤器接受所有消息
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
  * @brief  更新发送统计
  * @retval None
  */
static void CAN_UpdateTxStats(void)
{
    can_stats.tx_count++;
}

/**
  * @brief  更新接收统计
  * @retval None
  */
static void CAN_UpdateRxStats(void)
{
    can_stats.rx_count++;
}

/**
  * @brief  更新错误统计
  * @retval None
  */
static void CAN_UpdateErrorStats(void)
{
    can_stats.error_count++;
}

/* Interrupt Callbacks -------------------------------------------------------*/

/**
  * @brief  CAN接收FIFO0消息挂起回调
  * @param  hcan: CAN句柄指针
  * @retval None
  */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            // 打印CAN1接收到的消息
            printf("[CAN1-RX] ID:0x%03X, DLC:%d, Data:", (unsigned int)RxHeader.StdId, RxHeader.DLC);
            for (int i = 0; i < RxHeader.DLC && i < 8; i++) {
                printf("%02X ", RxData[i]);
            }
            printf("\r\n");
            
            // 处理双节点通信消息
            CAN_ProcessReceivedMessage(&RxHeader, RxData);
            
            // 处理循环测试消息
            CAN_LoopTest_ProcessSTM32Message(&RxHeader, RxData);
            
            // CAN1桥接测试消息处理已禁用
            // CAN1_CAN2_BridgeTest_ProcessCAN1Reception(&RxHeader, RxData);
        }
    }
    else if (hcan->Instance == CAN2)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            // CAN2仅作为接收器，打印所有CAN总线上的报文
            CAN2_Demo_ProcessReceivedMessage(&RxHeader, RxData);
            
            // 环回测试已禁用 - CAN2仅接收
            // CAN2_LoopbackTest_ProcessMessage(&RxHeader, RxData);
        }
    }
}

/**
  * @brief  CAN发送邮箱0完成回调
  * @param  hcan: CAN句柄指针
  * @retval None
  */
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        // 发送完成处理
    }
}

/**
  * @brief  CAN发送邮箱1完成回调
  * @param  hcan: CAN句柄指针
  * @retval None
  */
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        // 发送完成处理
    }
}

/**
  * @brief  CAN发送邮箱2完成回调
  * @param  hcan: CAN句柄指针
  * @retval None
  */
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        // 发送完成处理
    }
}

/**
  * @brief  CAN错误回调
  * @param  hcan: CAN句柄指针
  * @retval None
  */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        uint32_t error_code = HAL_CAN_GetError(hcan);
        // CAN_DEBUG_PRINTF("CAN错误: 0x%08lX\r\n", error_code);
        CAN_UpdateErrorStats();
    }
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/