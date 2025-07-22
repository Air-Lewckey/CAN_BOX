/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mcp2515_test_demo.c
  * @brief          : MCP2515 Receive Demo - Only receive CAN messages and print via UART
  * @author         : Assistant
  * @version        : V2.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * This file implements MCP2515 receive functionality, only receives CAN messages without sending
  * All received CAN messages will be printed in detail via UART logs
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "mcp2515_test_demo.h"
#include "mcp2515.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>

/* Private defines -----------------------------------------------------------*/
// Reception statistics period definitions (milliseconds)
#define STATS_PRINT_PERIOD_MS       5000    // Statistics print period: 5 seconds
#define RECEIVE_CHECK_PERIOD_MS     10      // Receive check period: 10 milliseconds

/* Private variables ---------------------------------------------------------*/
static MCP2515_TestDemo_Stats_t test_stats = {0};
static uint8_t mcp2515_test_initialized = 0;

/* Private function prototypes -----------------------------------------------*/
static void MCP2515_ProcessReceivedMessage(MCP2515_CANMessage_t *message);
static void MCP2515_PrintMessageDetails(MCP2515_CANMessage_t *message);
static void MCP2515_PrintStatistics(void);
static const char* MCP2515_GetMessageTypeName(uint32_t id);

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Initialize MCP2515 receive demo
  * @param  None
  * @retval HAL_OK: Success, HAL_ERROR: Failed
  */
HAL_StatusTypeDef MCP2515_TestDemo_Init(void)
{
    printf("[MCP2515-RX] Starting MCP2515 receive-only demo initialization...\r\n");
    
    // 初始化MCP2515为500Kbps波特率
    if (MCP2515_Init(MCP2515_BAUD_500K) != MCP2515_OK) {
        printf("[MCP2515-RX] ERROR: MCP2515 initialization failed!\r\n");
        return HAL_ERROR;
    }
    
    // 执行回环测试验证MCP2515硬件功能
    printf("[MCP2515-RX] Performing loopback test to verify hardware...\r\n");
    if (MCP2515_LoopbackTest() == MCP2515_OK) {
        printf("[MCP2515-RX] Loopback test PASSED - MCP2515 hardware is working correctly\r\n");
        printf("[MCP2515-RX] Switching to NORMAL mode for CAN bus reception\r\n");
        
        // Switch to normal mode to receive messages from CAN bus
        if (MCP2515_SetMode(MCP2515_MODE_NORMAL) != MCP2515_OK) {
            printf("[MCP2515-RX] WARNING: Failed to switch to normal mode\r\n");
        } else {
            printf("[MCP2515-RX] Successfully switched to normal mode\r\n");
        }
    } else {
        printf("[MCP2515-RX] Loopback test FAILED - Check MCP2515 hardware connections\r\n");
        printf("[MCP2515-RX] Continuing with normal mode for reception\r\n");
    }
    
    // Clear statistics
    memset(&test_stats, 0, sizeof(test_stats));
    test_stats.init_status = 1;
    
    mcp2515_test_initialized = 1;
    
    printf("[MCP2515-RX] MCP2515 receive-only demo initialization successful\r\n");
    printf("[MCP2515-RX] Baud rate: 500Kbps\r\n");
    printf("[MCP2515-RX] Mode: NORMAL (Receive-only mode)\r\n");
    printf("[MCP2515-RX] Ready to receive CAN messages from bus...\r\n");
    printf("[MCP2515-RX] All received messages will be printed via UART\r\n\r\n");
    
    return HAL_OK;
}

/**
  * @brief  MCP2515 receive demo main task
  * @param  argument: Task parameter
  * @retval None
  */
void MCP2515_TestDemo_Task(void *argument)
{
    uint32_t last_stats_print = 0;
    uint32_t last_diagnostic = 0;
    MCP2515_CANMessage_t received_message;
    
    printf("[MCP2515-RX] Receive task started\r\n");
    
    // Wait for MCP2515 initialization to complete
    while (!mcp2515_test_initialized) {
        osDelay(100);
    }
    
    printf("[MCP2515-RX] Starting CAN message reception...\r\n");
    
    // Run initial diagnostic after 10 seconds
    uint32_t diagnostic_delay = 10000;
    
    for (;;) {
        uint32_t current_time = HAL_GetTick();
        
        // Process pending interrupts
        MCP2515_ProcessPendingInterrupt();
        
        // Check if there are received messages
        if (MCP2515_CheckReceive()) {
            if (MCP2515_ReceiveMessage(&received_message) == MCP2515_OK) {
                // Process received message
                MCP2515_ProcessReceivedMessage(&received_message);
                test_stats.total_received++;
            } else {
                test_stats.receive_errors++;
                printf("[MCP2515-RX] ERROR: Failed to receive message\r\n");
            }
        }
        
        // Print statistics periodically
        if (current_time - last_stats_print >= STATS_PRINT_PERIOD_MS) {
            MCP2515_PrintStatistics();
            last_stats_print = current_time;
        }
        
        // Run diagnostic if no messages received after initial delay
        if (test_stats.total_received == 0 && 
            current_time > diagnostic_delay && 
            current_time - last_diagnostic >= 30000) {
            printf("\r\n[MCP2515-DIAG] No messages received, running diagnostic...\r\n");
            MCP2515_RunReceptionDiagnostic();
            last_diagnostic = current_time;
        }
        
        // Task delay
        osDelay(RECEIVE_CHECK_PERIOD_MS);  // 10ms delay for fast reception response
    }
}

/**
  * @brief  Get MCP2515 test statistics
  * @param  None
  * @retval Test statistics structure pointer
  */
MCP2515_TestDemo_Stats_t* MCP2515_TestDemo_GetStats(void)
{
    return &test_stats;
}

/**
  * @brief  Public function to run MCP2515 reception diagnostic
  * @param  None
  * @retval None
  */
void MCP2515_TestDemo_RunDiagnostic(void)
{
    MCP2515_RunReceptionDiagnostic();
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Process received CAN message
  * @param  message: Pointer to received CAN message
  * @retval None
  */
static void MCP2515_ProcessReceivedMessage(MCP2515_CANMessage_t *message)
{
    if (message == NULL) {
        printf("[MCP2515-RX] ERROR: NULL message pointer\r\n");
        return;
    }
    
    // Print detailed information of received message
    MCP2515_PrintMessageDetails(message);
    
    // Update statistics based on message ID
    const char* msg_type = MCP2515_GetMessageTypeName(message->id);
    
    // Update last reception time
    test_stats.last_rx_time = HAL_GetTick();
    
    printf("[MCP2515-RX] Received %s message (Total: %lu)\r\n", 
           msg_type, (unsigned long)test_stats.total_received);
}

/**
  * @brief  Print detailed information of received CAN message
  * @param  message: Pointer to received CAN message
  * @retval None
  */
static void MCP2515_PrintMessageDetails(MCP2515_CANMessage_t *message)
{
    printf("[MCP2515-RX] =======================================\r\n");
    printf("[MCP2515-RX] Message Details:\r\n");
    printf("[MCP2515-RX] ID: 0x%03X (%s)\r\n", 
           (unsigned int)message->id, message->ide ? "Extended" : "Standard");
    printf("[MCP2515-RX] Type: %s\r\n", 
           message->rtr ? "Remote Frame" : "Data Frame");
    printf("[MCP2515-RX] DLC: %d bytes\r\n", message->dlc);
    
    if (!message->rtr && message->dlc > 0) {
        printf("[MCP2515-RX] Data: ");
        for (int i = 0; i < message->dlc; i++) {
            printf("%02X ", message->data[i]);
        }
        printf("\r\n");
        
        // Try to parse data content
        printf("[MCP2515-RX] ASCII: ");
        for (int i = 0; i < message->dlc; i++) {
            if (message->data[i] >= 32 && message->data[i] <= 126) {
                printf("%c", message->data[i]);
            } else {
                printf(".");
            }
        }
        printf("\r\n");
    }
    
    printf("[MCP2515-RX] Timestamp: %lu ms\r\n", (unsigned long)HAL_GetTick());
    printf("[MCP2515-RX] =======================================\r\n");
}

/**
  * @brief  Print reception statistics
  * @param  None
  * @retval None
  */
static void MCP2515_PrintStatistics(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t uptime_seconds = current_time / 1000;
    uint32_t time_since_last_rx = current_time - test_stats.last_rx_time;
    
    printf("\r\n[MCP2515-RX] === Reception Statistics ===\r\n");
    printf("[MCP2515-RX] Uptime: %lu seconds\r\n", (unsigned long)uptime_seconds);
    printf("[MCP2515-RX] Total Received: %lu messages\r\n", (unsigned long)test_stats.total_received);
    
    if (test_stats.total_received > 0) {
        printf("[MCP2515-RX] Last Received: %lu ms ago\r\n", (unsigned long)time_since_last_rx);
        if (uptime_seconds > 0) {
            printf("[MCP2515-RX] Average Rate: %.2f msg/sec\r\n", 
                   (float)test_stats.total_received / uptime_seconds);
        }
    } else {
        printf("[MCP2515-RX] No messages received yet\r\n");
    }
    
    printf("[MCP2515-RX] =======================================\r\n\r\n");
}

/**
  * @brief  Get message type name based on message ID
  * @param  id: CAN message ID
  * @retval Message type name string
  */
static const char* MCP2515_GetMessageTypeName(uint32_t id)
{
    switch (id) {
        case 0x123: return "Heartbeat";
        case 0x124: return "Data";
        case 0x125: return "Status";
        case 0x126: return "Control";
        case 0x127: return "Debug";
        case 0x128: return "Sensor";
        case 0x129: return "Config";
        case 0x12A: return "Error";
        default:    return "Unknown";
    }
}

/**
  * @brief  Run comprehensive diagnostic for MCP2515 reception issues
  * @param  None
  * @retval None
  */
void MCP2515_RunReceptionDiagnostic(void)
{
    printf("\r\n========== MCP2515 Reception Diagnostic ==========\r\n");
    
    // 1. Check MCP2515 basic status
    printf("\r\n1. Basic Status Check:\r\n");
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    uint8_t current_mode = canstat & 0xE0;
    printf("   - Current Mode: 0x%02X (%s)\r\n", current_mode,
           (current_mode == MCP2515_MODE_NORMAL) ? "NORMAL" :
           (current_mode == MCP2515_MODE_CONFIG) ? "CONFIG" :
           (current_mode == MCP2515_MODE_LOOPBACK) ? "LOOPBACK" :
           (current_mode == MCP2515_MODE_LISTENONLY) ? "LISTEN-ONLY" :
           (current_mode == MCP2515_MODE_SLEEP) ? "SLEEP" : "UNKNOWN");
    
    if (current_mode != MCP2515_MODE_NORMAL) {
        printf("   [WARNING] MCP2515 not in NORMAL mode!\r\n");
    }
    
    // 2. Check interrupt configuration
    printf("\r\n2. Interrupt Configuration:\r\n");
    uint8_t caninte = MCP2515_ReadRegister(MCP2515_CANINTE);
    uint8_t canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    printf("   - CANINTE: 0x%02X\r\n", caninte);
    printf("   - CANINTF: 0x%02X\r\n", canintf);
    
    if (!(caninte & (MCP2515_INT_RX0IF | MCP2515_INT_RX1IF))) {
        printf("   [ERROR] RX interrupts not enabled!\r\n");
        printf("   [FIX] Enabling RX interrupts...\r\n");
        MCP2515_WriteRegister(MCP2515_CANINTE, MCP2515_INT_RX0IF | MCP2515_INT_RX1IF);
    } else {
        printf("   [OK] RX interrupts are enabled\r\n");
    }
    
    // 3. Check receive buffer configuration
    printf("\r\n3. Receive Buffer Configuration:\r\n");
    uint8_t rxb0ctrl = MCP2515_ReadRegister(MCP2515_RXB0CTRL);
    uint8_t rxb1ctrl = MCP2515_ReadRegister(MCP2515_RXB1CTRL);
    printf("   - RXB0CTRL: 0x%02X\r\n", rxb0ctrl);
    printf("   - RXB1CTRL: 0x%02X\r\n", rxb1ctrl);
    
    if ((rxb0ctrl & 0x60) != 0x60 || (rxb1ctrl & 0x60) != 0x60) {
        printf("   [WARNING] Filters may be blocking messages!\r\n");
        printf("   [FIX] Disabling filters to accept all messages...\r\n");
        MCP2515_WriteRegister(MCP2515_RXB0CTRL, 0x60);
        MCP2515_WriteRegister(MCP2515_RXB1CTRL, 0x60);
    } else {
        printf("   [OK] Configured to accept all messages\r\n");
    }
    
    // 4. Check error status
    printf("\r\n4. Error Status:\r\n");
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    uint8_t tec = MCP2515_ReadRegister(MCP2515_TEC);
    uint8_t rec = MCP2515_ReadRegister(MCP2515_REC);
    printf("   - Error Flags: 0x%02X\r\n", eflg);
    printf("   - TX Error Count: %d\r\n", tec);
    printf("   - RX Error Count: %d\r\n", rec);
    
    if (eflg != 0x00) {
        printf("   [WARNING] Error flags detected:\r\n");
        if (eflg & 0x80) printf("     - RX1OVR: Receive Buffer 1 Overflow\r\n");
        if (eflg & 0x40) printf("     - RX0OVR: Receive Buffer 0 Overflow\r\n");
        if (eflg & 0x20) printf("     - TXBO: Bus-Off State\r\n");
        if (eflg & 0x10) printf("     - TXEP: Transmit Error Passive\r\n");
        if (eflg & 0x08) printf("     - RXEP: Receive Error Passive\r\n");
        if (eflg & 0x04) printf("     - TXWAR: Transmit Error Warning\r\n");
        if (eflg & 0x02) printf("     - RXWAR: Receive Error Warning\r\n");
        if (eflg & 0x01) printf("     - EWARN: Error Warning\r\n");
        
        printf("   [FIX] Clearing error flags...\r\n");
        MCP2515_WriteRegister(MCP2515_EFLG, 0x00);
    } else {
        printf("   [OK] No error flags set\r\n");
    }
    
    /*
    // MCP2515相关代码已注释 - 硬件已移除
    // 5. Check hardware interrupt pin
    printf("\r\n5. Hardware Interrupt Pin:\r\n");
    GPIO_PinState int_pin = HAL_GPIO_ReadPin(MCP2515_INT_GPIO_Port, MCP2515_INT_Pin);
    printf("   - INT Pin State: %s\r\n", int_pin ? "HIGH (inactive)" : "LOW (active)");
    
    if (!int_pin) {
        printf("   [INFO] INT pin is LOW - interrupt may be pending\r\n");
        printf("   [ACTION] Processing pending interrupt...\r\n");
        MCP2515_ProcessPendingInterrupt();
    }
    */
    
    // 6. Check baud rate configuration
    printf("\r\n6. Baud Rate Configuration:\r\n");
    uint8_t cnf1 = MCP2515_ReadRegister(MCP2515_CNF1);
    uint8_t cnf2 = MCP2515_ReadRegister(MCP2515_CNF2);
    uint8_t cnf3 = MCP2515_ReadRegister(MCP2515_CNF3);
    printf("   - CNF1: 0x%02X\r\n", cnf1);
    printf("   - CNF2: 0x%02X\r\n", cnf2);
    printf("   - CNF3: 0x%02X\r\n", cnf3);
    
    // Expected values for 500kbps with 8MHz crystal
    if (cnf1 == 0x00 && cnf2 == 0x90 && cnf3 == 0x02) {
        printf("   [OK] Baud rate configured for 500kbps (8MHz crystal)\r\n");
    } else {
        printf("   [INFO] Baud rate configuration may not match expected values\r\n");
    }
    
    // 7. Manual receive check
    printf("\r\n7. Manual Receive Check:\r\n");
    uint8_t status = MCP2515_GetStatus();
    printf("   - Status Register: 0x%02X\r\n", status);
    
    if (status & 0x01) {
        printf("   [INFO] RX0IF flag set in status register\r\n");
    }
    if (status & 0x02) {
        printf("   [INFO] RX1IF flag set in status register\r\n");
    }
    
    // Force check for messages
    if (MCP2515_CheckReceive()) {
        printf("   [INFO] Message detected in receive buffer!\r\n");
        MCP2515_CANMessage_t test_msg;
        if (MCP2515_ReceiveMessage(&test_msg) == MCP2515_OK) {
            printf("   [SUCCESS] Message received during diagnostic!\r\n");
            test_stats.total_received++;
            MCP2515_ProcessReceivedMessage(&test_msg);
        }
    } else {
        printf("   [INFO] No messages in receive buffer\r\n");
    }
    
    // 8. Recommendations
    printf("\r\n8. Troubleshooting Recommendations:\r\n");
    printf("   - Verify CAN bus has 120 ohm termination resistors\r\n");
    printf("   - Check if other CAN nodes are transmitting\r\n");
    printf("   - Verify MCP2515 crystal oscillator (8MHz or 16MHz)\r\n");
    printf("   - Check CAN transceiver (TJA1050) connections\r\n");
    printf("   - Ensure CAN_H and CAN_L are properly connected\r\n");
    printf("   - Verify power supply to MCP2515 (3.3V or 5V)\r\n");
    
    // 如果RX错误计数很高或者长时间没有接收到消息，自动运行配置测试
    static uint8_t config_test_triggered = 0;
    uint32_t uptime_seconds = HAL_GetTick() / 1000;
    
    if((rec > 50) || (uptime_seconds > 10 && test_stats.total_received == 0 && !config_test_triggered)) {
        if(rec > 50) {
            printf("\r\n[AUTO-FIX] High RX error count detected, testing different configurations...\r\n");
        } else {
            printf("\r\n[AUTO-FIX] No messages received after %lu seconds, testing different configurations...\r\n", (unsigned long)uptime_seconds);
        }
        config_test_triggered = 1;  // 防止重复触发
        MCP2515_Test500KConfigs();
    }
    
    printf("\r\n========== Diagnostic Complete ==========\r\n\r\n");
}