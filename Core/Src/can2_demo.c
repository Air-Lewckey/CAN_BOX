/**
  ******************************************************************************
  * @file    can2_demo.c
  * @brief   CAN2静默监听模块实现文件
  *          提供CAN2的初始化、消息接收和统计功能（纯监听模式）
  ******************************************************************************
  * @attention
  *
  * STM32F407ZGT6 CAN2静默监听演示
  * 模式：CAN_MODE_SILENT（只接收，不发送任何信号包括ACK）
  * 波特率：500Kbps
  * 引脚：PB12(CAN2_RX), PB13(CAN2_TX - 未使用)
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can2_demo.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* 私有变量 */
static CAN2_Demo_Stats_t can2_stats = {0};
static uint32_t message_counter = 0;
static uint32_t last_heartbeat_time = 0;
static uint32_t last_data_time = 0;
static uint32_t last_status_time = 0;
static uint32_t interrupt_counter = 0;  // 中断计数器，用于调试

/* CAN2过滤器配置 */
static CAN_FilterTypeDef can2_filter_config;

/**
  * @brief  CAN2演示模块初始化
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN2_Demo_Init(void)
{
    HAL_StatusTypeDef status;
    
    printf("[CAN2-INIT] Starting CAN2 Demo initialization...\r\n");
    
    // 重要：必须先配置CAN1的过滤器，因为CAN2依赖于CAN1
    // CAN2过滤器配置 - 接收所有消息（修复版本）
    can2_filter_config.FilterBank = 14;  // CAN2使用过滤器组14-27
    can2_filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
    can2_filter_config.FilterScale = CAN_FILTERSCALE_32BIT;
    can2_filter_config.FilterIdHigh = 0x0000;
    can2_filter_config.FilterIdLow = 0x0000;
    can2_filter_config.FilterMaskIdHigh = 0x0000;  // 掩码为0，接收所有ID
    can2_filter_config.FilterMaskIdLow = 0x0000;
    can2_filter_config.FilterFIFOAssignment = CAN_RX_FIFO0;
    can2_filter_config.FilterActivation = ENABLE;
    can2_filter_config.SlaveStartFilterBank = 14;  // 关键：CAN2从过滤器组14开始
    
    // 临时启用调试信息以验证过滤器配置
    
    // 重要：必须使用CAN1句柄来配置CAN2过滤器（STM32硬件特性）
    status = HAL_CAN_ConfigFilter(&hcan1, &can2_filter_config);
    if (status != HAL_OK) {
        printf("[CAN2-ERROR] Filter configuration failed: %d\r\n", status);
        return status;
    }

    
    // 启动CAN2
    status = HAL_CAN_Start(&hcan2);
    if (status != HAL_OK) {
        printf("[CAN2-ERROR] CAN2 start failed: %d\r\n", status);
        return status;
    }

    
    // 激活CAN2接收中断
    status = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK) {
        printf("[CAN2-ERROR] RX interrupt activation failed: %d\r\n", status);
        return status;
    }

    
    // 检查NVIC中断是否启用
    if (NVIC_GetEnableIRQ(CAN2_RX0_IRQn)) {
        // NVIC CAN2_RX0 interrupt is enabled
    } else {
        printf("[CAN2-ERROR] NVIC CAN2_RX0 interrupt is NOT enabled\r\n");
    }
    
    // 检查CAN2寄存器状态
    
    // 初始化统计信息
    memset(&can2_stats, 0, sizeof(can2_stats));
    can2_stats.initialized = 1;
    
    printf("[CAN2-INIT] CAN2 Demo initialized successfully\r\n");
    printf("[CAN2-INIT] Filter Bank: %d, FIFO: %d\r\n", 
           can2_filter_config.FilterBank, can2_filter_config.FilterFIFOAssignment);
    printf("[CAN2-INIT] Bitrate: 500Kbps, Sample Point: 78.6%%\r\n");
    printf("[CAN2-INIT] Prescaler: 6, TimeSeg1: 10TQ, TimeSeg2: 3TQ\r\n");
    printf("[CAN2-INIT] Waiting for CAN messages from CANOE...\r\n");
    
    // 打印CAN2状态信息
    uint32_t can_state = HAL_CAN_GetState(&hcan2);
    
    return HAL_OK;
}

/**
  * @brief  CAN2纯监听任务函数 - 只接收不发送
  * @param  argument: 任务参数
  * @retval None
  */
void CAN2_Demo_Task(void *argument)
{
    uint32_t current_time;
    

    
    for(;;) {
        current_time = HAL_GetTick();
        
        // 静默运行，只在接收到数据时打印
        // 移除所有状态报告和诊断信息，专注于数据接收
        
        // 任务延时
        osDelay(100);  // 100ms
    }
}

/**
  * @brief  获取CAN2统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN2_Demo_GetStats(CAN2_Demo_Stats_t *stats)
{
    if (stats != NULL) {
        memcpy(stats, &can2_stats, sizeof(CAN2_Demo_Stats_t));
    }
}

/**
  * @brief  CAN2发送消息（静默模式下禁用）
  * @param  id: CAN消息ID
  * @param  data: 数据指针
  * @param  length: 数据长度
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN2_Demo_SendMessage(uint32_t id, uint8_t *data, uint8_t length)
{
    // CAN2配置为静默监听模式，禁止发送任何消息
    printf("[CAN2-SILENT] Send request blocked - CAN2 in silent mode\r\n");
    can2_stats.send_errors++;  // 记录为发送错误
    return HAL_ERROR;  // 返回错误状态
    
    // 以下代码在静默模式下被禁用
    /*
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;
    HAL_StatusTypeDef status;
    
    // 配置发送头
    tx_header.StdId = id;
    tx_header.ExtId = 0;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.IDE = CAN_ID_STD;
    tx_header.DLC = length;
    tx_header.TransmitGlobalTime = DISABLE;
    
    // 发送消息
    status = HAL_CAN_AddTxMessage(&hcan2, &tx_header, data, &tx_mailbox);
    
    if (status == HAL_OK) {
        can2_stats.total_sent++;
        can2_stats.last_tx_time = HAL_GetTick();
        // printf("[CAN2-TX] ID:0x%03lX, DLC:%d, Data:", id, length);
        // for (int i = 0; i < length; i++) {
        //     printf(" %02X", data[i]);
        // }
        // printf("\r\n");
    } else {
        can2_stats.send_errors++;
        uint32_t error_code = HAL_CAN_GetError(&hcan2);
        uint32_t can_state = HAL_CAN_GetState(&hcan2);
        // printf("[CAN2-ERROR] Send failed, ID:0x%03lX, Status:%d\r\n", id, status);
        // printf("[CAN2-ERROR] CAN2 State: %lu, Error Code: 0x%08lX\r\n", can_state, error_code);
        
        // 详细错误分析
        // if (error_code & HAL_CAN_ERROR_EWG) {
        //     printf("[CAN2-ERROR] Error Warning Flag\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_EPV) {
        //     printf("[CAN2-ERROR] Error Passive Flag\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_BOF) {
        //     printf("[CAN2-ERROR] Bus-Off Flag\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_STF) {
        //     printf("[CAN2-ERROR] Stuff Error\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_FOR) {
        //     printf("[CAN2-ERROR] Form Error\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_ACK) {
        //     printf("[CAN2-ERROR] ACK Error (No acknowledgment)\r\n");
        //     printf("[CAN2-INFO] This usually means no other CAN node is connected\r\n");
        //     printf("[CAN2-INFO] Check CANOE connection and bitrate configuration\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_BR) {
        //     printf("[CAN2-ERROR] Bit Recessive Error\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_BD) {
        //     printf("[CAN2-ERROR] Bit Dominant Error\r\n");
        // }
        // if (error_code & HAL_CAN_ERROR_CRC) {
        //     printf("[CAN2-ERROR] CRC Error\r\n");
        // }
    }
    
    return status;
    */
}

/**
  * @brief  处理CAN2接收到的消息
  * @param  rx_header: 接收头指针
  * @param  rx_data: 接收数据指针
  * @retval None
  */
void CAN2_Demo_ProcessReceivedMessage(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data)
{
    // 增加中断计数器
    interrupt_counter++;
    
    // 更新统计信息
    can2_stats.total_received++;
    can2_stats.last_rx_time = HAL_GetTick();
    
    // 打印详细的接收信息
    printf("[CAN2-RX #%lu] ID:0x%03lX DLC:%lu Data:", interrupt_counter, rx_header->StdId, rx_header->DLC);
    for (int i = 0; i < rx_header->DLC; i++) {
        printf(" %02X", rx_data[i]);
    }
    printf(" (Time:%lu)\r\n", HAL_GetTick());
}

/**
  * @brief  运行CAN2诊断测试（静默模式）
  * @retval None
  */
void CAN2_Demo_RunDiagnostic(void)
{
    printf("[CAN2-DIAG] === CAN2 Silent Mode Diagnostic ===\r\n");
    printf("[CAN2-DIAG] CAN2 State: %lu\r\n", HAL_CAN_GetState(&hcan2));
    printf("[CAN2-DIAG] Error Code: 0x%08lX\r\n", HAL_CAN_GetError(&hcan2));
    printf("[CAN2-DIAG] Total Received: %lu\r\n", can2_stats.total_received);
    printf("[CAN2-DIAG] Interrupt Counter: %lu\r\n", interrupt_counter);
    printf("[CAN2-DIAG] Last RX Time: %lu ms\r\n", can2_stats.last_rx_time);
    
    // Detailed troubleshooting guide
    printf("\r\n[CAN2-TROUBLESHOOT] === Troubleshooting Guide ===\r\n");
    printf("[CAN2-TROUBLESHOOT] If receive count is 0, please check:\r\n");
    printf("[CAN2-TROUBLESHOOT] 1. Hardware Connection:\r\n");
    printf("[CAN2-TROUBLESHOOT]    - STM32 PB12(CAN2_RX) connect to CANOE CAN_H\r\n");
    printf("[CAN2-TROUBLESHOOT]    - STM32 GND connect to CANOE GND\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Ensure CAN_L is NOT connected (single-wire CAN)\r\n");
    printf("[CAN2-TROUBLESHOOT] 2. CANOE Configuration:\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Bitrate set to 500Kbps\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Message ID set to 0x201\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Ensure CANOE is online\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Check CANOE send cycle is correct (200ms)\r\n");
    printf("[CAN2-TROUBLESHOOT] 3. Electrical Characteristics:\r\n");
    printf("[CAN2-TROUBLESHOOT]    - CAN_H level: 2.5V idle, 3.5V dominant\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Use oscilloscope to check PB12 pin signal\r\n");
    printf("[CAN2-TROUBLESHOOT] 4. Software Configuration:\r\n");
    printf("[CAN2-TROUBLESHOOT]    - CAN2 configured in silent mode (receive only)\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Filter configured to receive all messages\r\n");
    printf("[CAN2-TROUBLESHOOT]    - Interrupt correctly configured and enabled\r\n");
    printf("[CAN2-DIAG] === Silent Mode - No Transmission ===\r\n");
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/