/**
  ******************************************************************************
  * @file    can1_can2_bridge_test.c
  * @brief   CAN1和CAN2总线桥接测试模块
  *          验证CAN1为CAN2提供ACK应答的功能
  ******************************************************************************
  * @attention
  *
  * 使用场景：
  * - CAN1和CAN2的总线已经物理连接
  * - CAN1作为应答节点，为CAN2的消息提供ACK
  * - 验证两个CAN控制器在同一总线上的协同工作
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
static uint32_t bridge_test_can1_rx_count = 0;
static uint32_t bridge_test_can2_tx_count = 0;
static uint32_t bridge_test_can2_tx_success = 0;
static uint32_t bridge_test_can2_tx_errors = 0;

/* 函数声明 */
HAL_StatusTypeDef CAN1_CAN2_BridgeTest_Init(void);
void CAN1_CAN2_BridgeTest_Task(void *argument);
HAL_StatusTypeDef CAN1_CAN2_BridgeTest_SendFromCAN2(uint32_t id, uint8_t *data, uint8_t length);
void CAN1_CAN2_BridgeTest_ProcessCAN1Reception(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);
void CAN1_CAN2_BridgeTest_PrintStats(void);

/**
  * @brief  CAN1-CAN2桥接测试初始化
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN1_CAN2_BridgeTest_Init(void)
{
    HAL_StatusTypeDef status;
    CAN_FilterTypeDef filter_config;
    
    printf("[BRIDGE-TEST] Initializing CAN1-CAN2 bridge test...\r\n");
    
    // 确保CAN1和CAN2都是正常模式
    printf("[BRIDGE-TEST] Configuring CAN1 and CAN2 for normal mode...\r\n");
    
    // 停止CAN1和CAN2（如果已启动）
    HAL_CAN_Stop(&hcan1);
    HAL_CAN_Stop(&hcan2);
    
    // 重新配置CAN1为正常模式
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    status = HAL_CAN_Init(&hcan1);
    if (status != HAL_OK) {
        printf("[BRIDGE-TEST-ERROR] CAN1 init failed: %d\r\n", status);
        return status;
    }
    
    // 重新配置CAN2为正常模式
    hcan2.Init.Mode = CAN_MODE_NORMAL;
    status = HAL_CAN_Init(&hcan2);
    if (status != HAL_OK) {
        printf("[BRIDGE-TEST-ERROR] CAN2 init failed: %d\r\n", status);
        return status;
    }
    
    // 配置CAN1过滤器 - 接收所有消息（特别是CAN2发送的消息）
    filter_config.FilterBank = 0;
    filter_config.FilterMode = CAN_FILTERMODE_IDMASK;
    filter_config.FilterScale = CAN_FILTERSCALE_32BIT;
    filter_config.FilterIdHigh = 0x0000;
    filter_config.FilterIdLow = 0x0000;
    filter_config.FilterMaskIdHigh = 0x0000;  // 掩码为0，接收所有ID
    filter_config.FilterMaskIdLow = 0x0000;
    filter_config.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter_config.FilterActivation = ENABLE;
    filter_config.SlaveStartFilterBank = 14;
    
    status = HAL_CAN_ConfigFilter(&hcan1, &filter_config);
    if (status != HAL_OK) {
        printf("[BRIDGE-TEST-ERROR] CAN1 filter config failed: %d\r\n", status);
        return status;
    }
    
    // 配置CAN2过滤器 - 接收所有消息
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
        printf("[BRIDGE-TEST-ERROR] CAN2 filter config failed: %d\r\n", status);
        return status;
    }
    
    // 启动CAN1
    status = HAL_CAN_Start(&hcan1);
    if (status != HAL_OK) {
        printf("[BRIDGE-TEST-ERROR] CAN1 start failed: %d\r\n", status);
        return status;
    }
    
    // 启动CAN2
    status = HAL_CAN_Start(&hcan2);
    if (status != HAL_OK) {
        printf("[BRIDGE-TEST-ERROR] CAN2 start failed: %d\r\n", status);
        return status;
    }
    
    // 激活CAN1接收中断
    status = HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK) {
        printf("[BRIDGE-TEST-ERROR] CAN1 RX interrupt activation failed: %d\r\n", status);
        return status;
    }
    
    // 激活CAN2接收中断
    status = HAL_CAN_ActivateNotification(&hcan2, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (status != HAL_OK) {
        printf("[BRIDGE-TEST-ERROR] CAN2 RX interrupt activation failed: %d\r\n", status);
        return status;
    }
    
    // 重置统计信息
    bridge_test_can1_rx_count = 0;
    bridge_test_can2_tx_count = 0;
    bridge_test_can2_tx_success = 0;
    bridge_test_can2_tx_errors = 0;
    
    printf("[BRIDGE-TEST] CAN1-CAN2 bridge test initialized successfully\r\n");
    printf("[BRIDGE-TEST] CAN1 will provide ACK for CAN2 messages\r\n");
    printf("[BRIDGE-TEST] Both CAN controllers on same physical bus\r\n");
    
    return HAL_OK;
}

/**
  * @brief  CAN1-CAN2桥接测试任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN1_CAN2_BridgeTest_Task(void *argument)
{
    uint8_t test_data[8];
    uint32_t test_counter = 0;
    
    printf("[BRIDGE-TEST] Bridge test task started\r\n");
    printf("[BRIDGE-TEST] CAN2 will send messages, CAN1 will provide ACK\r\n");
    
    for(;;) {
        // 准备测试数据
        test_data[0] = 0xBB;  // 桥接测试标识
        test_data[1] = 0xCC;  // 桥接测试标识
        test_data[2] = (uint8_t)(test_counter >> 8);
        test_data[3] = (uint8_t)(test_counter);
        test_data[4] = (uint8_t)(HAL_GetTick() >> 24);
        test_data[5] = (uint8_t)(HAL_GetTick() >> 16);
        test_data[6] = (uint8_t)(HAL_GetTick() >> 8);
        test_data[7] = (uint8_t)(HAL_GetTick());
        
        // 从CAN2发送测试消息（期望CAN1提供ACK）
        uint32_t test_id = 0x300 + (test_counter % 10);  // ID范围：0x300-0x309
        
        if (CAN1_CAN2_BridgeTest_SendFromCAN2(test_id, test_data, 8) == HAL_OK) {
            bridge_test_can2_tx_success++;
            printf("[BRIDGE-TEST-CAN2-TX] Message #%lu sent successfully (ID:0x%03lX)\r\n", 
                   test_counter, test_id);
        } else {
            bridge_test_can2_tx_errors++;
            printf("[BRIDGE-TEST-CAN2-ERROR] Message #%lu send failed (ID:0x%03lX)\r\n", 
                   test_counter, test_id);
        }
        
        test_counter++;
        
        // 每5条消息打印统计信息
        if (test_counter % 5 == 0) {
            CAN1_CAN2_BridgeTest_PrintStats();
        }
        
        // 延时2秒
        osDelay(2000);
    }
}

/**
  * @brief  从CAN2发送桥接测试消息
  * @param  id: 消息ID
  * @param  data: 数据指针
  * @param  length: 数据长度
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN1_CAN2_BridgeTest_SendFromCAN2(uint32_t id, uint8_t *data, uint8_t length)
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
    
    // 从CAN2发送消息
    status = HAL_CAN_AddTxMessage(&hcan2, &tx_header, data, &tx_mailbox);
    
    bridge_test_can2_tx_count++;
    
    if (status != HAL_OK) {
        uint32_t error_code = HAL_CAN_GetError(&hcan2);
        printf("[BRIDGE-TEST-CAN2-ERROR] Send failed, Error: 0x%08lX\r\n", error_code);
    }
    
    return status;
}

/**
  * @brief  处理CAN1接收到的桥接测试消息
  * @param  rx_header: 接收头指针
  * @param  rx_data: 接收数据指针
  * @retval None
  */
void CAN1_CAN2_BridgeTest_ProcessCAN1Reception(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data)
{
    // 检查是否是桥接测试消息
    if (rx_header->DLC >= 2 && rx_data[0] == 0xBB && rx_data[1] == 0xCC) {
        bridge_test_can1_rx_count++;
        
        printf("[BRIDGE-TEST-CAN1-RX] Received bridge test message ID:0x%03lX, DLC:%lu, Data:", 
               rx_header->StdId, rx_header->DLC);
        for (int i = 0; i < rx_header->DLC; i++) {
            printf(" %02X", rx_data[i]);
        }
        printf("\r\n");
        
        printf("[BRIDGE-TEST-CAN1] ACK automatically provided by hardware\r\n");
        
        // 验证数据完整性
        if (rx_header->DLC >= 4) {
            uint16_t counter = (rx_data[2] << 8) | rx_data[3];
            printf("[BRIDGE-TEST-CAN1] Message counter: %u\r\n", counter);
        }
    }
}

/**
  * @brief  打印CAN1-CAN2桥接测试统计信息
  * @retval None
  */
void CAN1_CAN2_BridgeTest_PrintStats(void)
{
    printf("\r\n[BRIDGE-TEST-STATS] ==================\r\n");
    printf("[BRIDGE-TEST-STATS] CAN2 TX Total: %lu\r\n", bridge_test_can2_tx_count);
    printf("[BRIDGE-TEST-STATS] CAN2 TX Success: %lu\r\n", bridge_test_can2_tx_success);
    printf("[BRIDGE-TEST-STATS] CAN2 TX Errors: %lu\r\n", bridge_test_can2_tx_errors);
    printf("[BRIDGE-TEST-STATS] CAN1 RX Count: %lu\r\n", bridge_test_can1_rx_count);
    
    if (bridge_test_can2_tx_count > 0) {
        float success_rate = (float)bridge_test_can2_tx_success * 100.0f / bridge_test_can2_tx_count;
        printf("[BRIDGE-TEST-STATS] CAN2 Success Rate: %.1f%%\r\n", success_rate);
    }
    
    if (bridge_test_can2_tx_success > 0) {
        float ack_rate = (float)bridge_test_can1_rx_count * 100.0f / bridge_test_can2_tx_success;
        printf("[BRIDGE-TEST-STATS] CAN1 ACK Rate: %.1f%%\r\n", ack_rate);
    }
    
    printf("[BRIDGE-TEST-STATS] ==================\r\n\r\n");
}

/**
  * @brief  获取桥接测试统计信息
  * @param  can2_tx: CAN2发送计数指针
  * @param  can2_success: CAN2成功计数指针
  * @param  can1_rx: CAN1接收计数指针
  * @retval None
  */
void CAN1_CAN2_BridgeTest_GetStats(uint32_t *can2_tx, uint32_t *can2_success, uint32_t *can1_rx)
{
    if (can2_tx) *can2_tx = bridge_test_can2_tx_count;
    if (can2_success) *can2_success = bridge_test_can2_tx_success;
    if (can1_rx) *can1_rx = bridge_test_can1_rx_count;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/