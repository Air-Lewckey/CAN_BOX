/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_periodic_send.c
  * @brief          : STM32 CAN1 periodic send task implementation
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * This file implements STM32 built-in CAN1 periodic send functionality:
  * 1. Send a CAN message every 2 seconds
  * 2. Message contains timestamp and counter information
  * 3. Uses standard frame format
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "can_periodic_send.h"
#include "main.h"
#include "can.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define CAN_PERIODIC_SEND_ID    0x123   // CAN ID for periodic sending
#define SEND_PERIOD_MS          2000    // Send period: 2 seconds

/* Private variables ---------------------------------------------------------*/
static uint32_t send_counter = 0;       // Send counter
static uint32_t last_send_time = 0;     // Last send time
static uint8_t task_initialized = 0;    // Task initialization flag

/* External variables --------------------------------------------------------*/
extern CAN_HandleTypeDef hcan1;

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef CAN_SendPeriodicMessage(void);

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  CAN periodic send task initialization
  * @param  None
  * @retval HAL_OK: Success, HAL_ERROR: Failed
  */
HAL_StatusTypeDef CAN_PeriodicSend_Init(void)
{
    // Check if CAN1 is already started
    if ((hcan1.Instance->MSR & CAN_MSR_INAK) == 0) {
        // CAN1 is already started, no need to restart
        printf("[INFO] CAN1 already started, skipping initialization\r\n");
        
        // Initialize task state variables
        task_initialized = 1;
        send_counter = 0;
        last_send_time = 0;
        
        printf("[INFO] CAN Periodic Send initialized successfully\r\n");
        return HAL_OK;
    }
    
    // Configure CAN1 filter (accept all messages)
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 1;  // Use filter bank 1 to avoid conflicts with other modules
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;
    
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        printf("[ERROR] CAN1 filter configuration failed!\r\n");
        return HAL_ERROR;
    }
    
    // Start CAN1
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        printf("[ERROR] CAN1 start failed!\r\n");
        return HAL_ERROR;
    }
    
    // Activate CAN1 receive interrupt
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        printf("[ERROR] CAN1 RX interrupt activation failed!\r\n");
        return HAL_ERROR;
    }
    
    task_initialized = 1;
    send_counter = 0;
    last_send_time = 0;
    
    printf("[INFO] CAN1 periodic send task initialized successfully\r\n");
    printf("[INFO] Will send message every %d ms on ID 0x%03X\r\n", SEND_PERIOD_MS, CAN_PERIODIC_SEND_ID);
    
    return HAL_OK;
}

/**
  * @brief  CAN periodic send task main function
  * @param  argument: Task parameter
  * @retval None
  */
void CAN_PeriodicSend_Task(void *argument)
{
    printf("[TASK] CAN1 periodic send task started\r\n");
    
    // Wait for task initialization to complete
    while (!task_initialized) {
        osDelay(100);
    }
    
    printf("[TASK] Starting periodic CAN1 transmission (every %d ms)\r\n", SEND_PERIOD_MS);
    
    for (;;) {
        uint32_t current_time = HAL_GetTick();
        
        // Check if it's time to send
        if ((current_time - last_send_time) >= SEND_PERIOD_MS) {
            // Send periodic message
            if (CAN_SendPeriodicMessage() == HAL_OK) {
                send_counter++;
                last_send_time = current_time;
                printf("[CAN1-TX] Periodic message #%lu sent successfully at %lu ms\r\n", 
                       send_counter, current_time);
            } else {
                printf("[ERROR] CAN1 periodic message send failed at %lu ms\r\n", current_time);
            }
        }
        
        // Task delay 100ms
        osDelay(100);
    }
}

/**
  * @brief  Get periodic send statistics
  * @param  stats: Statistics structure pointer
  * @retval None
  */
void CAN_PeriodicSend_GetStats(CAN_PeriodicSend_Stats_t *stats)
{
    if (stats != NULL) {
        stats->send_count = send_counter;
        stats->last_send_time = last_send_time;
        stats->initialized = task_initialized;
        stats->next_send_time = last_send_time + SEND_PERIOD_MS;
    }
}

/**
  * @brief  Reset periodic send statistics
  * @retval None
  */
void CAN_PeriodicSend_ResetStats(void)
{
    send_counter = 0;
    last_send_time = 0;
    printf("[INFO] CAN1 periodic send statistics reset\r\n");
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Send periodic CAN message
  * @retval HAL status
  */
static HAL_StatusTypeDef CAN_SendPeriodicMessage(void)
{
    CAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    uint32_t TxMailbox;
    uint32_t current_time = HAL_GetTick();
    
    // Configure transmit header
    TxHeader.StdId = CAN_PERIODIC_SEND_ID;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = 8;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    // Construct message data
    TxData[0] = 0xCA;  // Message identifier
    TxData[1] = 0xFE;  // Message identifier
    TxData[2] = (uint8_t)(send_counter >> 8);   // Send counter high byte
    TxData[3] = (uint8_t)send_counter;          // Send counter low byte
    TxData[4] = (uint8_t)(current_time >> 24);  // Timestamp byte 3
    TxData[5] = (uint8_t)(current_time >> 16);  // Timestamp byte 2
    TxData[6] = (uint8_t)(current_time >> 8);   // Timestamp byte 1
    TxData[7] = (uint8_t)current_time;          // Timestamp byte 0
    
    // Send message
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