/**
  ******************************************************************************
  * @file    can_testbox_demo.c
  * @brief   CAN测试盒API使用演示
  * @author  正点原子团队
  * @version V1.0
  * @date    2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件演示了CAN测试盒专业API的各种使用方法，包括：
  * 1. 单帧事件报文发送
  * 2. 周期性报文发送
  * 3. 连续帧报文发送
  * 4. 报文接收处理
  * 5. 统计信息获取
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can_testbox_api.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t demo_running = 0;
static uint32_t demo_step = 0;

/* Private function prototypes -----------------------------------------------*/
static void Demo_RxCallback(const CAN_TestBox_Message_t *message);
static void Demo_SingleFrameTest(void);
static void Demo_PeriodicMessageTest(void);
static void Demo_BurstFramesTest(void);
static void Demo_StatisticsTest(void);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  CAN测试盒演示初始化
  * @param  None
  * @retval CAN_TestBox_Status_t 状态码
  */
CAN_TestBox_Status_t CAN_TestBox_Demo_Init(void)
{
    printf("\r\n=== CAN TestBox Professional Demo ===\r\n");
    printf("Initializing CAN TestBox Demo...\r\n");
    
    // 设置自定义接收回调函数
    CAN_TestBox_SetRxCallback(Demo_RxCallback);
    
    demo_running = 1;
    demo_step = 0;
    
    printf("CAN TestBox Demo initialized successfully\r\n");
    return CAN_TESTBOX_OK;
}

/**
  * @brief  CAN测试盒演示主任务
  * @param  None
  * @retval None
  */
void CAN_TestBox_Demo_Task(void)
{
    if (!demo_running) {
        return;
    }
    
    static uint32_t last_demo_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每5秒执行一次演示步骤
    if (current_time - last_demo_time >= 5000) {
        last_demo_time = current_time;
        
        switch (demo_step % 4) {
            case 0:
                Demo_SingleFrameTest();
                break;
            case 1:
                Demo_PeriodicMessageTest();
                break;
            case 2:
                Demo_BurstFramesTest();
                break;
            case 3:
                Demo_StatisticsTest();
                break;
        }
        
        demo_step++;
    }
}

/**
  * @brief  停止CAN测试盒演示
  * @param  None
  * @retval None
  */
void CAN_TestBox_Demo_Stop(void)
{
    demo_running = 0;
    printf("CAN TestBox Demo stopped\r\n");
}

/**
  * @brief  自定义接收回调函数
  * @param  message: 接收到的CAN消息
  * @retval None
  */
static void Demo_RxCallback(const CAN_TestBox_Message_t *message)
{
    printf("[RX] ID: 0x%03lX, DLC: %d, Data: ", message->id, message->dlc);
    for (int i = 0; i < message->dlc; i++) {
        printf("%02X ", message->data[i]);
    }
    if (message->is_extended) {
        printf("(Extended)");
    }
    if (message->is_remote) {
        printf("(Remote)");
    }
    printf("\r\n");
}

/**
  * @brief  单帧事件报文发送演示
  * @param  None
  * @retval None
  */
static void Demo_SingleFrameTest(void)
{
    printf("\r\n--- Demo: Single Frame Event Test ---\r\n");
    
    // 测试1: 标准帧发送
    uint8_t data1[] = {0x01, 0x02, 0x03, 0x04};
    CAN_TestBox_Status_t status = CAN_TestBox_SendSingleFrameQuick(0x123, 4, data1, false);
    if (status == CAN_TESTBOX_OK) {
        printf("Standard frame sent: ID=0x123, Data=[01 02 03 04]\r\n");
    } else {
        printf("Standard frame send failed: %d\r\n", status);
    }
    
    // 测试2: 扩展帧发送
    uint8_t data2[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    status = CAN_TestBox_SendSingleFrameQuick(0x12345678, 6, data2, true);
    if (status == CAN_TESTBOX_OK) {
        printf("Extended frame sent: ID=0x12345678, Data=[AA BB CC DD EE FF]\r\n");
    } else {
        printf("Extended frame send failed: %d\r\n", status);
    }
    
    // 测试3: 使用完整结构体发送
    CAN_TestBox_Message_t msg = {
        .id = 0x456,
        .dlc = 8,
        .data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88},
        .is_extended = false,
        .is_remote = false
    };
    status = CAN_TestBox_SendSingleFrame(&msg);
    if (status == CAN_TESTBOX_OK) {
        printf("Full structure frame sent: ID=0x456\r\n");
    } else {
        printf("Full structure frame send failed: %d\r\n", status);
    }
}

/**
  * @brief  周期性报文发送演示
  * @param  None
  * @retval None
  */
static void Demo_PeriodicMessageTest(void)
{
    printf("\r\n--- Demo: Periodic Message Test ---\r\n");
    
    static uint8_t periodic_handle = 0xFF;
    static uint8_t periodic_active = 0;
    
    if (!periodic_active) {
        // 启动周期性消息
        CAN_TestBox_Message_t periodic_msg = {
            .id = 0x100,
            .dlc = 8,
            .data = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80},
            .is_extended = false,
            .is_remote = false
        };
        
        CAN_TestBox_Status_t status = CAN_TestBox_StartPeriodicMessage(
            &periodic_msg, CAN_TESTBOX_PERIOD_500MS, &periodic_handle);
        
        if (status == CAN_TESTBOX_OK) {
            printf("Periodic message started: ID=0x100, Period=500ms, Handle=%d\r\n", periodic_handle);
            periodic_active = 1;
        } else {
            printf("Periodic message start failed: %d\r\n", status);
        }
    } else {
        // 修改周期性消息数据
        uint8_t new_data[] = {0x99, 0x88, 0x77, 0x66};
        CAN_TestBox_Status_t status = CAN_TestBox_ModifyPeriodicData(periodic_handle, new_data, 4);
        
        if (status == CAN_TESTBOX_OK) {
            printf("Periodic message data modified: Handle=%d\r\n", periodic_handle);
        } else {
            printf("Periodic message data modify failed: %d\r\n", status);
        }
        
        // 停止周期性消息
        status = CAN_TestBox_StopPeriodicMessage(periodic_handle);
        if (status == CAN_TESTBOX_OK) {
            printf("Periodic message stopped: Handle=%d\r\n", periodic_handle);
            periodic_active = 0;
        } else {
            printf("Periodic message stop failed: %d\r\n", status);
        }
    }
}

/**
  * @brief  连续帧报文发送演示
  * @param  None
  * @retval None
  */
static void Demo_BurstFramesTest(void)
{
    printf("\r\n--- Demo: Burst Frames Test ---\r\n");
    
    // 测试1: 快速连续帧发送
    uint8_t burst_data1[] = {0xAA, 0xBB, 0xCC, 0xDD};
    CAN_TestBox_Status_t status = CAN_TestBox_SendBurstFramesQuick(
        0x200, 4, burst_data1, 3, CAN_TESTBOX_INTERVAL_10MS, false);
    
    if (status == CAN_TESTBOX_OK) {
        printf("Burst frames sent: ID=0x200, Count=3, Interval=10ms\r\n");
    } else {
        printf("Burst frames send failed: %d\r\n", status);
    }
    
    // 等待一段时间
    osDelay(100);
    
    // 测试2: 使用完整配置的连续帧发送
    CAN_TestBox_BurstMsg_t burst_config = {
        .message = {
            .id = 0x300,
            .dlc = 8,
            .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
            .is_extended = false,
            .is_remote = false
        },
        .burst_count = 5,
        .interval_ms = CAN_TESTBOX_INTERVAL_20MS,
        .auto_increment_id = true,
        .auto_increment_data = true
    };
    
    status = CAN_TestBox_SendBurstFrames(&burst_config);
    if (status == CAN_TESTBOX_OK) {
        printf("Advanced burst frames sent: ID=0x300+, Count=5, Auto-increment enabled\r\n");
    } else {
        printf("Advanced burst frames send failed: %d\r\n", status);
    }
}

/**
  * @brief  统计信息演示
  * @param  None
  * @retval None
  */
static void Demo_StatisticsTest(void)
{
    printf("\r\n--- Demo: Statistics Test ---\r\n");
    
    CAN_TestBox_Statistics_t stats;
    CAN_TestBox_Status_t status = CAN_TestBox_GetStatistics(&stats);
    
    if (status == CAN_TESTBOX_OK) {
        printf("=== CAN TestBox Statistics ===\r\n");
        printf("TX Messages: %lu\r\n", stats.tx_count);
        printf("RX Messages: %lu\r\n", stats.rx_count);
        printf("TX Errors: %lu\r\n", stats.tx_error_count);
        printf("RX Errors: %lu\r\n", stats.rx_error_count);
        printf("Total Errors: %lu\r\n", stats.error_count);
        printf("Bus Load: %lu%%\r\n", stats.bus_load_percent);
        printf("Uptime: %lu ms\r\n", stats.uptime_ms);
        
        // 获取详细状态
        CAN_TestBox_Status_Info_t status_info;
        if (CAN_TestBox_GetStatusInfo(&status_info) == CAN_TESTBOX_OK) {
            printf("Bus State: %s\r\n", 
                   status_info.bus_state == CAN_BUS_STATE_ACTIVE ? "Active" :
                   status_info.bus_state == CAN_BUS_STATE_PASSIVE ? "Passive" :
                   status_info.bus_state == CAN_BUS_STATE_OFF ? "Bus Off" : "Unknown");
            printf("Queue Usage: TX=%d%%, RX=%d%%\r\n", 
                   status_info.tx_queue_usage_percent, status_info.rx_queue_usage_percent);
        }
    } else {
        printf("Failed to get statistics: %d\r\n", status);
    }
    
    // 演示清除统计信息
    printf("\r\nClearing statistics...\r\n");
    CAN_TestBox_ClearStatistics();
    printf("Statistics cleared\r\n");
}

/**
  * @brief  高级功能演示
  * @param  None
  * @retval None
  */
void CAN_TestBox_Demo_AdvancedFeatures(void)
{
    printf("\r\n=== Advanced Features Demo ===\r\n");
    
    // 演示过滤器设置
    printf("Setting up message filters...\r\n");
    CAN_TestBox_Filter_t filter = {
        .id = 0x100,
        .mask = 0x700,  // 只接收0x100-0x1FF范围的消息
        .is_extended = false
    };
    
    CAN_TestBox_Status_t status = CAN_TestBox_SetFilter(&filter);
    if (status == CAN_TESTBOX_OK) {
        printf("Filter set: Accept ID 0x100-0x1FF\r\n");
    } else {
        printf("Filter setup failed: %d\r\n", status);
    }
    
    // 演示自检功能
    printf("\r\nRunning self-test...\r\n");
    status = CAN_TestBox_SelfTest();
    if (status == CAN_TESTBOX_OK) {
        printf("Self-test passed\r\n");
    } else {
        printf("Self-test failed: %d\r\n", status);
    }
    
    // 演示配置保存/加载
    printf("\r\nSaving configuration...\r\n");
    CAN_TestBox_Config_t config;
    status = CAN_TestBox_SaveConfig(&config);
    if (status == CAN_TESTBOX_OK) {
        printf("Configuration saved\r\n");
        
        // 加载配置
        status = CAN_TestBox_LoadConfig(&config);
        if (status == CAN_TESTBOX_OK) {
            printf("Configuration loaded\r\n");
        } else {
            printf("Configuration load failed: %d\r\n", status);
        }
    } else {
        printf("Configuration save failed: %d\r\n", status);
    }
}

/**
  * @brief  压力测试演示
  * @param  duration_ms: 测试持续时间(毫秒)
  * @retval None
  */
void CAN_TestBox_Demo_StressTest(uint32_t duration_ms)
{
    printf("\r\n=== Stress Test Demo ===\r\n");
    printf("Starting %lu ms stress test...\r\n", duration_ms);
    
    uint32_t start_time = HAL_GetTick();
    uint32_t message_count = 0;
    
    while ((HAL_GetTick() - start_time) < duration_ms) {
        // 快速发送消息
        uint8_t data[] = {(uint8_t)(message_count & 0xFF), 
                         (uint8_t)((message_count >> 8) & 0xFF),
                         (uint8_t)((message_count >> 16) & 0xFF),
                         (uint8_t)((message_count >> 24) & 0xFF)};
        
        CAN_TestBox_Status_t status = CAN_TestBox_SendSingleFrameQuick(
            0x500 + (message_count % 16), 4, data, false);
        
        if (status == CAN_TESTBOX_OK) {
            message_count++;
        }
        
        // 短暂延时
        osDelay(1);
    }
    
    printf("Stress test completed: %lu messages sent in %lu ms\r\n", 
           message_count, duration_ms);
    printf("Average rate: %.2f msg/s\r\n", 
           (float)message_count * 1000.0f / (float)duration_ms);
}