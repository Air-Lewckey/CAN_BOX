/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_app.c
  * @brief          : CAN应用层实现文件
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件实现了基于MCP2515的CAN通信应用层功能，包括：
  * 1. CAN发送任务实现
  * 2. CAN接收任务实现
  * 3. 中断服务程序
  * 4. 消息队列处理
  * 5. 应用示例和测试函数
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "can_app.h"
#include "mcp2515.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CAN_QUEUE_SIZE          10      // CAN消息队列大小
#define CAN_SEND_PERIOD         1000    // CAN发送周期(ms)
#define CAN_HEARTBEAT_ID        0x100   // 心跳消息ID
#define CAN_DATA_ID             0x200   // 数据消息ID
#define CAN_STATUS_ID           0x300   // 状态消息ID

/* Private variables ---------------------------------------------------------*/
static uint32_t can_tx_counter = 0;     // 发送计数器
static uint32_t can_rx_counter = 0;     // 接收计数器
static uint32_t can_error_counter = 0;  // 错误计数器
static uint8_t can_app_initialized = 0; // 应用初始化标志

/* 外部变量 */
extern osMessageQueueId_t myQueue01Handle;  // 消息队列句柄

/* Private function prototypes -----------------------------------------------*/
static void CAN_SendHeartbeat(void);
static void CAN_SendTestData(void);
static void CAN_ProcessReceivedMessage(MCP2515_CANMessage_t *message);
static void CAN_PrintMessage(const char *prefix, MCP2515_CANMessage_t *message);

/* 应用初始化函数 ------------------------------------------------------------*/

/**
  * @brief  CAN应用初始化
  * @param  None
  * @retval CAN_APP_OK: 成功, CAN_APP_ERROR: 失败
  */
uint8_t CAN_App_Init(void)
{
    // 初始化MCP2515 (500Kbps波特率)
    if (MCP2515_Init(MCP2515_BAUD_500K) != MCP2515_OK) {
        printf("MCP2515 initialization failed!\r\n");
        printf("CAN application initialization failed!\r\n");
        
        printf("\r\nWARNING: CAN initialization failed, starting diagnosis...\r\n");
        
        // 调用初始化失败诊断函数
        MCP2515_InitFailureDiagnosis();
        
        return CAN_APP_ERROR;
    }
    
    printf("MCP2515 initialization successful!\r\n");
    
    // 配置接收过滤器 (接收所有消息)
    MCP2515_SetMask(0, 0x00000000, 0);  // 掩码0: 接收所有标准帧
    MCP2515_SetMask(1, 0x00000000, 0);  // 掩码1: 接收所有标准帧
    
    // 打印初始状态
    MCP2515_PrintStatus();
    
    can_app_initialized = 1;
    
    printf("CAN application initialization completed!\r\n");
    return CAN_APP_OK;
}

/**
  * @brief  获取CAN应用统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN_App_GetStats(CAN_App_Stats_t *stats)
{
    if (stats != NULL) {
        stats->tx_count = can_tx_counter;
        stats->rx_count = can_rx_counter;
        stats->error_count = can_error_counter;
        stats->initialized = can_app_initialized;
    }
}

/* CAN发送任务实现 -----------------------------------------------------------*/

/**
  * @brief  CAN发送任务主函数
  * @param  argument: 任务参数
  * @retval None
  * @note   此函数在StartCANSendTask中调用
  */
void CAN_SendTask_Main(void *argument)
{
    uint32_t last_heartbeat = 0;
    uint32_t last_data_send = 0;
    uint32_t current_time;
    
    printf("CAN send task started\r\n");
    
    // 等待CAN应用初始化完成
    while (!can_app_initialized) {
        osDelay(100);
    }
    
    for (;;) {
        current_time = HAL_GetTick();
        
        // 每1秒发送一次心跳消息
        if ((current_time - last_heartbeat) >= 1000) {
            CAN_SendHeartbeat();
            last_heartbeat = current_time;
        }
        
        // 每2秒发送一次测试数据
        if ((current_time - last_data_send) >= 2000) {
            CAN_SendTestData();
            last_data_send = current_time;
        }
        
        // 检查是否有来自队列的发送请求
        CAN_QueueMessage_t queue_msg;
        if (osMessageQueueGet(myQueue01Handle, &queue_msg, NULL, 10) == osOK) {
            // 处理队列中的发送请求
            if (MCP2515_SendMessage(&queue_msg.message) == MCP2515_OK) {
                can_tx_counter++;
                printf("Queue message sent successfully, ID: 0x%03X\r\n", (unsigned int)queue_msg.message.id);
            } else {
                can_error_counter++;
                printf("Queue message send failed, ID: 0x%03X\r\n", (unsigned int)queue_msg.message.id);
            }
        }
        
        osDelay(50);  // 50ms周期
    }
}

/**
  * @brief  发送心跳消息
  * @param  None
  * @retval None
  */
static void CAN_SendHeartbeat(void)
{
    MCP2515_CANMessage_t heartbeat;
    
    // 构造心跳消息
    heartbeat.id = CAN_HEARTBEAT_ID;
    heartbeat.ide = 0;  // 标准帧
    heartbeat.rtr = 0;  // 数据帧
    heartbeat.dlc = 8;  // 8字节数据
    
    // 填充心跳数据
    heartbeat.data[0] = 0xAA;  // 心跳标识
    heartbeat.data[1] = 0x55;
    heartbeat.data[2] = (uint8_t)(can_tx_counter >> 24);
    heartbeat.data[3] = (uint8_t)(can_tx_counter >> 16);
    heartbeat.data[4] = (uint8_t)(can_tx_counter >> 8);
    heartbeat.data[5] = (uint8_t)can_tx_counter;
    heartbeat.data[6] = (uint8_t)(HAL_GetTick() >> 8);
    heartbeat.data[7] = (uint8_t)HAL_GetTick();
    
    // 发送心跳消息
    if (MCP2515_SendMessage(&heartbeat) == MCP2515_OK) {
        can_tx_counter++;
        printf("Heartbeat message sent successfully [%lu]\r\n", can_tx_counter);
    } else {
        can_error_counter++;
        printf("Heartbeat message send failed\r\n");
    }
}

/**
  * @brief  发送测试数据
  * @param  None
  * @retval None
  */
static void CAN_SendTestData(void)
{
    MCP2515_CANMessage_t test_data;
    static uint16_t data_counter = 0;
    
    // 构造测试数据消息
    test_data.id = CAN_DATA_ID;
    test_data.ide = 0;  // 标准帧
    test_data.rtr = 0;  // 数据帧
    test_data.dlc = 6;  // 6字节数据
    
    // 填充测试数据
    test_data.data[0] = 0x12;  // 数据标识
    test_data.data[1] = 0x34;
    test_data.data[2] = (uint8_t)(data_counter >> 8);
    test_data.data[3] = (uint8_t)data_counter;
    test_data.data[4] = (uint8_t)(HAL_GetTick() >> 16);
    test_data.data[5] = (uint8_t)(HAL_GetTick() >> 8);
    
    // 发送测试数据
    if (MCP2515_SendMessage(&test_data) == MCP2515_OK) {
        can_tx_counter++;
        data_counter++;
        printf("Test data sent successfully, count: %d\r\n", data_counter);
    } else {
        can_error_counter++;
        printf("Test data send failed\r\n");
    }
}

/* CAN接收任务实现 -----------------------------------------------------------*/

/**
  * @brief  CAN接收任务主函数
  * @param  argument: 任务参数
  * @retval None
  * @note   此函数在StartCANReceiveTask中调用
  */
void CAN_ReceiveTask_Main(void *argument)
{
    MCP2515_CANMessage_t received_message;
    
    printf("CAN receive task started\r\n");
    
    // 等待CAN应用初始化完成
    while (!can_app_initialized) {
        osDelay(100);
    }
    
    for (;;) {
        // 检查是否有消息接收
        if (MCP2515_CheckReceive()) {
            // 接收消息
            if (MCP2515_ReceiveMessage(&received_message) == MCP2515_OK) {
                can_rx_counter++;
                
                // 打印接收到的消息
                CAN_PrintMessage("Received", &received_message);
                
                // 处理接收到的消息
                CAN_ProcessReceivedMessage(&received_message);
            } else {
                can_error_counter++;
                printf("Message receive failed\r\n");
            }
        }
        
        osDelay(10);  // 10ms周期检查
    }
}

/**
  * @brief  处理接收到的CAN消息
  * @param  message: 接收到的消息
  * @retval None
  */
static void CAN_ProcessReceivedMessage(MCP2515_CANMessage_t *message)
{
    switch (message->id) {
        case CAN_HEARTBEAT_ID:
            // 处理心跳消息
            if (message->dlc >= 2 && message->data[0] == 0xAA && message->data[1] == 0x55) {
                printf("Heartbeat message received\r\n");
            }
            break;
            
        case CAN_DATA_ID:
            // 处理数据消息
            if (message->dlc >= 2 && message->data[0] == 0x12 && message->data[1] == 0x34) {
                uint16_t counter = (message->data[2] << 8) | message->data[3];
                printf("Test data received, count: %d\r\n", counter);
            }
            break;
            
        case CAN_STATUS_ID:
            // 处理状态消息
            printf("Status message received\r\n");
            break;
            
        default:
            // 处理其他消息
            printf("Unknown message received, ID: 0x%03X\r\n", (unsigned int)message->id);
            break;
    }
}

/* 中断服务程序 --------------------------------------------------------------*/

/**
  * @brief  外部中断回调函数
  * @param  GPIO_Pin: 中断引脚
  * @retval None
  * @note   此函数在stm32f4xx_it.c的HAL_GPIO_EXTI_Callback中调用
  */
void CAN_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == MCP2515_INT_Pin) {
        // MCP2515中断处理
        MCP2515_IRQHandler();
        
        // 可以在这里发送信号量通知接收任务
        // 或者直接在中断中处理消息接收
    }
}

/* 应用接口函数 --------------------------------------------------------------*/

/**
  * @brief  发送CAN消息到队列
  * @param  id: CAN ID
  * @param  data: 数据指针
  * @param  length: 数据长度
  * @param  extended: 0=标准帧, 1=扩展帧
  * @retval CAN_APP_OK: 成功, CAN_APP_ERROR: 失败
  */
uint8_t CAN_App_SendMessage(uint32_t id, uint8_t *data, uint8_t length, uint8_t extended)
{
    CAN_QueueMessage_t queue_msg;
    
    if (!can_app_initialized || data == NULL || length > 8) {
        return CAN_APP_ERROR;
    }
    
    // 构造队列消息
    queue_msg.message.id = id;
    queue_msg.message.ide = extended;
    queue_msg.message.rtr = 0;  // 数据帧
    queue_msg.message.dlc = length;
    
    // 复制数据
    memcpy(queue_msg.message.data, data, length);
    
    // 发送到队列
    if (osMessageQueuePut(myQueue01Handle, &queue_msg, 0, 100) == osOK) {
        return CAN_APP_OK;
    }
    
    return CAN_APP_ERROR;
}

/**
  * @brief  发送远程帧
  * @param  id: CAN ID
  * @param  dlc: 数据长度代码
  * @param  extended: 0=标准帧, 1=扩展帧
  * @retval CAN_APP_OK: 成功, CAN_APP_ERROR: 失败
  */
uint8_t CAN_App_SendRemoteFrame(uint32_t id, uint8_t dlc, uint8_t extended)
{
    MCP2515_CANMessage_t remote_frame;
    
    if (!can_app_initialized || dlc > 8) {
        return CAN_APP_ERROR;
    }
    
    // 构造远程帧
    remote_frame.id = id;
    remote_frame.ide = extended;
    remote_frame.rtr = 1;  // 远程帧
    remote_frame.dlc = dlc;
    
    // 远程帧不包含数据
    memset(remote_frame.data, 0, 8);
    
    // 直接发送远程帧
    if (MCP2515_SendMessage(&remote_frame) == MCP2515_OK) {
        can_tx_counter++;
        printf("Remote frame sent successfully, ID: 0x%03X\r\n", (unsigned int)id);
        return CAN_APP_OK;
    }
    
    can_error_counter++;
    printf("Remote frame send failed, ID: 0x%03X\r\n", (unsigned int)id);
    return CAN_APP_ERROR;
}

/**
  * @brief  设置CAN过滤器
  * @param  filter_id: 过滤器ID
  * @param  mask: 掩码值
  * @param  extended: 0=标准帧, 1=扩展帧
  * @retval CAN_APP_OK: 成功, CAN_APP_ERROR: 失败
  */
uint8_t CAN_App_SetFilter(uint32_t filter_id, uint32_t mask, uint8_t extended)
{
    if (!can_app_initialized) {
        return CAN_APP_ERROR;
    }
    
    // 设置过滤器0和掩码0
    if (MCP2515_SetFilter(0, filter_id, extended) != MCP2515_OK) {
        return CAN_APP_ERROR;
    }
    
    if (MCP2515_SetMask(0, mask, extended) != MCP2515_OK) {
        return CAN_APP_ERROR;
    }
    
    printf("Filter set successfully, ID: 0x%03X, Mask: 0x%03X\r\n", 
           (unsigned int)filter_id, (unsigned int)mask);
    
    return CAN_APP_OK;
}

/* 调试和测试函数 ------------------------------------------------------------*/

/**
  * @brief  CAN应用自检测试
  * @param  None
  * @retval CAN_APP_OK: 成功, CAN_APP_ERROR: 失败
  */
uint8_t CAN_App_SelfTest(void)
{
    printf("Starting CAN application self-test...\r\n");
    
    // 检查MCP2515硬件
    if (MCP2515_SelfTest() != MCP2515_OK) {
        printf("MCP2515 hardware test failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    printf("MCP2515 hardware test passed\r\n");
    
    // 检查回环模式
    if (MCP2515_SetMode(MCP2515_MODE_LOOPBACK) != MCP2515_OK) {
        printf("Set loopback mode failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    printf("Loopback mode set successfully\r\n");
    
    // 发送测试消息
    MCP2515_CANMessage_t test_msg;
    test_msg.id = 0x123;
    test_msg.ide = 0;
    test_msg.rtr = 0;
    test_msg.dlc = 8;
    for (int i = 0; i < 8; i++) {
        test_msg.data[i] = i + 1;
    }
    
    if (MCP2515_SendMessage(&test_msg) != MCP2515_OK) {
        printf("Loopback test message send failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    printf("Loopback test message sent successfully\r\n");
    
    // 等待并接收消息
    osDelay(100);
    
    MCP2515_CANMessage_t received_msg;
    if (MCP2515_ReceiveMessage(&received_msg) == MCP2515_OK) {
        // 验证接收到的消息
        if (received_msg.id == test_msg.id && 
            received_msg.dlc == test_msg.dlc &&
            memcmp(received_msg.data, test_msg.data, test_msg.dlc) == 0) {
            printf("Loopback test successful!\r\n");
        } else {
            printf("Loopback test data mismatch!\r\n");
            return CAN_APP_ERROR;
        }
    } else {
        printf("Loopback test message receive failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    // 恢复正常模式
    if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
        printf("Restore normal mode failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    printf("CAN application self-test completed!\r\n");
    return CAN_APP_OK;
}

/**
  * @brief  打印CAN消息信息
  * @param  prefix: 前缀字符串
  * @param  message: CAN消息指针
  * @retval None
  */
static void CAN_PrintMessage(const char *prefix, MCP2515_CANMessage_t *message)
{
    printf("%s Message: ID=0x%03X, %s, %s, DLC=%d, Data=", 
           prefix,
           (unsigned int)message->id,
           message->ide ? "Extended" : "Standard",
           message->rtr ? "Remote" : "Data",
           message->dlc);
    
    if (!message->rtr) {
        for (int i = 0; i < message->dlc && i < 8; i++) {
            printf("%02X ", message->data[i]);
        }
    }
    
    printf("\r\n");
}

/**
  * @brief  打印CAN应用状态
  * @param  None
  * @retval None
  */
void CAN_App_PrintStatus(void)
{
    CAN_App_Stats_t stats;
    CAN_App_GetStats(&stats);
    
    printf("\r\n=== CAN Application Status ===\r\n");
    printf("Initialization Status: %s\r\n", stats.initialized ? "Initialized" : "Not Initialized");
    printf("TX Count: %lu\r\n", stats.tx_count);
    printf("RX Count: %lu\r\n", stats.rx_count);
    printf("Error Count: %lu\r\n", stats.error_count);
    printf("TX Buffer Free: %d\r\n", MCP2515_CheckTransmit());
    printf("RX Status: %s\r\n", MCP2515_CheckReceive() ? "Message Available" : "No Message");
    
    // 打印MCP2515状态
    MCP2515_PrintStatus();
    
    printf("==================\r\n\r\n");
}

/**
  * @brief  MCP2515中断回调函数
  * @param  None
  * @retval None
  * @note   此函数在外部中断服务程序中被调用
  *         用于处理MCP2515的中断信号
  */
void CAN_App_IRQ_Callback(void)
{
    // 设置中断标志，通知接收任务处理
    // 注意：在中断服务程序中应该尽量减少处理时间
    // 实际的消息处理在CAN接收任务中进行
    
    // 可以在这里设置事件标志或信号量来通知任务
    // 这里暂时不做具体处理，实际处理在接收任务的轮询中进行
    
    // 如果需要立即处理，可以发送信号给接收任务
    // 例如：osThreadFlagsSet(CANReceiveTaskHandle, 0x01);
}