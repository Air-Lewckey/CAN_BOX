/**
 * @file can_testbox_peps_helper.c
 * @brief PEPS系统CAN测试辅助模块
 * @version 1.1
 * @date 2024
 * 
 * @note 周期性消息管理机制说明：
 * 本模块使用两种方式管理周期性消息：
 * 1. g_peps_periodic_handles数组：用于管理常规周期性消息
 * 2. g_scw1_handle变量：专门用于管理0x05B报文
 * 
 * 为确保所有周期性消息能被正确停止，本模块提供以下函数：
 * - PEPS_Helper_StopPeriodicMessage：停止单个周期性消息
 * - PEPS_Helper_StopAllPeriodicMessages：停止g_peps_periodic_handles数组中的所有周期性消息和g_scw1_handle对应的消息
 * - PEPS_Helper_StopAllPeriodicMessagesEx：封装CAN_TestBox_StopAllPeriodicMessages函数，并重置所有句柄
 * 
 * 所有需要停止全部周期性消息的指令（如B1、FF等）都应使用PEPS_Helper_StopAllPeriodicMessagesEx函数
 */

#include "can_testbox_api.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* ========================= 私有宏定义 ========================= */

// PEPS报文ID定义 - 根据SCW1_SCW2 PEPS CAN通讯矩阵
#define PEPS_WAKEUP_TX_ID        0x104   // PEPS唤醒帧发送ID
#define PEPS_WAKEUP_RX_ID        0x105   // PEPS唤醒帧接收ID
#define PEPS_DIAG_REQ_ID         0x7A0   // PEPS诊断请求ID
#define PEPS_DIAG_RESP_ID        0x7A8   // PEPS诊断响应ID
#define PEPS_VERSION_ID          0x300   // PEPS版本信息ID
#define PEPS_STATUS_ID           0x301   // PEPS状态监控ID
#define PEPS_KEY_LEARN_ID        0x302   // PEPS钥匙学习ID
#define PEPS_SECURITY_ID         0x303   // PEPS网络安全ID

// 周期性消息索引
#define PEPS_WAKEUP_INDEX        0
#define PEPS_STATUS_INDEX        1
#define PEPS_VERSION_INDEX       2
#define PEPS_SECURITY_INDEX      3

// 周期性消息周期
#define PEPS_WAKEUP_PERIOD       200  // 200ms
#define PEPS_STATUS_PERIOD       100  // 100ms
#define PEPS_VERSION_PERIOD      500  // 500ms
#define PEPS_SECURITY_PERIOD     1000 // 1000ms

/* ========================= 私有变量定义 ========================= */

// 周期性消息句柄
static uint8_t g_peps_periodic_handles[4] = {0};

// 特定报文句柄
static uint8_t g_scw1_handle = 0;  // 0x05B报文句柄

// UART接收缓冲区
static uint8_t g_uart_rx_char = 0;

/* ========================= 私有函数声明 ========================= */

static void PEPS_Helper_ProcessChar(uint8_t received_char);
static void PEPS_Helper_StopAllPeriodicMessagesEx(void);
static void PEPS_Helper_StartPeriodicMessage(uint8_t index, uint32_t id, uint8_t *data, uint32_t period);
static void PEPS_Helper_StopPeriodicMessage(uint8_t index);

/* ========================= 公共API实现 ========================= */

/**
 * @brief 初始化PEPS测试辅助模块
 * @retval HAL状态
 */
HAL_StatusTypeDef PEPS_Helper_Init(void)
{
    // 确保UART已经初始化 (Ensure UART is initialized)
    // 先中止可能存在的接收 (Abort any existing reception)
    HAL_UART_AbortReceive_IT(&huart2);
    
    // 重新启动串口单字符中断接收 (Restart UART single character interrupt reception)
    HAL_StatusTypeDef uart_status = HAL_UART_Receive_IT(&huart2, &g_uart_rx_char, 1);
    
    if (uart_status != HAL_OK)
    {
        // 不打印错误信息 (Don't print error message)
        return HAL_ERROR;
    }
    
    // 不打印初始化成功信息 (Don't print initialization success message)
    
    return HAL_OK;
}

/**
 * @brief 停止所有周期性消息
 * @note 此函数会停止g_peps_periodic_handles数组中的所有周期性消息和g_scw1_handle对应的消息
 */
void PEPS_Helper_StopAllPeriodicMessages(void)
{
    // 停止g_peps_periodic_handles数组中的所有周期性消息
    for (uint8_t i = 0; i < 4; i++)
    {
        PEPS_Helper_StopPeriodicMessage(i);
    }
    
    // 确保g_scw1_handle也被停止（可能不在g_peps_periodic_handles数组中）
    if (g_scw1_handle != 0)
    {
        CAN_TestBox_StopPeriodicMessage(g_scw1_handle);
        g_scw1_handle = 0;
    }
    
    // 不打印停止消息信息 (Don't print stop message information)
}

/**
 * @brief 停止所有周期性消息的扩展函数
 * @note 此函数会调用CAN_TestBox_StopAllPeriodicMessages并重置g_scw1_handle和g_peps_periodic_handles数组
 */
static void PEPS_Helper_StopAllPeriodicMessagesEx(void)
{
    // 调用CAN_TestBox_StopAllPeriodicMessages停止所有周期性消息
    CAN_TestBox_StopAllPeriodicMessages();
    
    // 重置所有相关句柄
    g_scw1_handle = 0;
    for (uint8_t i = 0; i < 4; i++)
    {
        g_peps_periodic_handles[i] = 0;
    }
    
    // 不打印停止消息信息 (Don't print stop message information)
}

/* ========================= 私有函数实现 ========================= */

/**
 * @brief 处理串口接收到的字符
 * @param received_char: 接收到的字符
 */
static void PEPS_Helper_ProcessChar(uint8_t received_char)
{
    uint8_t data[8] = {0};
    
    switch (received_char)
    {
        // PEPS控制指令 (0xA1-0xB4)
        case 0xA1:  // 开启SCW1唤醒
            {
                // 先停止之前可能存在的SCW1唤醒报文
                if (g_scw1_handle != 0) {
                    CAN_TestBox_StopPeriodicMessage(g_scw1_handle);
                    g_scw1_handle = 0;
                }
                
                data[0] = 0x01;
                
                // 创建新的CAN消息
                CAN_TestBox_Message_t message;
                message.id = 0x05B;
                message.dlc = 8;
                message.is_extended = false;
                message.is_remote = false;
                memcpy(message.data, data, 8);
                
                // 直接启动周期性消息，并保存句柄
                CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(&message, PEPS_WAKEUP_PERIOD, &g_scw1_handle);
                (void)status;  // 消除未使用变量警告
                
                // 确保全局数组中保存的是相同的句柄
                g_peps_periodic_handles[PEPS_WAKEUP_INDEX] = g_scw1_handle;
                
                // 不打印启动消息状态 (Don't print message start status)
            }
            break;
            
        case 0xB1:  // 关闭SCW1唤醒
            // 停止所有周期性消息，确保0x05B报文被停止
            PEPS_Helper_StopAllPeriodicMessagesEx();
            
            // 打印停止消息状态
            printf("SCW1 wakeup message stopped\r\n");
            break;
            
        case 0xA2:  // 开启SCW2唤醒
            data[0] = 0x00;
            PEPS_Helper_StartPeriodicMessage(PEPS_WAKEUP_INDEX, 0x401, data, 500);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xB2:  // 关闭SCW2唤醒
            PEPS_Helper_StopPeriodicMessage(PEPS_WAKEUP_INDEX);
            // 不打印停止消息状态 (Don't print message stop status)
            break;
            
        case 0xA3:  // 开启钥匙位置
            data[0] = 0x01;
            PEPS_Helper_StartPeriodicMessage(PEPS_STATUS_INDEX, 0x442, data, PEPS_STATUS_PERIOD);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xB3:  // 关闭钥匙位置
            PEPS_Helper_StopPeriodicMessage(PEPS_STATUS_INDEX);
            printf("[PEPS-TX] Stopped key position message\r\n");
            break;
            
        case 0xA4:  // 开启BSI状态
            data[0] = 0x01;
            PEPS_Helper_StartPeriodicMessage(PEPS_VERSION_INDEX, 0x036, data, PEPS_STATUS_PERIOD);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xB4:  // 关闭BSI状态
            PEPS_Helper_StopPeriodicMessage(PEPS_VERSION_INDEX);
            printf("[PEPS-TX] Stopped BSI status message\r\n");
            break;
            
        // 数据变体指令 (0xC1-0xE4)
        case 0xC1:  // SCW1唤醒(REV=0)
            {
                // 先停止之前可能存在的SCW1唤醒报文
                if (g_scw1_handle != 0) {
                    CAN_TestBox_StopPeriodicMessage(g_scw1_handle);
                    g_scw1_handle = 0;
                }
                
                data[0] = 0x00;
                
                // 创建新的CAN消息
                CAN_TestBox_Message_t message;
                message.id = 0x05B;
                message.dlc = 8;
                message.is_extended = false;
                message.is_remote = false;
                memcpy(message.data, data, 8);
                
                // 直接启动周期性消息，并保存句柄
                CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(&message, PEPS_WAKEUP_PERIOD, &g_scw1_handle);
                (void)status;  // 消除未使用变量警告
                
                // 确保全局数组中保存的是相同的句柄
                g_peps_periodic_handles[PEPS_WAKEUP_INDEX] = g_scw1_handle;
                
                // 不打印启动消息状态 (Don't print message start status)
            }
            break;
            
        case 0xC2:  // SCW2唤醒(激活)
            data[0] = 0x01;
            PEPS_Helper_StartPeriodicMessage(PEPS_WAKEUP_INDEX, 0x401, data, 500);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xC3:  // 钥匙不在位
            data[0] = 0x00;
            PEPS_Helper_StartPeriodicMessage(PEPS_STATUS_INDEX, 0x442, data, PEPS_STATUS_PERIOD);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xC4:  // BSI异常状态
            data[0] = 0x00;
            PEPS_Helper_StartPeriodicMessage(PEPS_VERSION_INDEX, 0x036, data, PEPS_STATUS_PERIOD);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xD1:  // 开启SCW1唤醒(自定义1)
            {
                // 先停止之前可能存在的SCW1唤醒报文
                if (g_scw1_handle != 0) {
                    CAN_TestBox_StopPeriodicMessage(g_scw1_handle);
                    g_scw1_handle = 0;
                }
                
                data[0] = 0x02;
                
                // 创建新的CAN消息
                CAN_TestBox_Message_t message;
                message.id = 0x05B;
                message.dlc = 8;
                message.is_extended = false;
                message.is_remote = false;
                memcpy(message.data, data, 8);
                
                // 直接启动周期性消息，并保存句柄
                CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(&message, PEPS_WAKEUP_PERIOD, &g_scw1_handle);
                (void)status;  // 消除未使用变量警告
                
                // 确保全局数组中保存的是相同的句柄
                g_peps_periodic_handles[PEPS_WAKEUP_INDEX] = g_scw1_handle;
                
                // 不打印启动消息状态 (Don't print message start status)
            }
            break;
            
        case 0xD2:  // SCW2唤醒(自定义1)
            data[0] = 0x02;
            PEPS_Helper_StartPeriodicMessage(PEPS_WAKEUP_INDEX, 0x401, data, 500);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xD3:  // 钥匙插入中
            data[0] = 0x02;
            PEPS_Helper_StartPeriodicMessage(PEPS_STATUS_INDEX, 0x442, data, PEPS_STATUS_PERIOD);
            printf("[PEPS-TX] Started key position message (inserting)\r\n");
            break;
            
        case 0xD4:  // BSI待机状态
            data[0] = 0x02;
            PEPS_Helper_StartPeriodicMessage(PEPS_VERSION_INDEX, 0x036, data, PEPS_STATUS_PERIOD);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xE1:  // SCW1唤醒(自定义2)
            {
                // 先停止之前可能存在的SCW1唤醒报文
                if (g_scw1_handle != 0) {
                    CAN_TestBox_StopPeriodicMessage(g_scw1_handle);
                    g_scw1_handle = 0;
                }
                
                data[0] = 0x03;
                
                // 创建新的CAN消息
                CAN_TestBox_Message_t message;
                message.id = 0x05B;
                message.dlc = 8;
                message.is_extended = false;
                message.is_remote = false;
                memcpy(message.data, data, 8);
                
                // 直接启动周期性消息，并保存句柄
                CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(&message, PEPS_WAKEUP_PERIOD, &g_scw1_handle);
                (void)status;  // 消除未使用变量警告
                
                // 确保全局数组中保存的是相同的句柄
                g_peps_periodic_handles[PEPS_WAKEUP_INDEX] = g_scw1_handle;
                
                // 不打印启动消息状态 (Don't print message start status)
            }
            break;
            
        case 0xE2:  // SCW2唤醒(自定义2)
            data[0] = 0x03;
            PEPS_Helper_StartPeriodicMessage(PEPS_WAKEUP_INDEX, 0x401, data, 500);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xE3:  // 钥匙拔出中
            data[0] = 0x03;
            PEPS_Helper_StartPeriodicMessage(PEPS_STATUS_INDEX, 0x442, data, PEPS_STATUS_PERIOD);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        case 0xE4:  // BSI初始化状态
            data[0] = 0x03;
            PEPS_Helper_StartPeriodicMessage(PEPS_VERSION_INDEX, 0x036, data, PEPS_STATUS_PERIOD);
            // 不打印启动消息状态 (Don't print message start status)
            break;
            
        // 测试指令 (0xF1-0xF4)
        case 0xF1:  // SCW1完整测试数据
            {
                // 先停止之前可能存在的SCW1唤醒报文
                if (g_scw1_handle != 0) {
                    CAN_TestBox_StopPeriodicMessage(g_scw1_handle);
                    g_scw1_handle = 0;
                }
                
                data[0] = 0x01; data[1] = 0x02; data[2] = 0x03; data[3] = 0x04;
                data[4] = 0x05; data[5] = 0x06; data[6] = 0x07; data[7] = 0x08;
                
                // 创建新的CAN消息
                CAN_TestBox_Message_t message;
                message.id = 0x05B;
                message.dlc = 8;
                message.is_extended = false;
                message.is_remote = false;
                memcpy(message.data, data, 8);
                
                // 直接启动周期性消息，并保存句柄
                CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(&message, PEPS_WAKEUP_PERIOD, &g_scw1_handle);
                (void)status;  // 消除未使用变量警告
                
                // 确保全局数组中保存的是相同的句柄
                g_peps_periodic_handles[PEPS_WAKEUP_INDEX] = g_scw1_handle;
                
                // 不打印测试序列状态 (Don't print test sequence status)
            }
            break;
            
        case 0xF2:  // SCW2完整测试数据
            data[0] = 0x11; data[1] = 0x22; data[2] = 0x33; data[3] = 0x44;
            data[4] = 0x55; data[5] = 0x66; data[6] = 0x77; data[7] = 0x88;
            PEPS_Helper_StartPeriodicMessage(PEPS_WAKEUP_INDEX, 0x401, data, 500);
            // 不打印测试序列状态 (Don't print test sequence status)
            break;
            
        case 0xF3:  // 钥匙位置完整数据
            data[0] = 0xAA; data[1] = 0xBB; data[2] = 0xCC; data[3] = 0xDD;
            data[4] = 0xEE; data[5] = 0xFF; data[6] = 0x00; data[7] = 0x11;
            PEPS_Helper_StartPeriodicMessage(PEPS_STATUS_INDEX, 0x442, data, PEPS_STATUS_PERIOD);
            // 不打印测试序列状态 (Don't print test sequence status)
            break;
            
        case 0xF4:  // BSI完整测试数据
            data[0] = 0xFF; data[1] = 0xEE; data[2] = 0xDD; data[3] = 0xCC;
            data[4] = 0xBB; data[5] = 0xAA; data[6] = 0x99; data[7] = 0x88;
            PEPS_Helper_StartPeriodicMessage(PEPS_VERSION_INDEX, 0x036, data, PEPS_STATUS_PERIOD);
            // 不打印测试序列状态 (Don't print test sequence status)
            break;
            
        // 系统控制指令 (0xFF-0x00)
        case 0xFF:  // 停止所有周期报文
            // 使用封装函数停止所有周期性消息
            PEPS_Helper_StopAllPeriodicMessagesEx();
            
            // 不打印停止消息状态 (Don't print stop message status)
            break;
            
        case 0x00:  // 系统复位
            // 不打印系统重置信息 (Don't print system reset information)
            NVIC_SystemReset();
            break;
            
        default:
            // 忽略其他字符
            // 不打印未知命令信息 (Don't print unknown command information)
            break;
    }
}

/**
 * @brief 启动周期性消息
 * @param index: 消息索引
 * @param id: CAN ID
 * @param data: 数据指针
 * @param period: 周期(ms)
 */
static void PEPS_Helper_StartPeriodicMessage(uint8_t index, uint32_t id, uint8_t *data, uint32_t period)
{
    // 先停止之前的周期性消息
    PEPS_Helper_StopPeriodicMessage(index);
    
    // 创建新的CAN消息
    CAN_TestBox_Message_t message;
    message.id = id;
    message.dlc = 8;
    message.is_extended = false;
    message.is_remote = false;
    memcpy(message.data, data, 8);
    
    // 启动周期性消息
    CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(&message, period, &g_peps_periodic_handles[index]);
    (void)status;  // 消除未使用变量警告
    
    // 不打印启动周期性消息错误信息 (Don't print periodic message start error)
}

/**
 * @brief 停止周期性消息
 * @param index: 消息索引
 */
static void PEPS_Helper_StopPeriodicMessage(uint8_t index)
{
    if (g_peps_periodic_handles[index] != 0)
    {
        CAN_TestBox_StopPeriodicMessage(g_peps_periodic_handles[index]);
        g_peps_periodic_handles[index] = 0;
    }
}

/**
 * @brief 串口接收完成回调函数（弱定义，可被其他模块覆盖）
 * @param huart: UART句柄
 */
__weak void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // 处理接收到的字符
        PEPS_Helper_ProcessChar(g_uart_rx_char);
        
        // 重新启动单字符接收
        HAL_UART_Receive_IT(&huart2, &g_uart_rx_char, 1);
    }
}