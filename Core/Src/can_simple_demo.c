/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_simple_demo.c
  * @brief          : 最简单的CAN1发送Demo
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件实现了最简单的CAN1发送Demo：
  * 1. 周期发送多种类型的CAN报文
  * 2. 便于在CANoe中观察报文
  * 3. 取消双节点通信功能
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "can_simple_demo.h"
#include "can.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CAN_DEMO_HEARTBEAT_ID   0x100   // 心跳消息ID
#define CAN_DEMO_DATA_ID        0x200   // 数据消息ID
#define CAN_DEMO_STATUS_ID      0x300   // 状态消息ID
#define CAN_DEMO_CONTROL_ID     0x400   // 控制消息ID
#define CAN_DEMO_DEBUG_ID       0x500   // 调试消息ID

#define HEARTBEAT_PERIOD_MS     1000    // 心跳周期：1秒
#define DATA_PERIOD_MS          2000    // 数据周期：2秒
#define STATUS_PERIOD_MS        3000    // 状态周期：3秒
#define CONTROL_PERIOD_MS       5000    // 控制周期：5秒
#define DEBUG_PERIOD_MS         10000   // 调试周期：10秒

/* Private variables ---------------------------------------------------------*/
static uint32_t heartbeat_counter = 0;
static uint32_t data_counter = 0;
static uint32_t status_counter = 0;
static uint32_t control_counter = 0;
static uint32_t debug_counter = 0;

static uint32_t last_heartbeat_time = 0;
static uint32_t last_data_time = 0;
static uint32_t last_status_time = 0;
static uint32_t last_control_time = 0;
static uint32_t last_debug_time = 0;

static uint8_t demo_initialized = 0;

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan1;

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef CAN_Demo_SendHeartbeat(void);
static HAL_StatusTypeDef CAN_Demo_SendDataMessage(void);
static HAL_StatusTypeDef CAN_Demo_SendStatusMessage(void);
static HAL_StatusTypeDef CAN_Demo_SendControlMessage(void);
static HAL_StatusTypeDef CAN_Demo_SendDebugMessage(void);

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  CAN简单Demo初始化
  * @param  None
  * @retval HAL_OK: 成功, HAL_ERROR: 失败
  */
HAL_StatusTypeDef CAN_SimpleDemo_Init(void)
{
    // printf("\r\n=== CAN Simple Demo Initialization ===\r\n");
    
    // 配置CAN1过滤器（接受所有消息）
    CAN_FilterTypeDef sFilterConfig;
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
    
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        // printf("[ERROR] CAN1 filter configuration failed!\r\n");
        return HAL_ERROR;
    }
    
    // 启动CAN1
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        // printf("[ERROR] CAN1 start failed!\r\n");
        return HAL_ERROR;
    }
    
    // 激活CAN1接收中断
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        // printf("[ERROR] CAN1 RX interrupt activation failed!\r\n");
        return HAL_ERROR;
    }
    
    // 初始化计数器和时间戳
    heartbeat_counter = 0;
    data_counter = 0;
    status_counter = 0;
    control_counter = 0;
    debug_counter = 0;
    
    last_heartbeat_time = 0;
    last_data_time = 0;
    last_status_time = 0;
    last_control_time = 0;
    last_debug_time = 0;
    
    demo_initialized = 1;
    
    // printf("[INFO] CAN Simple Demo initialized successfully\r\n");
    // printf("[INFO] Message types and periods:\r\n");
    // printf("  - Heartbeat (0x%03X): %d ms\r\n", CAN_DEMO_HEARTBEAT_ID, HEARTBEAT_PERIOD_MS);
    // printf("  - Data (0x%03X): %d ms\r\n", CAN_DEMO_DATA_ID, DATA_PERIOD_MS);
    // printf("  - Status (0x%03X): %d ms\r\n", CAN_DEMO_STATUS_ID, STATUS_PERIOD_MS);
    // printf("  - Control (0x%03X): %d ms\r\n", CAN_DEMO_CONTROL_ID, CONTROL_PERIOD_MS);
    // printf("  - Debug (0x%03X): %d ms\r\n", CAN_DEMO_DEBUG_ID, DEBUG_PERIOD_MS);
    // printf("=====================================\r\n\r\n");
    
    return HAL_OK;
}

/**
  * @brief  CAN简单Demo主任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_SimpleDemo_Task(void *argument)
{
    // printf("[TASK] CAN Simple Demo task started\r\n");
    
    // 等待初始化完成
    while (!demo_initialized) {
        osDelay(100);
    }
    
    // printf("[TASK] Starting CAN message transmission...\r\n");
    
    for (;;) {
        uint32_t current_time = HAL_GetTick();
        
        // 发送心跳消息
        if ((current_time - last_heartbeat_time) >= HEARTBEAT_PERIOD_MS) {
            if (CAN_Demo_SendHeartbeat() == HAL_OK) {
                heartbeat_counter++;
                last_heartbeat_time = current_time;
                // printf("[TX] Heartbeat #%lu sent (ID: 0x%03X)\r\n", heartbeat_counter, CAN_DEMO_HEARTBEAT_ID);
            }
        }
        
        // 发送数据消息
        if ((current_time - last_data_time) >= DATA_PERIOD_MS) {
            if (CAN_Demo_SendDataMessage() == HAL_OK) {
                data_counter++;
                last_data_time = current_time;
                // printf("[TX] Data #%lu sent (ID: 0x%03X)\r\n", data_counter, CAN_DEMO_DATA_ID);
            }
        }
        
        // 发送状态消息
        if ((current_time - last_status_time) >= STATUS_PERIOD_MS) {
            if (CAN_Demo_SendStatusMessage() == HAL_OK) {
                status_counter++;
                last_status_time = current_time;
                // printf("[TX] Status #%lu sent (ID: 0x%03X)\r\n", status_counter, CAN_DEMO_STATUS_ID);
            }
        }
        
        // 发送控制消息
        if ((current_time - last_control_time) >= CONTROL_PERIOD_MS) {
            if (CAN_Demo_SendControlMessage() == HAL_OK) {
                control_counter++;
                last_control_time = current_time;
                // printf("[TX] Control #%lu sent (ID: 0x%03X)\r\n", control_counter, CAN_DEMO_CONTROL_ID);
            }
        }
        
        // 发送调试消息
        if ((current_time - last_debug_time) >= DEBUG_PERIOD_MS) {
            if (CAN_Demo_SendDebugMessage() == HAL_OK) {
                debug_counter++;
                last_debug_time = current_time;
                // printf("[TX] Debug #%lu sent (ID: 0x%03X)\r\n", debug_counter, CAN_DEMO_DEBUG_ID);
            }
        }
        
        // 任务延时
        osDelay(50);
    }
}

/**
  * @brief  获取Demo统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN_SimpleDemo_GetStats(CAN_SimpleDemo_Stats_t *stats)
{
    if (stats != NULL) {
        stats->heartbeat_count = heartbeat_counter;
        stats->data_count = data_counter;
        stats->status_count = status_counter;
        stats->control_count = control_counter;
        stats->debug_count = debug_counter;
        stats->total_count = heartbeat_counter + data_counter + status_counter + control_counter + debug_counter;
        stats->initialized = demo_initialized;
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  发送心跳消息
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_Demo_SendHeartbeat(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    uint32_t current_time = HAL_GetTick();
    
    TxHeader.StdId = CAN_DEMO_HEARTBEAT_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 心跳消息数据
    TxData[0] = 0xAA;  // 心跳标识
    TxData[1] = 0x55;  // 心跳标识
    TxData[2] = (uint8_t)(heartbeat_counter >> 8);
    TxData[3] = (uint8_t)heartbeat_counter;
    TxData[4] = (uint8_t)(current_time >> 24);
    TxData[5] = (uint8_t)(current_time >> 16);
    TxData[6] = (uint8_t)(current_time >> 8);
    TxData[7] = (uint8_t)current_time;
    
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    
    if (status == HAL_OK) {
        // Print CAN1 transmit log
        printf("[CAN1-TX] ID:0x%03X, DLC:%d, Data:", (unsigned int)TxHeader.StdId, TxHeader.DLC);
        for (int i = 0; i < TxHeader.DLC && i < 8; i++) {
            printf("%02X ", TxData[i]);
        }
        printf("\r\n");
    }
    
    return status;
}

/**
  * @brief  发送数据消息
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_Demo_SendDataMessage(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    
    TxHeader.StdId = CAN_DEMO_DATA_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 数据消息（模拟传感器数据）
    TxData[0] = 0xDA;  // 数据标识
    TxData[1] = 0x7A;  // 数据标识
    TxData[2] = (uint8_t)(data_counter >> 8);
    TxData[3] = (uint8_t)data_counter;
    TxData[4] = (uint8_t)((data_counter * 123) & 0xFF);  // 模拟温度
    TxData[5] = (uint8_t)((data_counter * 456) & 0xFF);  // 模拟湿度
    TxData[6] = (uint8_t)((data_counter * 789) & 0xFF);  // 模拟压力
    TxData[7] = (uint8_t)((data_counter * 321) & 0xFF);  // 模拟电压
    
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    
    if (status == HAL_OK) {
        // Print CAN1 transmit log
        printf("[CAN1-TX] ID:0x%03X, DLC:%d, Data:", (unsigned int)TxHeader.StdId, TxHeader.DLC);
        for (int i = 0; i < TxHeader.DLC && i < 8; i++) {
            printf("%02X ", TxData[i]);
        }
        printf("\r\n");
    }
    
    return status;
}

/**
  * @brief  发送状态消息
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_Demo_SendStatusMessage(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    
    TxHeader.StdId = CAN_DEMO_STATUS_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 状态消息
    TxData[0] = 0x5A;  // 状态标识
    TxData[1] = 0xA5;  // 状态标识
    TxData[2] = (uint8_t)(status_counter >> 8);
    TxData[3] = (uint8_t)status_counter;
    TxData[4] = 0x01;  // 系统状态：正常
    TxData[5] = 0x00;  // 错误代码：无错误
    TxData[6] = (uint8_t)(heartbeat_counter & 0xFF);  // 心跳计数
    TxData[7] = (uint8_t)(data_counter & 0xFF);       // 数据计数
    
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    
    if (status == HAL_OK) {
        // Print CAN1 transmit log
        printf("[CAN1-TX] ID:0x%03X, DLC:%d, Data:", (unsigned int)TxHeader.StdId, TxHeader.DLC);
        for (int i = 0; i < TxHeader.DLC && i < 8; i++) {
            printf("%02X ", TxData[i]);
        }
        printf("\r\n");
    }
    
    return status;
}

/**
  * @brief  发送控制消息
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_Demo_SendControlMessage(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    
    TxHeader.StdId = CAN_DEMO_CONTROL_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 控制消息
    TxData[0] = 0xC0;  // 控制标识
    TxData[1] = 0x01;  // 控制标识
    TxData[2] = (uint8_t)(control_counter >> 8);
    TxData[3] = (uint8_t)control_counter;
    TxData[4] = (control_counter % 2) ? 0xFF : 0x00;  // 开关控制
    TxData[5] = (uint8_t)(control_counter % 256);     // PWM值
    TxData[6] = 0x12;  // 预留
    TxData[7] = 0x34;  // 预留
    
    return HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
}

/**
  * @brief  发送调试消息
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_Demo_SendDebugMessage(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    uint32_t current_time = HAL_GetTick();
    
    TxHeader.StdId = CAN_DEMO_DEBUG_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 调试消息
    TxData[0] = 0xDE;  // 调试标识
    TxData[1] = 0xBE;  // 调试标识
    TxData[2] = (uint8_t)(debug_counter >> 8);
    TxData[3] = (uint8_t)debug_counter;
    TxData[4] = (uint8_t)(current_time >> 24);
    TxData[5] = (uint8_t)(current_time >> 16);
    TxData[6] = (uint8_t)(current_time >> 8);
    TxData[7] = (uint8_t)current_time;
    
    return HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
}