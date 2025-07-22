/**
  ******************************************************************************
  * @file    can2_loopback_test.c
  * @brief   CAN2环回测试模块 - 用于验证CAN2基本功能
  *          当没有外部CAN节点时，可以使用此模块进行自测试
  ******************************************************************************
  * @attention
  *
  * 环回模式说明：
  * - CAN控制器内部将发送的消息直接路由到接收FIFO
  * - 不需要外部CAN总线连接
  * - 可以验证CAN控制器的发送和接收功能
  * - 适用于硬件调试和功能验证
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can.h"
#include "usart.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* 私有变量 */
static uint32_t loopback_tx_count = 0;
static uint32_t loopback_rx_count = 0;
static uint32_t loopback_errors = 0;

/* 函数声明 */
HAL_StatusTypeDef CAN2_LoopbackTest_Init(void);
void CAN2_LoopbackTest_Task(void *argument);
HAL_StatusTypeDef CAN2_LoopbackTest_SendMessage(uint32_t id, uint8_t *data, uint8_t length);
void CAN2_LoopbackTest_PrintStats(void);

/**
  * @brief  CAN2环回测试初始化
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN2_LoopbackTest_Init(void)
{
    HAL_StatusTypeDef status;
    CAN_FilterTypeDef filter_config;
    
    printf("[CAN2-LOOPBACK] Initializing CAN2 loopback test...\r\n");
    
    // 停止CAN2（如果已启动）
    HAL_CAN_Stop(&hcan2);
    
    // 重新配置CAN2为环回模式
    hcan2.Init.Mode = CAN_MODE_LOOPBACK;
    status = HAL_CAN_Init(&hcan2);
    if (status != HAL_OK) {
        printf("[CAN2-LOOPBACK-ERROR] CAN2 init failed: %d\r\n", status);
        return status;
    }
    
    // 配置过滤器
    filter_config.FilterBank = 14;
    filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
    filter_config.FilterScale = CAN_FILTERSCALE_32BIT;
    filter_config.FilterIdHigh = 0x0000;
    filter_config.FilterIdLow = 0x0000;
    filter_config.FilterMaskIdHigh = 0x0000;
    filter_config.FilterMaskIdLow = 0x0000;
    filter_config.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter_config.FilterActivation = ENABLE;
    filter_config.SlaveStartFilterBank = 14;
    
    status = HAL_CAN_ConfigFilter(&hcan2, &filter_config);
    if (status != HAL_OK) {
        printf("[CAN2-LOOPBACK-ERROR] Filter config failed: %d\r\n", status);
        return status;
    }
    
    // 启动CAN2
    status = HAL_CAN_Start(&hcan2);
    if (status != HAL_OK) {
        printf("[CAN2-LOOPBACK-ERROR] CAN2 start failed: %d\r\n", status);
        return status;
    }
    
    // 激活接收中断
    status = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK) {
        printf("[CAN2-LOOPBACK-ERROR] RX interrupt activation failed: %d\r\n", status);
        return status;
    }
    
    // 重置统计信息
    loopback_tx_count = 0;
    loopback_rx_count = 0;
    loopback_errors = 0;
    
    printf("[CAN2-LOOPBACK] CAN2 loopback test initialized successfully\r\n");
    printf("[CAN2-LOOPBACK] Mode: Internal loopback, no external bus required\r\n");
    
    return HAL_OK;
}

/**
  * @brief  CAN2环回测试任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN2_LoopbackTest_Task(void *argument)
{
    uint8_t test_data[8];
    uint32_t test_counter = 0;
    
    printf("[CAN2-LOOPBACK] Loopback test task started\r\n");
    
    for(;;) {
        // 准备测试数据
        test_data[0] = 0xAA;  // 固定标识
        test_data[1] = 0x55;  // 固定标识
        test_data[2] = (uint8_t)(test_counter >> 8);
        test_data[3] = (uint8_t)(test_counter);
        test_data[4] = (uint8_t)(HAL_GetTick() >> 24);
        test_data[5] = (uint8_t)(HAL_GetTick() >> 16);
        test_data[6] = (uint8_t)(HAL_GetTick() >> 8);
        test_data[7] = (uint8_t)(HAL_GetTick());
        
        // 发送测试消息
        if (CAN2_LoopbackTest_SendMessage(0x123, test_data, 8) == HAL_OK) {
            printf("[CAN2-LOOPBACK-TX] Test message #%lu sent\r\n", test_counter);
        }
        
        test_counter++;
        
        // 每5秒打印统计信息
        if (test_counter % 5 == 0) {
            CAN2_LoopbackTest_PrintStats();
        }
        
        // 延时1秒
        osDelay(1000);
    }
}

/**
  * @brief  CAN2环回测试发送消息
  * @param  id: 消息ID
  * @param  data: 数据指针
  * @param  length: 数据长度
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN2_LoopbackTest_SendMessage(uint32_t id, uint8_t *data, uint8_t length)
{
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
        loopback_tx_count++;
    } else {
        loopback_errors++;
        printf("[CAN2-LOOPBACK-ERROR] Send failed, Status: %d\r\n", status);
    }
    
    return status;
}

/**
  * @brief  处理CAN2环回测试接收消息
  * @param  rx_header: 接收头指针
  * @param  rx_data: 接收数据指针
  * @retval None
  */
void CAN2_LoopbackTest_ProcessMessage(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data)
{
    loopback_rx_count++;
    
    printf("[CAN2-LOOPBACK-RX] ID:0x%03lX, DLC:%lu, Data:", 
           rx_header->StdId, rx_header->DLC);
    for (int i = 0; i < rx_header->DLC; i++) {
        printf(" %02X", rx_data[i]);
    }
    printf("\r\n");
    
    // 验证数据完整性
    if (rx_header->DLC >= 2 && rx_data[0] == 0xAA && rx_data[1] == 0x55) {
        printf("[CAN2-LOOPBACK] Message integrity verified\r\n");
    } else {
        printf("[CAN2-LOOPBACK-ERROR] Message integrity check failed\r\n");
        loopback_errors++;
    }
}

/**
  * @brief  打印CAN2环回测试统计信息
  * @retval None
  */
void CAN2_LoopbackTest_PrintStats(void)
{
    printf("\r\n[CAN2-LOOPBACK-STATS] ==================\r\n");
    printf("[CAN2-LOOPBACK-STATS] TX Count: %lu\r\n", loopback_tx_count);
    printf("[CAN2-LOOPBACK-STATS] RX Count: %lu\r\n", loopback_rx_count);
    printf("[CAN2-LOOPBACK-STATS] Errors: %lu\r\n", loopback_errors);
    printf("[CAN2-LOOPBACK-STATS] Success Rate: %.1f%%\r\n", 
           loopback_tx_count > 0 ? (float)loopback_rx_count * 100.0f / loopback_tx_count : 0.0f);
    printf("[CAN2-LOOPBACK-STATS] ==================\r\n\r\n");
}

/**
  * @brief  恢复CAN2正常模式
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN2_LoopbackTest_RestoreNormalMode(void)
{
    HAL_StatusTypeDef status;
    
    printf("[CAN2-LOOPBACK] Restoring CAN2 to normal mode...\r\n");
    
    // 停止CAN2
    HAL_CAN_Stop(&hcan2);
    
    // 恢复正常模式
    hcan2.Init.Mode = CAN_MODE_NORMAL;
    status = HAL_CAN_Init(&hcan2);
    if (status != HAL_OK) {
        printf("[CAN2-LOOPBACK-ERROR] Failed to restore normal mode: %d\r\n", status);
        return status;
    }
    
    printf("[CAN2-LOOPBACK] CAN2 restored to normal mode\r\n");
    return HAL_OK;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/