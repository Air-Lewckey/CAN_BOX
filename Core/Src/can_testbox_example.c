/**
 * @file can_testbox_example.c
 * @brief CAN测试盒API使用示例
 * @version 1.0
 * @date 2024
 * 
 * 本文件展示了如何使用CAN测试盒的各种API接口
 */

#include "can_testbox_api.h"
#include "can.h"
#include "usart.h"
#include <stdio.h>

/* ========================= 示例函数声明 ========================= */

void CAN_TestBox_Example_Init(void);
void CAN_TestBox_Example_SingleFrame(void);
void CAN_TestBox_Example_PeriodicMessage(void);
void CAN_TestBox_Example_BurstFrames(void);
void CAN_TestBox_Example_ReceiveMessage(void);
void CAN_TestBox_Example_Statistics(void);
void CAN_TestBox_Example_RxCallback(const CAN_TestBox_Message_t *message);

/* ========================= 全局变量 ========================= */

// 周期性消息句柄
static uint8_t g_periodic_handle_1 = 0;
static uint8_t g_periodic_handle_2 = 0;
static uint8_t g_periodic_handle_3 = 0;

/* ========================= 示例函数实现 ========================= */

/**
 * @brief CAN测试盒初始化示例
 */
void CAN_TestBox_Example_Init(void)
{
    printf("\r\n=== CAN TestBox API Example ===\r\n");
    
    // 初始化CAN测试盒
    CAN_TestBox_Status_t status = CAN_TestBox_Init(&hcan1);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] CAN TestBox initialized successfully\r\n");
    } else {
        printf("[Example] CAN TestBox initialization failed: %d\r\n", status);
        return;
    }
    
    // 设置接收回调函数
    CAN_TestBox_SetRxCallback(CAN_TestBox_Example_RxCallback);
    
    printf("[Example] Ready for CAN communication\r\n\r\n");
}

/**
 * @brief 单帧事件报文发送示例
 */
void CAN_TestBox_Example_SingleFrame(void)
{
    printf("=== Single Frame Example ===\r\n");
    
    // 方法1: 使用完整结构体
    CAN_TestBox_Message_t msg1 = {
        .id = 0x123,
        .dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
        .is_extended = false,
        .is_remote = false
    };
    
    CAN_TestBox_Status_t status = CAN_TestBox_SendSingleFrame(&msg1);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Single frame sent: ID=0x123\r\n");
    } else {
        printf("[Example] Single frame send failed: %d\r\n", status);
    }
    
    // 方法2: 使用快速接口
    uint8_t data2[] = {0xAA, 0xBB, 0xCC, 0xDD};
    status = CAN_TestBox_SendSingleFrameQuick(0x456, 4, data2, false);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Quick single frame sent: ID=0x456\r\n");
    } else {
        printf("[Example] Quick single frame send failed: %d\r\n", status);
    }
    
    // 发送扩展帧
    uint8_t data3[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    status = CAN_TestBox_SendSingleFrameQuick(0x12345678, 6, data3, true);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Extended frame sent: ID=0x12345678\r\n");
    } else {
        printf("[Example] Extended frame send failed: %d\r\n", status);
    }
    
    printf("\r\n");
}

/**
 * @brief 周期性消息发送示例
 */
void CAN_TestBox_Example_PeriodicMessage(void)
{
    printf("=== Periodic Message Example ===\r\n");
    
    // 启动第一个周期性消息 - 100ms周期
    CAN_TestBox_Message_t periodic_msg1 = {
        .id = 0x100,
        .dlc = 8,
        .data = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80},
        .is_extended = false,
        .is_remote = false
    };
    
    CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(&periodic_msg1, CAN_TESTBOX_PERIOD_100MS, &g_periodic_handle_1);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Periodic message 1 started: ID=0x100, Period=100ms, Handle=%d\r\n", g_periodic_handle_1);
    } else {
        printf("[Example] Periodic message 1 start failed: %d\r\n", status);
    }
    
    // 启动第二个周期性消息 - 500ms周期
    CAN_TestBox_Message_t periodic_msg2 = {
        .id = 0x200,
        .dlc = 4,
        .data = {0xA1, 0xB2, 0xC3, 0xD4},
        .is_extended = false,
        .is_remote = false
    };
    
    status = CAN_TestBox_StartPeriodicMessage(&periodic_msg2, CAN_TESTBOX_PERIOD_500MS, &g_periodic_handle_2);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Periodic message 2 started: ID=0x200, Period=500ms, Handle=%d\r\n", g_periodic_handle_2);
    } else {
        printf("[Example] Periodic message 2 start failed: %d\r\n", status);
    }
    
    // 启动第三个周期性消息 - 1000ms周期
    CAN_TestBox_Message_t periodic_msg3 = {
        .id = 0x300,
        .dlc = 2,
        .data = {0xFF, 0x00},
        .is_extended = false,
        .is_remote = false
    };
    
    status = CAN_TestBox_StartPeriodicMessage(&periodic_msg3, CAN_TESTBOX_PERIOD_1000MS, &g_periodic_handle_3);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Periodic message 3 started: ID=0x300, Period=1000ms, Handle=%d\r\n", g_periodic_handle_3);
    } else {
        printf("[Example] Periodic message 3 start failed: %d\r\n", status);
    }
    
    printf("[Example] All periodic messages started. They will send automatically.\r\n");
    printf("\r\n");
}

/**
 * @brief 连续帧发送示例
 */
void CAN_TestBox_Example_BurstFrames(void)
{
    printf("=== Burst Frames Example ===\r\n");
    
    // 方法1: 使用完整配置结构体
    CAN_TestBox_BurstMsg_t burst_config1 = {
        .message = {
            .id = 0x400,
            .dlc = 8,
            .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
            .is_extended = false,
            .is_remote = false
        },
        .burst_count = 5,
        .interval_ms = CAN_TESTBOX_INTERVAL_10MS,
        .auto_increment_id = true,
        .auto_increment_data = false
    };
    
    printf("[Example] Sending burst frames with auto-increment ID...\r\n");
    CAN_TestBox_Status_t status = CAN_TestBox_SendBurstFrames(&burst_config1);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Burst frames 1 completed\r\n");
    } else {
        printf("[Example] Burst frames 1 failed: %d\r\n", status);
    }
    
    // 等待一段时间
    HAL_Delay(100);
    
    // 方法2: 使用快速接口
    uint8_t burst_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    printf("[Example] Sending burst frames with quick interface...\r\n");
    status = CAN_TestBox_SendBurstFramesQuick(0x500, 6, burst_data, 3, CAN_TESTBOX_INTERVAL_5MS, false);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Burst frames 2 completed\r\n");
    } else {
        printf("[Example] Burst frames 2 failed: %d\r\n", status);
    }
    
    // 等待一段时间
    HAL_Delay(100);
    
    // 方法3: 发送大量连续帧
    CAN_TestBox_BurstMsg_t burst_config2 = {
        .message = {
            .id = 0x600,
            .dlc = 8,
            .data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
            .is_extended = false,
            .is_remote = false
        },
        .burst_count = 20,
        .interval_ms = CAN_TESTBOX_INTERVAL_2MS,
        .auto_increment_id = true,
        .auto_increment_data = true
    };
    
    printf("[Example] Sending 20 burst frames with auto-increment ID and data...\r\n");
    status = CAN_TestBox_SendBurstFrames(&burst_config2);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Large burst frames completed\r\n");
    } else {
        printf("[Example] Large burst frames failed: %d\r\n", status);
    }
    
    printf("\r\n");
}

/**
 * @brief 接收消息示例
 */
void CAN_TestBox_Example_ReceiveMessage(void)
{
    printf("=== Receive Message Example ===\r\n");
    
    CAN_TestBox_Message_t rx_message;
    
    printf("[Example] Waiting for CAN messages (timeout: 5000ms)...\r\n");
    
    // 尝试接收消息
    for (int i = 0; i < 5; i++) {
        CAN_TestBox_Status_t status = CAN_TestBox_ReceiveMessage(&rx_message, 1000);
        
        if (status == CAN_TESTBOX_OK) {
            printf("[Example] Received message %d: ID=0x%03X, DLC=%d, Data=", 
                   i+1, (unsigned int)rx_message.id, rx_message.dlc);
            for (uint8_t j = 0; j < rx_message.dlc; j++) {
                printf("%02X ", rx_message.data[j]);
            }
            printf("\r\n");
        } else if (status == CAN_TESTBOX_TIMEOUT) {
            printf("[Example] Receive timeout %d\r\n", i+1);
        } else {
            printf("[Example] Receive error: %d\r\n", status);
            break;
        }
    }
    
    printf("\r\n");
}

/**
 * @brief 统计信息示例
 */
void CAN_TestBox_Example_Statistics(void)
{
    printf("=== Statistics Example ===\r\n");
    
    CAN_TestBox_Statistics_t stats;
    CAN_TestBox_Status_t status = CAN_TestBox_GetStatistics(&stats);
    
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] CAN TestBox Statistics:\r\n");
        printf("  TX Total:    %lu\r\n", stats.tx_total_count);
        printf("  TX Success:  %lu\r\n", stats.tx_success_count);
        printf("  TX Error:    %lu\r\n", stats.tx_error_count);
        printf("  RX Total:    %lu\r\n", stats.rx_total_count);
        printf("  RX Valid:    %lu\r\n", stats.rx_valid_count);
        printf("  RX Error:    %lu\r\n", stats.rx_error_count);
        printf("  Bus Error:   %lu\r\n", stats.bus_error_count);
        printf("  Last Error:  0x%08lX\r\n", stats.last_error_code);
        printf("  Uptime:      %lu ms\r\n", stats.uptime_ms);
    } else {
        printf("[Example] Get statistics failed: %d\r\n", status);
    }
    
    printf("\r\n");
}

/**
 * @brief 接收回调函数示例
 */
void CAN_TestBox_Example_RxCallback(const CAN_TestBox_Message_t *message)
{
    // 这个回调函数会在接收到CAN消息时自动调用
    printf("[Callback] RX: ID=0x%03X, DLC=%d, Data=", 
           (unsigned int)message->id, message->dlc);
    for (uint8_t i = 0; i < message->dlc; i++) {
        printf("%02X ", message->data[i]);
    }
    printf("\r\n");
}

/**
 * @brief 周期性消息管理示例
 */
void CAN_TestBox_Example_PeriodicManagement(void)
{
    printf("=== Periodic Message Management Example ===\r\n");
    
    // 修改周期性消息的发送周期
    CAN_TestBox_Status_t status = CAN_TestBox_ModifyPeriodicPeriod(g_periodic_handle_1, CAN_TESTBOX_PERIOD_200MS);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Modified periodic message 1 period to 200ms\r\n");
    }
    
    // 修改周期性消息的数据内容
    uint8_t new_data[] = {0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22};
    status = CAN_TestBox_ModifyPeriodicData(g_periodic_handle_2, new_data, 8);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Modified periodic message 2 data\r\n");
    }
    
    // 等待一段时间观察效果
    HAL_Delay(2000);
    
    // 停止一个周期性消息
    status = CAN_TestBox_StopPeriodicMessage(g_periodic_handle_3);
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Stopped periodic message 3\r\n");
    }
    
    printf("\r\n");
}

/**
 * @brief 自检示例
 */
void CAN_TestBox_Example_SelfTest(void)
{
    printf("=== Self Test Example ===\r\n");
    
    CAN_TestBox_Status_t status = CAN_TestBox_SelfTest();
    if (status == CAN_TESTBOX_OK) {
        printf("[Example] Self test passed\r\n");
    } else {
        printf("[Example] Self test failed: %d\r\n", status);
    }
    
    // 获取总线状态
    uint32_t bus_status = CAN_TestBox_GetBusStatus();
    printf("[Example] CAN Bus Status: 0x%08lX\r\n", bus_status);
    
    // 获取最后错误
    uint32_t last_error = CAN_TestBox_GetLastError();
    printf("[Example] Last Error: 0x%08lX\r\n", last_error);
    
    printf("\r\n");
}

/**
 * @brief 完整示例主函数
 */
void CAN_TestBox_Example_Main(void)
{
    // 1. 初始化
    CAN_TestBox_Example_Init();
    
    // 2. 发送单帧事件报文
    CAN_TestBox_Example_SingleFrame();
    
    // 3. 启动周期性消息
    CAN_TestBox_Example_PeriodicMessage();
    
    // 4. 发送连续帧
    CAN_TestBox_Example_BurstFrames();
    
    // 5. 接收消息测试
    CAN_TestBox_Example_ReceiveMessage();
    
    // 6. 周期性消息管理
    CAN_TestBox_Example_PeriodicManagement();
    
    // 7. 自检测试
    CAN_TestBox_Example_SelfTest();
    
    // 8. 显示统计信息
    CAN_TestBox_Example_Statistics();
    
    printf("=== CAN TestBox Example Completed ===\r\n");
    
    // 最后停止所有周期性消息
    CAN_TestBox_StopAllPeriodicMessages();
}

/**
 * @brief 简单的任务循环示例
 */
void CAN_TestBox_Example_TaskLoop(void)
{
    static uint32_t last_stats_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 调用CAN测试盒主任务
    CAN_TestBox_Task();
    
    // 每10秒显示一次统计信息
    if (current_time - last_stats_time >= 10000) {
        CAN_TestBox_Example_Statistics();
        last_stats_time = current_time;
    }
}