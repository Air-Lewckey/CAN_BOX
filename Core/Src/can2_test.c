/**
 * @file can2_test.c
 * @brief CAN2增强测试功能实现
 * @author AI Assistant
 * @date 2024
 */

#include "can2_test.h"
#include "can2_demo.h"
#include "main.h"
#include "can.h"
#include <stdio.h>
#include <string.h>

// 测试状态变量
static uint32_t test_counter = 0;
static uint32_t burst_count = 0;

/**
 * @brief CAN2测试初始化
 * @return HAL状态
 */
HAL_StatusTypeDef CAN2_Test_Init(void)
{
    printf("CAN2 Enhanced Test Module Initialized\r\n");
    printf("Available test modes:\r\n");
    printf("  - Auto Mode: Continuous message sending\r\n");
    printf("  - Manual Mode: On-demand message sending\r\n");
    printf("  - Burst Mode: High frequency message burst\r\n");
    test_counter = 0;
    burst_count = 0;
    return HAL_OK;
}

/**
 * @brief CAN2快速测试 - 发送一组测试消息
 */
void CAN2_Test_QuickTest(void)
{
    uint8_t tx_data[8];
    uint32_t current_time = HAL_GetTick();
    
    printf("CAN2 Quick Test Started at %lu ms\r\n", current_time);
    
    // 测试消息1：快速心跳
    tx_data[0] = 0xCA;
    tx_data[1] = 0x02;
    tx_data[2] = 0xFF;  // 快速测试标识
    tx_data[3] = 0x01;  // 测试序号
    tx_data[4] = (uint8_t)(current_time >> 8);
    tx_data[5] = (uint8_t)(current_time);
    tx_data[6] = 0x12;
    tx_data[7] = 0x34;
    
    if (CAN2_Demo_SendMessage(CAN2_HEARTBEAT_ID, tx_data, 8) == HAL_OK) {
        printf("CAN2 Quick Heartbeat sent successfully\r\n");
    }
    
    HAL_Delay(50);  // 短暂延时
    
    // 测试消息2：数据包
    tx_data[0] = 0xDA;
    tx_data[1] = 0x02;
    tx_data[2] = 0xFF;  // 快速测试标识
    tx_data[3] = 0x02;  // 测试序号
    tx_data[4] = (uint8_t)(test_counter >> 8);
    tx_data[5] = (uint8_t)(test_counter);
    tx_data[6] = 0xAB;
    tx_data[7] = 0xCD;
    
    if (CAN2_Demo_SendMessage(CAN2_DATA_ID, tx_data, 8) == HAL_OK) {
        printf("CAN2 Quick Data sent successfully\r\n");
    }
    
    HAL_Delay(50);
    
    // 测试消息3：控制命令
    tx_data[0] = 0x43;
    tx_data[1] = 0x02;
    tx_data[2] = 0xFF;  // 快速测试标识
    tx_data[3] = 0x03;  // 测试序号
    tx_data[4] = 0x01;  // 启动命令
    tx_data[5] = 0x00;
    tx_data[6] = 0xEF;
    tx_data[7] = 0x12;
    
    if (CAN2_Demo_SendMessage(CAN2_CONTROL_ID, tx_data, 8) == HAL_OK) {
        printf("CAN2 Quick Control sent successfully\r\n");
    }
    
    test_counter++;
    printf("CAN2 Quick Test Completed (Test #%lu)\r\n", test_counter);
}

/**
 * @brief CAN2突发测试 - 高频率发送消息
 * @param count 发送消息数量
 */
void CAN2_Test_BurstMode(uint16_t count)
{
    uint8_t tx_data[8];
    uint32_t start_time = HAL_GetTick();
    uint16_t success_count = 0;
    
    printf("CAN2 Burst Test Started: %d messages\r\n", count);
    
    for (uint16_t i = 0; i < count; i++) {
        // 构造突发测试消息
        tx_data[0] = 0x42;  // Burst标识 ('B')
        tx_data[1] = 0x02;  // CAN2编号
        tx_data[2] = (uint8_t)(i >> 8);     // 消息序号高位
        tx_data[3] = (uint8_t)(i);          // 消息序号低位
        tx_data[4] = (uint8_t)(burst_count >> 8);
        tx_data[5] = (uint8_t)(burst_count);
        tx_data[6] = 0x55 + (i & 0x0F);     // 变化数据
        tx_data[7] = 0xAA - (i & 0x0F);     // 变化数据
        
        if (CAN2_Demo_SendMessage(CAN2_TEST_ID, tx_data, 8) == HAL_OK) {
            success_count++;
        }
        
        // 短暂延时避免总线拥塞
        HAL_Delay(10);
    }
    
    uint32_t end_time = HAL_GetTick();
    burst_count++;
    
    printf("CAN2 Burst Test Completed:\r\n");
    printf("  - Duration: %lu ms\r\n", end_time - start_time);
    printf("  - Success: %d/%d messages\r\n", success_count, count);
    printf("  - Success Rate: %.1f%%\r\n", (float)success_count * 100.0f / count);
}

/**
 * @brief CAN2配置测试 - 发送配置消息
 * @param config_type 配置类型
 * @param value 配置值
 */
void CAN2_Test_SendConfig(uint8_t config_type, uint32_t value)
{
    uint8_t tx_data[8];
    
    tx_data[0] = 0x43;  // Config标识 ('C')
    tx_data[1] = 0x02;  // CAN2编号
    tx_data[2] = config_type;  // 配置类型
    tx_data[3] = 0x00;  // 保留
    tx_data[4] = (uint8_t)(value >> 24);
    tx_data[5] = (uint8_t)(value >> 16);
    tx_data[6] = (uint8_t)(value >> 8);
    tx_data[7] = (uint8_t)(value);
    
    if (CAN2_Demo_SendMessage(CAN2_CONFIG_ID, tx_data, 8) == HAL_OK) {
        printf("CAN2 Config sent: Type=0x%02X, Value=0x%08lX\r\n", config_type, value);
    } else {
        printf("CAN2 Config send failed\r\n");
    }
}

/**
 * @brief CAN2错误模拟测试
 */
void CAN2_Test_ErrorSimulation(void)
{
    uint8_t tx_data[8];
    uint32_t current_time = HAL_GetTick();
    
    printf("CAN2 Error Simulation Test\r\n");
    
    // 模拟错误消息
    tx_data[0] = 0x45;  // Error标识 ('E')
    tx_data[1] = 0x02;  // CAN2编号
    tx_data[2] = 0x01;  // 错误类型：通信错误
    tx_data[3] = 0x00;  // 错误级别：警告
    tx_data[4] = (uint8_t)(current_time >> 24);
    tx_data[5] = (uint8_t)(current_time >> 16);
    tx_data[6] = (uint8_t)(current_time >> 8);
    tx_data[7] = (uint8_t)(current_time);
    
    if (CAN2_Demo_SendMessage(CAN2_ERROR_ID, tx_data, 8) == HAL_OK) {
        printf("CAN2 Error simulation message sent\r\n");
    }
}

/**
 * @brief CAN2测试任务（主任务函数）
 */
void CAN2_Test_Task(void)
{
    CAN2_Test_ComprehensiveTask();
}

/**
 * @brief CAN2综合测试任务
 */
void CAN2_Test_ComprehensiveTask(void)
{
    static uint32_t last_test_time = 0;
    static uint8_t test_phase = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每10秒执行一次综合测试
    if (current_time - last_test_time >= 10000) {
        switch (test_phase) {
            case 0:
                printf("=== CAN2 Comprehensive Test Phase 1: Quick Test ===\r\n");
                CAN2_Test_QuickTest();
                break;
                
            case 1:
                printf("=== CAN2 Comprehensive Test Phase 2: Config Test ===\r\n");
                CAN2_Test_SendConfig(0x01, 0x12345678);
                HAL_Delay(100);
                CAN2_Test_SendConfig(0x02, 0xABCDEF00);
                break;
                
            case 2:
                printf("=== CAN2 Comprehensive Test Phase 3: Burst Test ===\r\n");
                CAN2_Test_BurstMode(5);  // 发送5条突发消息
                break;
                
            case 3:
                printf("=== CAN2 Comprehensive Test Phase 4: Error Simulation ===\r\n");
                CAN2_Test_ErrorSimulation();
                break;
                
            default:
                test_phase = 0;
                printf("=== CAN2 Comprehensive Test Cycle Completed ===\r\n");
                break;
        }
        
        test_phase++;
        if (test_phase > 4) {
            test_phase = 0;
        }
        
        last_test_time = current_time;
    }
}

/**
 * @brief 获取CAN2测试统计信息
 */
void CAN2_Test_PrintStats(void)
{
    CAN2_Demo_Stats_t stats;
    CAN2_Demo_GetStats(&stats);  // 修正函数调用方式
    
    printf("\r\n=== CAN2 Enhanced Test Statistics ===\r\n");
    printf("Test Counter: %lu\r\n", test_counter);
    printf("Burst Test Count: %lu\r\n", burst_count);
    printf("Total Messages Sent: %lu\r\n", stats.total_sent);
    printf("Total Messages Received: %lu\r\n", stats.total_received);
    printf("Heartbeat Count: %lu\r\n", stats.heartbeat_count);
    printf("Data Count: %lu\r\n", stats.data_count);
    printf("Status Count: %lu\r\n", stats.status_count);
    printf("Control Count: %lu\r\n", stats.control_count);
    printf("Debug Count: %lu\r\n", stats.debug_count);
    printf("Test Count: %lu\r\n", stats.test_count);
    printf("Config Count: %lu\r\n", stats.config_count);
    printf("Error Count: %lu\r\n", stats.error_count);
    printf("Send Errors: %lu\r\n", stats.send_errors);
    printf("Receive Errors: %lu\r\n", stats.receive_errors);
    printf("========================================\r\n\r\n");
}