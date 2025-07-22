/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_periodic_send.c
  * @brief          : STM32 CAN1周期性发送任务实现
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件实现了STM32自带CAN1的周期性发送功能：
  * 1. 每2秒发送一个CAN报文
  * 2. 报文包含时间戳和计数器信息
  * 3. 使用标准帧格式
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "can_periodic_send.h"
#include "main.h"
#include "can.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CAN_PERIODIC_SEND_ID    0x123   // 周期性发送的CAN ID
#define SEND_PERIOD_MS          2000    // 发送周期：2秒

/* Private variables ---------------------------------------------------------*/
static uint32_t send_counter = 0;       // 发送计数器
static uint32_t last_send_time = 0;     // 上次发送时间
static uint8_t task_initialized = 0;    // 任务初始化标志

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan1;

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef CAN_SendPeriodicMessage(void);

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  CAN周期性发送任务初始化
  * @param  None
  * @retval HAL_OK: 成功, HAL_ERROR: 失败
  */
HAL_StatusTypeDef CAN_PeriodicSend_Init(void)
{
    // 检查CAN1是否已经启动
    if ((hcan1.Instance->MSR & CAN_MSR_INAK) == 0) {
        // CAN1已经启动，无需重复启动
        printf("[INFO] CAN1 already started, skipping initialization\r\n");
        
        // 初始化任务状态变量
        task_initialized = 1;
        send_counter = 0;
        last_send_time = 0;
        
        printf("[INFO] CAN Periodic Send initialized successfully\r\n");
        return HAL_OK;
    }
    
    // 配置CAN1过滤器（接受所有消息）
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 1;  // 使用过滤器组1，避免与其他模块冲突
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
        printf("[ERROR] CAN1 filter configuration failed!\r\n");
        return HAL_ERROR;
    }
    
    // 启动CAN1
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        printf("[ERROR] CAN1 start failed!\r\n");
        return HAL_ERROR;
    }
    
    // 激活CAN1接收中断
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        printf("[ERROR] CAN1 RX interrupt activation failed!\r\n");
        return HAL_ERROR;
    }
    
    task_initialized = 1;
    send_counter = 0;
    last_send_time = 0;
    
    printf("[INFO] CAN1 periodic send task initialized successfully\r\n");
    printf("[INFO] Will send message every %d ms on ID 0x%03X\r\n", SEND_PERIOD_MS, CAN_PERIODIC_SEND_ID);
    
    return HAL_OK;
}

/**
  * @brief  CAN周期性发送任务主函数
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_PeriodicSend_Task(void *argument)
{
    printf("[TASK] CAN1 periodic send task started\r\n");
    
    // 等待任务初始化完成
    while (!task_initialized) {
        osDelay(100);
    }
    
    printf("[TASK] Starting periodic CAN1 transmission (every %d ms)\r\n", SEND_PERIOD_MS);
    
    for (;;) {
        uint32_t current_time = HAL_GetTick();
        
        // 检查是否到了发送时间
        if ((current_time - last_send_time) >= SEND_PERIOD_MS) {
            // 发送周期性消息
            if (CAN_SendPeriodicMessage() == HAL_OK) {
                send_counter++;
                last_send_time = current_time;
                printf("[CAN1-TX] Periodic message #%lu sent successfully at %lu ms\r\n", 
                       send_counter, current_time);
            } else {
                printf("[ERROR] CAN1 periodic message send failed at %lu ms\r\n", current_time);
            }
        }
        
        // 任务延时100ms
        osDelay(100);
    }
}

/**
  * @brief  获取周期性发送统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN_PeriodicSend_GetStats(CAN_PeriodicSend_Stats_t *stats)
{
    if (stats != NULL) {
        stats->send_count = send_counter;
        stats->last_send_time = last_send_time;
        stats->initialized = task_initialized;
        stats->next_send_time = last_send_time + SEND_PERIOD_MS;
    }
}

/**
  * @brief  重置周期性发送统计信息
  * @retval None
  */
void CAN_PeriodicSend_ResetStats(void)
{
    send_counter = 0;
    last_send_time = 0;
    printf("[INFO] CAN1 periodic send statistics reset\r\n");
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  发送周期性CAN消息
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_SendPeriodicMessage(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    uint32_t current_time = HAL_GetTick();
    
    // 配置发送头
    TxHeader.StdId = CAN_PERIODIC_SEND_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 构造消息数据
    TxData[0] = 0xCA;  // 消息标识
    TxData[1] = 0xFE;  // 消息标识
    TxData[2] = (uint8_t)(send_counter >> 8);   // 发送计数高字节
    TxData[3] = (uint8_t)send_counter;          // 发送计数低字节
    TxData[4] = (uint8_t)(current_time >> 24);  // 时间戳字节3
    TxData[5] = (uint8_t)(current_time >> 16);  // 时间戳字节2
    TxData[6] = (uint8_t)(current_time >> 8);   // 时间戳字节1
    TxData[7] = (uint8_t)current_time;          // 时间戳字节0
    
    // 发送消息
    return HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
}