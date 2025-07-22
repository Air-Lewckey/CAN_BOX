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
#include "can_dual_node.h"
#include "can_bus_diagnosis.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CAN_QUEUE_SIZE          10      // CAN消息队列大小
#define CAN_SEND_PERIOD         1000    // CAN发送周期(ms)
#define CAN_HEARTBEAT_ID        0x100   // 心跳消息ID
#define CAN_DATA_ID             0x200   // 数据消息ID
#define CAN_APP_STATUS_ID       0x300   // 状态消息ID (MCP2515应用层)
#define CAN_SENSOR_ID           0x400   // 传感器数据ID
#define CAN_CONTROL_ID          0x500   // 控制指令ID

// 发送周期定义
#define HEARTBEAT_PERIOD        500     // 心跳消息周期(ms)
#define DATA_PERIOD             1000    // 测试数据周期(ms)
#define STATUS_PERIOD           2000    // 状态消息周期(ms)
#define SENSOR_PERIOD           800     // 传感器数据周期(ms)
#define CONTROL_PERIOD          1500    // 控制指令周期(ms)

/* Private variables ---------------------------------------------------------*/
static uint32_t can_tx_counter = 0;     // 发送计数器
static uint32_t can_rx_counter = 0;     // 接收计数器
static uint32_t can_error_counter = 0;  // 错误计数器
static uint8_t can_app_initialized = 0; // 应用初始化标志

/* 外部变量 */
extern osMessageQueueId_t myQueue01Handle;  // 消息队列句柄

/* Private function prototypes -----------------------------------------------*/
static void CAN_SendHeartbeat_App(void);
static void CAN_SendTestData(void);
static void CAN_SendStatusMessage_App(void);
static void CAN_SendSensorData(void);
static void CAN_SendControlCommand_App(void);
static void CAN_ProcessReceivedMessage_App(MCP2515_CANMessage_t *message);
static void CAN_PrintMessage_App(const char *prefix, MCP2515_CANMessage_t *message);

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
        // printf("MCP2515 initialization failed!\r\n");
        // printf("CAN application initialization failed!\r\n");
        
        // printf("\r\nWARNING: CAN initialization failed, starting diagnosis...\r\n");
        
        // 调用初始化失败诊断函数
        MCP2515_InitFailureDiagnosis();
        
        return CAN_APP_ERROR;
    }
    
    // printf("MCP2515 initialization successful!\r\n");
    
    // 配置接收过滤器 (接收所有消息)
    MCP2515_SetMask(0, 0x00000000, 0);  // 掩码0: 接收所有标准帧
    MCP2515_SetMask(1, 0x00000000, 0);  // 掩码1: 接收所有标准帧
    
    // 打印初始状态
    // MCP2515_PrintStatus();
    
    // 初始化STM32 CAN1双节点通信
    if (CAN_DualNode_Init() != HAL_OK) {
        // printf("STM32 CAN1 dual node initialization failed!\r\n");
        return CAN_APP_ERROR;
    }
    // printf("STM32 CAN1 dual node initialization successful!\r\n");
    
    can_app_initialized = 1;
    
    // printf("CAN application initialization completed!\r\n");
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
    uint32_t last_status_send = 0;
    uint32_t last_sensor_send = 0;
    uint32_t last_control_send = 0;
    uint32_t current_time;
    
    // printf("CAN send task started - Enhanced periodic transmission\r\n");
    
    // 等待CAN应用初始化完成
    while (!can_app_initialized) {
        osDelay(100);
    }
    
    // printf("Starting periodic CAN message transmission:\r\n");
    // printf("- Heartbeat: every %dms\r\n", HEARTBEAT_PERIOD);
    // printf("- Test Data: every %dms\r\n", DATA_PERIOD);
    // printf("- Status: every %dms\r\n", STATUS_PERIOD);
    // printf("- Sensor: every %dms\r\n", SENSOR_PERIOD);
    // printf("- Control: every %dms\r\n", CONTROL_PERIOD);
    
    for (;;) {
        current_time = HAL_GetTick();
        
        // 临时注释掉心跳包以便观察循环测试日志
        // if ((current_time - last_heartbeat) >= HEARTBEAT_PERIOD) {
        //     CAN_SendHeartbeat_App();
        //     last_heartbeat = current_time;
        // }
        
        // 临时注释掉其他周期性消息以便观察循环测试日志
        // if ((current_time - last_data_send) >= DATA_PERIOD) {
        //     CAN_SendTestData();
        //     last_data_send = current_time;
        // }
        // 
        // if ((current_time - last_status_send) >= STATUS_PERIOD) {
        //     CAN_SendStatusMessage_App();
        //     last_status_send = current_time;
        // }
        // 
        // if ((current_time - last_sensor_send) >= SENSOR_PERIOD) {
        //     CAN_SendSensorData();
        //     last_sensor_send = current_time;
        // }
        // 
        // if ((current_time - last_control_send) >= CONTROL_PERIOD) {
        //     CAN_SendControlCommand_App();
        //     last_control_send = current_time;
        // }
        
        // 检查是否有来自队列的发送请求
        CAN_QueueMessage_t queue_msg;
        if (osMessageQueueGet(myQueue01Handle, &queue_msg, NULL, 10) == osOK) {
            // 处理队列中的发送请求
            if (MCP2515_SendMessage(&queue_msg.message) == MCP2515_OK) {
                can_tx_counter++;
                // printf("Queue message sent successfully, ID: 0x%03X\r\n", (unsigned int)queue_msg.message.id);
            } else {
                can_error_counter++;
                // printf("Queue message send failed, ID: 0x%03X\r\n", (unsigned int)queue_msg.message.id);
            }
        }
        
        osDelay(50);  // 50ms周期
    }
}

/**
  * @brief  发送心跳消息 (MCP2515应用层)
  * @param  None
  * @retval None
  */
static void CAN_SendHeartbeat_App(void)
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
        // printf("Heartbeat message sent successfully [%lu]\r\n", can_tx_counter);
    } else {
        can_error_counter++;
        // printf("Heartbeat message send failed\r\n");
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
        // printf("Test data sent successfully, count: %d\r\n", data_counter);
    } else {
        can_error_counter++;
        // printf("Test data send failed\r\n");
    }
}

/**
  * @brief  发送状态消息 (MCP2515应用层)
  * @param  None
  * @retval None
  */
static void CAN_SendStatusMessage_App(void)
{
    MCP2515_CANMessage_t status_msg;
    static uint8_t system_status = 0;
    
    // 构造状态消息
    status_msg.id = CAN_APP_STATUS_ID;
    status_msg.ide = 0;  // 标准帧
    status_msg.rtr = 0;  // 数据帧
    status_msg.dlc = 8;  // 8字节数据
    
    // 填充状态数据
    status_msg.data[0] = 0x53;  // 状态标识 'S'
    status_msg.data[1] = 0x54;  // 状态标识 'T'
    status_msg.data[2] = system_status++;  // 系统状态计数
    status_msg.data[3] = (can_error_counter > 0) ? 0xFF : 0x00;  // 错误状态
    status_msg.data[4] = (uint8_t)(can_tx_counter >> 8);  // 发送计数高字节
    status_msg.data[5] = (uint8_t)can_tx_counter;          // 发送计数低字节
    status_msg.data[6] = (uint8_t)(can_rx_counter >> 8);  // 接收计数高字节
    status_msg.data[7] = (uint8_t)can_rx_counter;          // 接收计数低字节
    
    // 发送状态消息
    if (MCP2515_SendMessage(&status_msg) == MCP2515_OK) {
        can_tx_counter++;
        // printf("Status message sent, system status: %d\r\n", system_status-1);
    } else {
        can_error_counter++;
        // printf("Status message send failed\r\n");
    }
}

/**
  * @brief  发送传感器数据
  * @param  None
  * @retval None
  */
static void CAN_SendSensorData(void)
{
    MCP2515_CANMessage_t sensor_msg;
    static uint16_t sensor_value = 1000;
    
    // 构造传感器数据消息
    sensor_msg.id = CAN_SENSOR_ID;
    sensor_msg.ide = 0;  // 标准帧
    sensor_msg.rtr = 0;  // 数据帧
    sensor_msg.dlc = 8;  // 8字节数据
    
    // 模拟传感器数据变化
    sensor_value += (HAL_GetTick() % 100) - 50;  // 随机变化
    if (sensor_value > 2000) sensor_value = 1000;
    if (sensor_value < 500) sensor_value = 1500;
    
    // 填充传感器数据
    sensor_msg.data[0] = 0x53;  // 传感器标识 'S'
    sensor_msg.data[1] = 0x45;  // 传感器标识 'E'
    sensor_msg.data[2] = (uint8_t)(sensor_value >> 8);     // 传感器值高字节
    sensor_msg.data[3] = (uint8_t)sensor_value;            // 传感器值低字节
    sensor_msg.data[4] = (uint8_t)((HAL_GetTick()/1000) >> 8);  // 时间戳高字节
    sensor_msg.data[5] = (uint8_t)(HAL_GetTick()/1000);         // 时间戳低字节
    sensor_msg.data[6] = 0x01;  // 传感器类型
    sensor_msg.data[7] = 0xA5;  // 校验码
    
    // 发送传感器数据
    if (MCP2515_SendMessage(&sensor_msg) == MCP2515_OK) {
        can_tx_counter++;
        // printf("Sensor data sent, value: %d\r\n", sensor_value);
    } else {
        can_error_counter++;
        // printf("Sensor data send failed\r\n");
    }
}

/**
  * @brief  发送控制指令 (MCP2515应用层)
  * @param  None
  * @retval None
  */
static void CAN_SendControlCommand_App(void)
{
    MCP2515_CANMessage_t control_msg;
    static uint8_t command_seq = 0;
    
    // 构造控制指令消息
    control_msg.id = CAN_CONTROL_ID;
    control_msg.ide = 0;  // 标准帧
    control_msg.rtr = 0;  // 数据帧
    control_msg.dlc = 6;  // 6字节数据
    
    // 填充控制指令数据
    control_msg.data[0] = 0x43;  // 控制标识 'C'
    control_msg.data[1] = 0x4D;  // 控制标识 'M'
    control_msg.data[2] = command_seq++;  // 指令序号
    control_msg.data[3] = (command_seq % 4) + 1;  // 指令类型 (1-4循环)
    control_msg.data[4] = (uint8_t)(HAL_GetTick() >> 16);  // 时间戳
    control_msg.data[5] = (uint8_t)(HAL_GetTick() >> 8);
    
    // 发送控制指令
    if (MCP2515_SendMessage(&control_msg) == MCP2515_OK) {
        can_tx_counter++;
        // printf("Control command sent, seq: %d, type: %d\r\n", command_seq-1, (command_seq-1) % 4 + 1);
    } else {
        can_error_counter++;
        // printf("Control command send failed\r\n");
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
    
    // printf("CAN receive task started\r\n");
    
    // 等待CAN应用初始化完成
    while (!can_app_initialized) {
        osDelay(100);
    }
    
    uint32_t debug_counter = 0;
    
    for (;;) {
        // 首先处理待处理的中断
        MCP2515_ProcessPendingInterrupt();
        
        // 检查是否有消息接收
        if (MCP2515_CheckReceive()) {
            // 接收消息
            if (MCP2515_ReceiveMessage(&received_message) == MCP2515_OK) {
                can_rx_counter++;
                
                // 打印MCP2515扩展CAN接收日志 - 接收到任何报文都打印
                printf("[MCP2515-EXT-RX] ID:0x%03X, DLC:%d, %s, Data:", 
                       (unsigned int)received_message.id, 
                       received_message.dlc,
                       received_message.ide ? "Ext" : "Std");
                if (!received_message.rtr) {
                    for (int i = 0; i < received_message.dlc && i < 8; i++) {
                        printf("%02X ", received_message.data[i]);
                    }
                } else {
                    printf("RTR ");
                }
                printf("\r\n");
                
                // 处理接收到的消息
                CAN_ProcessReceivedMessage_App(&received_message);
            } else {
                can_error_counter++;
                // printf("Message receive failed\r\n");
            }
        } else {
            // 每5秒打印一次MCP2515状态用于调试
            debug_counter++;
            if (debug_counter >= 500) {  // 500 * 10ms = 5秒
                debug_counter = 0;
                uint8_t canintf = MCP2515_ReadRegister(0x2C);  // CANINTF寄存器
                uint8_t canstat = MCP2515_ReadRegister(0x0E);  // CANSTAT寄存器
                // printf("[DEBUG] MCP2515 Status - CANINTF: 0x%02X, CANSTAT: 0x%02X\r\n", canintf, canstat);
                if (canintf & 0x03) {
                    // printf("[DEBUG] MCP2515 has pending RX interrupts but CheckReceive failed!\r\n");
                }
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
static void CAN_ProcessReceivedMessage_App(MCP2515_CANMessage_t *message)
{
    switch (message->id) {
        case CAN_HEARTBEAT_ID:
            // 处理心跳消息
            if (message->dlc >= 2 && message->data[0] == 0xAA && message->data[1] == 0x55) {
                uint32_t tx_count = (message->data[2] << 24) | (message->data[3] << 16) | 
                                   (message->data[4] << 8) | message->data[5];
                // printf("Heartbeat received, TX count: %lu\r\n", tx_count);
            }
            break;
            
        case CAN_DATA_ID:
            // 处理数据消息
            if (message->dlc >= 4 && message->data[0] == 0x12 && message->data[1] == 0x34) {
                uint16_t counter = (message->data[2] << 8) | message->data[3];
                // printf("Test data received, count: %d\r\n", counter);
            }
            break;
            
        case CAN_APP_STATUS_ID:
            // 处理状态消息
            if (message->dlc >= 8 && message->data[0] == 0x53 && message->data[1] == 0x54) {
                uint8_t sys_status = message->data[2];
                uint8_t error_flag = message->data[3];
                uint16_t tx_count = (message->data[4] << 8) | message->data[5];
                uint16_t rx_count = (message->data[6] << 8) | message->data[7];
                // printf("Status received - Sys:%d, Err:%s, TX:%d, RX:%d\r\n", 
                //        sys_status, (error_flag ? "YES" : "NO"), tx_count, rx_count);
            }
            break;
            
        case CAN_SENSOR_ID:
            // 处理传感器数据
            if (message->dlc >= 8 && message->data[0] == 0x53 && message->data[1] == 0x45) {
                uint16_t sensor_val = (message->data[2] << 8) | message->data[3];
                uint16_t timestamp = (message->data[4] << 8) | message->data[5];
                uint8_t sensor_type = message->data[6];
                // printf("Sensor data received - Value:%d, Type:%d, Time:%d\r\n", 
                //        sensor_val, sensor_type, timestamp);
            }
            break;
            
        case CAN_CONTROL_ID:
            // 处理控制指令
            if (message->dlc >= 6 && message->data[0] == 0x43 && message->data[1] == 0x4D) {
                uint8_t cmd_seq = message->data[2];
                uint8_t cmd_type = message->data[3];
                // printf("Control command received - Seq:%d, Type:%d\r\n", cmd_seq, cmd_type);
            }
            break;
            
        case 0x600:
        case 0x601:
        case 0x602:
        case 0x603:
        case 0x604:
            // 处理循环测试消息
            if (message->dlc >= 8 && message->data[0] == 0x4C && message->data[1] == 0x54) {
                uint16_t seq_num = (message->data[2] << 8) | message->data[3];
                uint32_t timestamp = (message->data[4] << 24) | (message->data[5] << 16) | 
                                   (message->data[6] << 8) | message->data[7];
                (void)seq_num;    // 抑制未使用变量警告
                (void)timestamp;  // 抑制未使用变量警告
                // printf("Loop test message received - ID:0x%03X, Seq:%d, Time:%lu\r\n", 
                //        (unsigned int)message->id, seq_num, timestamp);
            }
            break;
            
        case 0x123:
            // 处理CAN1周期性发送的消息
            if (message->dlc >= 8 && message->data[0] == 0xCA && message->data[1] == 0xFE) {
                uint16_t counter = (message->data[2] << 8) | message->data[3];
                uint32_t timestamp = (message->data[4] << 24) | (message->data[5] << 16) | 
                                   (message->data[6] << 8) | message->data[7];
                printf("[MCP2515-RX] CAN1 periodic message received - Counter:%d, Timestamp:%lu ms\r\n", 
                       counter, timestamp);
                printf("[MCP2515-RX] Message data: CA FE %02X %02X %02X %02X %02X %02X\r\n",
                       message->data[2], message->data[3], message->data[4], 
                       message->data[5], message->data[6], message->data[7]);
            } else {
                printf("[MCP2515-RX] CAN1 message with unexpected format - ID:0x123, DLC:%d\r\n", 
                       message->dlc);
            }
            break;
            
        default:
            /*
            // MCP2515相关代码已注释 - 硬件已移除
            // 检查是否为循环测试消息
            CAN_LoopTest_ProcessMCP2515Message(message);
            */
            
            // 处理其他未知消息
            printf("[MCP2515-RX] Unknown message received - ID:0x%03X, DLC:%d\r\n", 
                   (unsigned int)message->id, message->dlc);
            break;
    }
    
    /*
    // MCP2515相关代码已注释 - 硬件已移除
    // 统一处理循环测试消息（所有消息都检查）
    CAN_LoopTest_ProcessMCP2515Message(message);
    */
}

/* 中断服务程序 --------------------------------------------------------------*/

/*
// MCP2515相关函数已注释 - 硬件已移除
// 外部中断回调函数
// @brief  外部中断回调函数
// @param  GPIO_Pin: 中断引脚
// @retval None
// @note   此函数在stm32f4xx_it.c的HAL_GPIO_EXTI_Callback中调用
void CAN_EXTI_Callback(uint16_t GPIO_Pin)
{
    // if (GPIO_Pin == MCP2515_INT_Pin) {
    //     // MCP2515中断处理
    //     MCP2515_IRQHandler();
    //     
    //     // 可以在这里发送信号量通知接收任务
    //     // 或者直接在中断中处理消息接收
    // }
}

// GPIO外部中断回调函数
// @brief  GPIO外部中断回调函数
// @param  GPIO_Pin: 触发中断的引脚
// @retval None
// @note   重写HAL库的弱函数，处理MCP2515中断
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // if (GPIO_Pin == MCP2515_INT_Pin) {
    //     // 检查中断引脚的实际状态
    //     GPIO_PinState pin_state = HAL_GPIO_ReadPin(MCP2515_INT_GPIO_Port, MCP2515_INT_Pin);
    //     printf("[MCP2515-INT] Interrupt triggered on pin %d, pin state: %s\r\n", 
    //            GPIO_Pin, pin_state == GPIO_PIN_RESET ? "LOW" : "HIGH");
    //     
    //     // 调用MCP2515中断处理
    //     CAN_EXTI_Callback(GPIO_Pin);
    // }
}
*/

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
        // printf("Remote frame sent successfully, ID: 0x%03X\r\n", (unsigned int)id);
        return CAN_APP_OK;
    }
    
    can_error_counter++;
    // printf("Remote frame send failed, ID: 0x%03X\r\n", (unsigned int)id);
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
    
    // printf("Filter set successfully, ID: 0x%03X, Mask: 0x%03X\r\n", 
    //        (unsigned int)filter_id, (unsigned int)mask);
    
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
    // printf("Starting CAN application self-test...\r\n");
    
    // 检查MCP2515硬件
    if (MCP2515_SelfTest() != MCP2515_OK) {
        // printf("MCP2515 hardware test failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    // printf("MCP2515 hardware test passed\r\n");
    
    // 检查回环模式
    if (MCP2515_SetMode(MCP2515_MODE_LOOPBACK) != MCP2515_OK) {
        // printf("Set loopback mode failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    // printf("Loopback mode set successfully\r\n");
    
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
        // printf("Loopback test message send failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    // printf("Loopback test message sent successfully\r\n");
    
    // 等待并接收消息
    osDelay(100);
    
    MCP2515_CANMessage_t received_msg;
    if (MCP2515_ReceiveMessage(&received_msg) == MCP2515_OK) {
        // 验证接收到的消息
        if (received_msg.id == test_msg.id && 
            received_msg.dlc == test_msg.dlc &&
            memcmp(received_msg.data, test_msg.data, test_msg.dlc) == 0) {
            // printf("Loopback test successful!\r\n");
        } else {
            // printf("Loopback test data mismatch!\r\n");
            return CAN_APP_ERROR;
        }
    } else {
        // printf("Loopback test message receive failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    // 恢复正常模式
    if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
        // printf("Restore normal mode failed!\r\n");
        return CAN_APP_ERROR;
    }
    
    // printf("CAN application self-test completed!\r\n");
    return CAN_APP_OK;
}

/**
  * @brief  打印CAN消息信息
  * @param  prefix: 前缀字符串
  * @param  message: CAN消息指针
  * @retval None
  */
static void CAN_PrintMessage_App(const char *prefix, MCP2515_CANMessage_t *message)
{
    // printf("%s Message: ID=0x%03X, %s, %s, DLC=%d, Data=", 
    //        prefix,
    //        (unsigned int)message->id,
    //        message->ide ? "Extended" : "Standard",
    //        message->rtr ? "Remote" : "Data",
    //        message->dlc);
    // 
    // if (!message->rtr) {
    //     for (int i = 0; i < message->dlc && i < 8; i++) {
    //         printf("%02X ", message->data[i]);
    //     }
    // }
    // 
    // printf("\r\n");
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
    
    // printf("\r\n=== CAN Application Status ===\r\n");
    // printf("Initialization Status: %s\r\n", stats.initialized ? "Initialized" : "Not Initialized");
    // printf("TX Count: %lu\r\n", stats.tx_count);
    // printf("RX Count: %lu\r\n", stats.rx_count);
    // printf("Error Count: %lu\r\n", stats.error_count);
    // printf("TX Buffer Free: %d\r\n", MCP2515_CheckTransmit());
    // printf("RX Status: %s\r\n", MCP2515_CheckReceive() ? "Message Available" : "No Message");
    // 
    // // 打印MCP2515状态
    // MCP2515_PrintStatus();
    // 
    // printf("==================\r\n\r\n");
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