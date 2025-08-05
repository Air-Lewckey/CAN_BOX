/**
  ******************************************************************************
  * @file           : can_trigger_send.c
  * @brief          : 触发式CAN发送模块
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件实现了触发式CAN发送功能：
  * 1. 通过串口接收命令触发CAN报文发送
  * 2. 支持三种不同ID的报文发送
  * 3. 替代原有的周期发送方式
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can_trigger_send.h"
#include "can.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CAN_TRIGGER_MESSAGE1_ID   0x100   // 消息1 ID
#define CAN_TRIGGER_MESSAGE2_ID   0x200   // 消息2 ID
#define CAN_TRIGGER_MESSAGE3_ID   0x300   // 消息3 ID

/* Private variables ---------------------------------------------------------*/
static uint8_t uart_rx_char = 0;

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan1;
extern UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef CAN_TriggerSend_SendMessage(uint32_t message_id, uint8_t* data, uint8_t dlc);
static void CAN_TriggerSend_ProcessChar(uint8_t received_char);

/**
  * @brief  初始化触发式CAN发送模块
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_TriggerSend_Init(void)
{
    // 启动串口单字符中断接收
    HAL_StatusTypeDef uart_status = HAL_UART_Receive_IT(&huart2, &uart_rx_char, 1);
    
    if (uart_status != HAL_OK)
    {
        printf("[CAN-TRIGGER] Failed to start UART RX interrupt\r\n");
        return HAL_ERROR;
    }
    
    // printf("[CAN-TRIGGER] Init success! Send '1', '2', '3' to trigger CAN messages\r\n");  // 已删除
    
    return HAL_OK;
}

/**
  * @brief  触发式CAN发送任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_TriggerSend_Task(void *argument)
{
    while (1) {
        // 触发式发送任务，主要逻辑在串口中断回调中处理
        // 这里只需要保持任务运行
        osDelay(100);
    }
}

/**
  * @brief  发送CAN消息1 (ID: 0x100)
  * @param  None
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef CAN_TriggerSend_SendMessage1(void)
{
    uint8_t data[8] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return CAN_TriggerSend_SendMessage(CAN_TRIGGER_MESSAGE1_ID, data, 8);
}

/**
  * @brief  发送CAN消息2 (ID: 0x200)
  * @param  None
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef CAN_TriggerSend_SendMessage2(void)
{
    uint8_t data[8] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return CAN_TriggerSend_SendMessage(CAN_TRIGGER_MESSAGE2_ID, data, 8);
}

/**
  * @brief  发送CAN消息3 (ID: 0x300)
  * @param  None
  * @retval HAL_StatusTypeDef
  */
HAL_StatusTypeDef CAN_TriggerSend_SendMessage3(void)
{
    uint8_t data[8] = {0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return CAN_TriggerSend_SendMessage(CAN_TRIGGER_MESSAGE3_ID, data, 8);
}

/**
  * @brief  发送CAN消息的通用函数
  * @param  message_id: 消息ID
  * @param  data: 数据指针
  * @param  dlc: 数据长度
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_TriggerSend_SendMessage(uint32_t message_id, uint8_t* data, uint8_t dlc)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;
    
    // 配置发送头
    tx_header.StdId = message_id;
    tx_header.ExtId = 0x00;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = dlc;
    tx_header.TransmitGlobalTime = DISABLE;
    
    // 发送CAN消息
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &tx_header, data, &tx_mailbox);
    
    if (status == HAL_OK)
    {
        // 按照用户要求的格式打印日志
        printf("[CAN1-TX] ID:0x%03X, DLC:%d, Data:", message_id, dlc);
        for (int i = 0; i < dlc; i++)
        {
            printf("%02X", data[i]);
            if (i < dlc - 1) printf(" ");
        }
        printf("\r\n");
    }
    else
    {
        printf("[CAN1-ERROR] Failed to send message - ID:0x%03X, Error:%d\r\n", message_id, status);
    }
    
    return status;
}

/**
  * @brief  处理串口接收到的字符
  * @param  received_char: 接收到的字符
  * @retval None
  */
static void CAN_TriggerSend_ProcessChar(uint8_t received_char)
{
    switch (received_char)
    {
        case '1':
            CAN_TriggerSend_SendMessage1();
            break;
            
        case '2':
            CAN_TriggerSend_SendMessage2();
            break;
            
        case '3':
            CAN_TriggerSend_SendMessage3();
            break;
            
        default:
            // 忽略其他字符
            break;
    }
}



/**
  * @brief  串口接收完成回调函数
  * @param  huart: UART句柄
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // 处理接收到的字符
        CAN_TriggerSend_ProcessChar(uart_rx_char);
        
        // 重新启动单字符接收
        HAL_UART_Receive_IT(&huart2, &uart_rx_char, 1);
    }
}