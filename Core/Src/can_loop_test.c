/**
  ******************************************************************************
  * @file    can_loop_test.c
  * @brief   双CAN节点循环通信测试实现
  * @author  正点原子技术专家
  * @version V1.0
  * @date    2024-12-19
  ******************************************************************************
  * @attention
  *
  * 本文件实现双CAN节点的循环通信测试：
  * 1. STM32内置CAN先发送报文
  * 2. MCP2515接收后转发给内置CAN
  * 3. 内置CAN收到后继续下一轮发送
  * 4. 周期为1秒，带详细英文日志输出
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "can_loop_test.h"
#include "can.h"
#include "mcp2515.h"
#include "can_bus_diagnosis.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define LOOP_TEST_ID            0x123   // 循环测试消息ID
#define LOOP_TEST_PERIOD        1000    // 1秒发送周期
#define LOOP_TEST_TIMEOUT       2000    // 2秒超时

/* Private variables ---------------------------------------------------------*/
static uint32_t loop_counter = 0;           // 循环计数器
static uint32_t last_send_time = 0;         // 最后发送时间
static uint32_t last_receive_time = 0;      // 最后接收时间
static uint8_t waiting_for_response = 0;    // 等待响应标志
static uint32_t total_loops = 0;            // 总循环次数
static uint32_t successful_loops = 0;       // 成功循环次数
static uint32_t timeout_count = 0;          // 超时次数

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan1;

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef CAN_SendLoopMessage_STM32(void);
static HAL_StatusTypeDef CAN_SendLoopMessage_MCP2515(uint8_t* data, uint8_t len);
static void CAN_ProcessLoopMessage_STM32(CAN_RxHeaderTypeDef* header, uint8_t* data);
static void CAN_ProcessLoopMessage_MCP2515(MCP2515_CANMessage_t* message);
static void CAN_PrintLoopStats(void);

/* Exported functions --------------------------------------------------------*/

/**
  * @brief  初始化CAN循环测试
  * @retval HAL状态
  */
HAL_StatusTypeDef CAN_LoopTest_Init(void)
{
    // printf("\r\n=== CAN Loop Communication Test Initialization ===\r\n");
    // printf("Test Mode: STM32 CAN1 -> MCP2515 -> STM32 CAN1\r\n");
    // printf("Test Period: %d ms\r\n", LOOP_TEST_PERIOD);
    // printf("Test Message ID: 0x%03X\r\n", LOOP_TEST_ID);
    // printf("================================================\r\n\r\n");
    
    // 重置统计信息
    loop_counter = 0;
    total_loops = 0;
    successful_loops = 0;
    timeout_count = 0;
    waiting_for_response = 0;
    last_send_time = 0;
    last_receive_time = 0;
    
    // printf("[INIT] CAN Loop Test initialized successfully\r\n");
    return HAL_OK;
}

/**
  * @brief  CAN循环测试主任务
  * @param  argument: 任务参数
  * @retval None
  */
void CAN_LoopTest_Task(void *argument)
{
    uint32_t current_time;
    
    // printf("[TASK] CAN Loop Test Task started\r\n");
    
    // 等待系统稳定
    osDelay(2000);
    
    // printf("[TASK] Starting loop communication test...\r\n\r\n");
    
    for(;;)
    {
        current_time = HAL_GetTick();
        
        // 检查是否需要开始新的循环
        if (!waiting_for_response && (current_time - last_send_time) >= LOOP_TEST_PERIOD)
        {
            // 开始新的循环：STM32 CAN发送消息
            if (CAN_SendLoopMessage_STM32() == HAL_OK)
            {
                waiting_for_response = 1;
                last_send_time = current_time;
                total_loops++;
                
                printf("[LOOP #%lu] STM32 CAN1 -> Message sent to MCP2515 (Time: %lu ms)\r\n", 
               total_loops, current_time);
                // printf("[DEBUG] CAN message sent with ID: 0x%03X, waiting for MCP2515 to receive...\r\n", LOOP_TEST_ID);
            }
            else
            {
                // printf("[ERROR] STM32 CAN1 message send failed\r\n");
                // printf("[DEBUG] Check CAN1 initialization and bus connection\r\n");
            }
        }
        
        // 检查超时
        if (waiting_for_response && (current_time - last_send_time) > LOOP_TEST_TIMEOUT)
        {
            waiting_for_response = 0;
            timeout_count++;
            
            // printf("[TIMEOUT] Loop #%lu timed out after %lu ms\r\n", 
            //        total_loops, current_time - last_send_time);
            // printf("[STATS] Success Rate: %.1f%% (%lu/%lu), Timeouts: %lu\r\n",
            //        total_loops > 0 ? (float)successful_loops * 100.0f / total_loops : 0.0f,
            //        successful_loops, total_loops, timeout_count);
            
            // 连续超时诊断：当连续超时达到5次时触发诊断
            if (timeout_count >= 5 && successful_loops == 0) {
                // printf("\r\n[DIAGNOSIS] Continuous timeouts detected, starting CAN bus diagnosis...\r\n");
                CAN_Bus_Diagnosis_t* diagnosis = CAN_Bus_PerformDiagnosis();
                if (diagnosis) {
                    // printf("[DIAGNOSIS] Overall Status: %s\r\n", 
                    //        diagnosis->overall_status == CAN_DIAGNOSIS_OK ? "OK" : 
                    //        diagnosis->overall_status == CAN_DIAGNOSIS_WARNING ? "WARNING" : "CRITICAL");
                    // printf("[DIAGNOSIS] Connection: %s\r\n", 
                    //        diagnosis->connectivity_test.status == CAN_BUS_CONNECTED ? "Connected" : 
                    //        diagnosis->connectivity_test.status == CAN_BUS_POOR_CONNECTION ? "Poor Connection" : "No Nodes");
                    // printf("[DIAGNOSIS] Termination: %s\r\n", 
                    //        diagnosis->termination_test.status == CAN_TERM_OK ? "OK" : 
                    //        diagnosis->termination_test.status == CAN_TERM_INCORRECT ? "Incorrect" : "Missing");
                }
                // printf("[DIAGNOSIS] Diagnosis completed\r\n\r\n");
            }
            // printf("\r\n");
        }
        
        // 每10个循环打印一次统计信息
        if (total_loops > 0 && (total_loops % 10) == 0 && !waiting_for_response)
        {
            CAN_PrintLoopStats();
        }
        
        osDelay(50);  // 50ms检查周期
    }
}

/**
  * @brief  处理STM32 CAN接收到的循环消息
  * @param  header: 接收头
  * @param  data: 接收数据
  * @retval None
  */
void CAN_LoopTest_ProcessSTM32Message(CAN_RxHeaderTypeDef* header, uint8_t* data)
{
    if (header->StdId == LOOP_TEST_ID && waiting_for_response)
    {
        uint32_t current_time = HAL_GetTick();
        uint32_t loop_time = current_time - last_send_time;
        
        waiting_for_response = 0;
        successful_loops++;
        last_receive_time = current_time;
        
        // printf("[LOOP #%lu] STM32 CAN1 <- Message received from MCP2515 (Loop time: %lu ms)\r\n", 
        //        total_loops, loop_time);
        // printf("[SUCCESS] Loop #%lu completed successfully\r\n", total_loops);
        
        // 打印接收到的数据
        // printf("[DATA] Received: ");
        // for (int i = 0; i < header->DLC; i++)
        // {
        //     printf("%02X ", data[i]);
        // }
        // printf("\r\n\r\n");
    }
}

/**
  * @brief  处理MCP2515接收到的循环消息
  * @param  message: CAN消息
  * @retval None
  */
void CAN_LoopTest_ProcessMCP2515Message(MCP2515_CANMessage_t* message)
{
    if (message->id == LOOP_TEST_ID)
    {
        uint32_t current_time = HAL_GetTick();
        
        // printf("[RELAY] MCP2515 received message from STM32 CAN1 (Time: %lu ms)\r\n", current_time);
        
        // 打印接收到的数据
        // printf("[DATA] MCP2515 received: ");
        // for (int i = 0; i < message->dlc; i++)
        // {
        //     printf("%02X ", message->data[i]);
        // }
        // printf("\r\n");
        
        // MCP2515转发消息给STM32 CAN
        if (CAN_SendLoopMessage_MCP2515(message->data, message->dlc) == HAL_OK)
        {
            // printf("[RELAY] MCP2515 -> Message relayed to STM32 CAN1\r\n");
        }
        else
        {
            printf("[ERROR] MCP2515 message relay failed\r\n");
        }
    }
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  STM32 CAN发送循环消息
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_SendLoopMessage_STM32(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    uint32_t current_time = HAL_GetTick();
    
    // 配置发送头
    TxHeader.StdId = LOOP_TEST_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // 构造测试数据
    TxData[0] = 0xAA;  // 起始标识
    TxData[1] = 0x55;
    TxData[2] = (uint8_t)(total_loops >> 8);   // 循环计数高字节
    TxData[3] = (uint8_t)total_loops;          // 循环计数低字节
    TxData[4] = (uint8_t)(current_time >> 24); // 时间戳
    TxData[5] = (uint8_t)(current_time >> 16);
    TxData[6] = (uint8_t)(current_time >> 8);
    TxData[7] = (uint8_t)current_time;
    
    // 发送消息
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
    
    if (status == HAL_OK) {
        // Print CAN1 transmit log
        printf("[CAN1-TX] ID:0x%03X, DLC:%d, Data:", (unsigned int)TxHeader.StdId, TxHeader.DLC);
        for (int i = 0; i < TxHeader.DLC && i < 8; i++) {
            printf("%02X ", TxData[i]);
        }
        printf("\r\n");
    }
    
    return status;
}

/**
  * @brief  MCP2515发送循环消息
  * @param  data: 数据指针
  * @param  len: 数据长度
  * @retval HAL状态
  */
static HAL_StatusTypeDef CAN_SendLoopMessage_MCP2515(uint8_t* data, uint8_t len)
{
    MCP2515_CANMessage_t message;
    
    // 构造消息
    message.id = LOOP_TEST_ID;
    message.ide = 0;  // 标准帧
    message.rtr = 0;  // 数据帧
    message.dlc = len;
    
    // 复制数据
    memcpy(message.data, data, len);
    
    // 发送消息
    if (MCP2515_SendMessage(&message) == MCP2515_OK)
    {
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
  * @brief  打印循环测试统计信息
  * @retval None
  */
static void CAN_PrintLoopStats(void)
{
    float success_rate = total_loops > 0 ? (float)successful_loops * 100.0f / total_loops : 0.0f;
    uint32_t current_time = HAL_GetTick();
    
    // printf("\r\n=== CAN Loop Test Statistics ===\r\n");
    // printf("Total Loops: %lu\r\n", total_loops);
    // printf("Successful Loops: %lu\r\n", successful_loops);
    // printf("Failed Loops: %lu\r\n", total_loops - successful_loops);
    // printf("Timeout Count: %lu\r\n", timeout_count);
    // printf("Success Rate: %.1f%%\r\n", success_rate);
    // printf("Current Time: %lu ms\r\n", current_time);
    // printf("Last Send Time: %lu ms\r\n", last_send_time);
    // printf("Last Receive Time: %lu ms\r\n", last_receive_time);
    // printf("===============================\r\n\r\n");
}

/**
  * @brief  获取循环测试统计信息
  * @param  stats: 统计信息结构体指针
  * @retval None
  */
void CAN_LoopTest_GetStats(CAN_LoopTest_Stats_t* stats)
{
    if (stats != NULL)
    {
        stats->total_loops = total_loops;
        stats->successful_loops = successful_loops;
        stats->timeout_count = timeout_count;
        stats->success_rate = total_loops > 0 ? (float)successful_loops * 100.0f / total_loops : 0.0f;
        stats->last_send_time = last_send_time;
        stats->last_receive_time = last_receive_time;
        stats->waiting_for_response = waiting_for_response;
    }
}

/**
  * @brief  重置循环测试统计信息
  * @retval None
  */
void CAN_LoopTest_ResetStats(void)
{
    total_loops = 0;
    successful_loops = 0;
    timeout_count = 0;
    waiting_for_response = 0;
    last_send_time = 0;
    last_receive_time = 0;
    
    // printf("[RESET] CAN Loop Test statistics reset\r\n");
}