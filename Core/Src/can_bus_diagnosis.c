/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_bus_diagnosis.c
  * @brief          : CAN bus connection diagnosis implementation
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * This file implements CAN bus connection diagnosis functions, including:
 * 1. CAN_H and CAN_L level detection
 * 2. Termination resistance detection
 * 3. Bus load analysis
 * 4. Connection problem diagnosis and repair suggestions
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "can_bus_diagnosis.h"
#include "mcp2515.h"
#include "can_dual_node.h"
#include "can_loop_test.h"
#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/
#define DIAGNOSIS_TIMEOUT_MS    5000    // Diagnosis timeout
#define TEST_MESSAGE_COUNT      10      // Test message count
#define BUS_IDLE_THRESHOLD      100     // Bus idle threshold (ms)

/* Private variables ---------------------------------------------------------*/
static CAN_Bus_Diagnosis_t diagnosis_result;
static uint8_t diagnosis_in_progress = 0;

/* Private function prototypes -----------------------------------------------*/
static void CAN_Bus_TestConnectivity(void);
static void CAN_Bus_TestTermination(void);
static void CAN_Bus_AnalyzeBusLoad(void);
static void CAN_Bus_CheckElectricalLevels(void);
static void CAN_Bus_GenerateReport(void);

/* Public functions ----------------------------------------------------------*/

/**
  * @brief  Perform complete CAN bus diagnosis
  * @param  None
  * @retval CAN_Bus_Diagnosis_t: Diagnosis result
  */
CAN_Bus_Diagnosis_t* CAN_Bus_PerformDiagnosis(void)
{
    if (diagnosis_in_progress) {
        // printf("[WARN] Diagnosis already in progress\r\n");
        return &diagnosis_result;
    }
    
    diagnosis_in_progress = 1;
    memset(&diagnosis_result, 0, sizeof(CAN_Bus_Diagnosis_t));
    
    // printf("\r\n=== CAN Bus Comprehensive Diagnosis ===\r\n");
    // printf("Starting comprehensive CAN bus analysis...\r\n");
    
    // Step 1: Check electrical connections
    // printf("\r\n--- Step 1: Electrical Connection Check ---\r\n");
    CAN_Bus_CheckElectricalLevels();
    
    // Step 2: Test bus connectivity
    // printf("\r\n--- Step 2: Bus Connectivity Test ---\r\n");
    CAN_Bus_TestConnectivity();
    
    // Step 3: Termination resistance detection
    // printf("\r\n--- Step 3: Termination Resistance Check ---\r\n");
    CAN_Bus_TestTermination();
    
    // Step 4: Bus load analysis
    // printf("\r\n--- Step 4: Bus Load Analysis ---\r\n");
    CAN_Bus_AnalyzeBusLoad();
    
    // Step 5: Generate diagnosis report
    // printf("\r\n--- Step 5: Diagnosis Report ---\r\n");
    CAN_Bus_GenerateReport();
    
    diagnosis_in_progress = 0;
    // printf("\r\n=== Diagnosis Completed ===\r\n");
    
    return &diagnosis_result;
}

/**
  * @brief  Quick CAN bus connection check
  * @param  None
  * @retval uint8_t: 1=connection normal, 0=connection abnormal
  */
uint8_t CAN_Bus_QuickCheck(void)
{
    // printf("\r\n=== Quick CAN Bus Check ===\r\n");
    
    // Check MCP2515 status
    uint8_t canstat = MCP2515_ReadRegister(0x0E);  // CANSTAT
    uint8_t canctrl = MCP2515_ReadRegister(0x0F);  // CANCTRL
    uint8_t eflg = MCP2515_ReadRegister(0x2D);     // EFLG
    
    // printf("MCP2515 Status: CANSTAT=0x%02X, CANCTRL=0x%02X, EFLG=0x%02X\r\n", 
    //        canstat, canctrl, eflg);
    
    // Check if in Bus-Off state
    if (eflg & 0x20) {
        // printf("[ERROR] CAN bus is in Bus-Off state\r\n");
        // printf("[SOLUTION] Check CAN_H and CAN_L connections\r\n");
        // printf("[SOLUTION] Verify 120 ohm termination resistors\r\n");
        return 0;
    }
    
    // Check error counters
    uint8_t tec = MCP2515_ReadRegister(0x1C);  // TEC
    uint8_t rec = MCP2515_ReadRegister(0x1D);  // REC
    
    // printf("Error Counters: TEC=%d, REC=%d\r\n", tec, rec);
    
    if (tec > 96 || rec > 96) {
        // printf("[WARN] High error count detected\r\n");
        // printf("[SOLUTION] Check bus termination and signal integrity\r\n");
        return 0;
    }
    
    // printf("[OK] Quick check passed\r\n");
    return 1;
}

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Test CAN bus connectivity
  * @param  None
  * @retval None
  */
static void CAN_Bus_TestConnectivity(void)
{
    // printf("Testing CAN bus connectivity...\r\n");
    
    // Record error count before test
    uint8_t tec_before = MCP2515_ReadRegister(0x1C);
    uint8_t rec_before = MCP2515_ReadRegister(0x1D);
    
    // printf("Initial error counters: TEC=%d, REC=%d\r\n", tec_before, rec_before);
    
    // Send test messages
    MCP2515_CANMessage_t test_msg;
    test_msg.id = 0x7FF;  // Use highest priority ID
    test_msg.ide = 0;
    test_msg.rtr = 0;
    test_msg.dlc = 8;
    memcpy(test_msg.data, "BUSTEST\0", 8);
    
    uint8_t success_count = 0;
    uint8_t timeout_count = 0;
    
    for (int i = 0; i < TEST_MESSAGE_COUNT; i++) {
        // printf("Sending test message %d/%d...\r\n", i+1, TEST_MESSAGE_COUNT);
        
        uint8_t result = MCP2515_SendMessage(&test_msg);
        if (result == MCP2515_OK) {
            success_count++;
            // printf("  [OK] Message sent successfully\r\n");
        } else if (result == MCP2515_TIMEOUT) {
            timeout_count++;
            // printf("  [TIMEOUT] No ACK received\r\n");
        } else {
            // printf("  [ERROR] Send failed\r\n");
        }
        
        osDelay(100);  // 100ms interval
    }
    
    // Record error count after test
    uint8_t tec_after = MCP2515_ReadRegister(0x1C);
    uint8_t rec_after = MCP2515_ReadRegister(0x1D);
    
    // printf("Final error counters: TEC=%d, REC=%d\r\n", tec_after, rec_after);
    
    // Analyze results
    diagnosis_result.connectivity_test.messages_sent = TEST_MESSAGE_COUNT;
    diagnosis_result.connectivity_test.messages_acked = success_count;
    diagnosis_result.connectivity_test.timeouts = timeout_count;
    diagnosis_result.connectivity_test.tec_increase = tec_after - tec_before;
    diagnosis_result.connectivity_test.rec_increase = rec_after - rec_before;
    
    if (timeout_count == TEST_MESSAGE_COUNT) {
        // printf("[CRITICAL] All messages timed out - No other CAN nodes detected\r\n");
        // printf("[SOLUTION] Connect another CAN node or CAN analyzer\r\n");
        // printf("[SOLUTION] Check CAN_H and CAN_L wiring\r\n");
        diagnosis_result.connectivity_test.status = CAN_BUS_NO_NODES;
    } else if (timeout_count > TEST_MESSAGE_COUNT / 2) {
        // printf("[WARN] High timeout rate - Possible connection issues\r\n");
        diagnosis_result.connectivity_test.status = CAN_BUS_POOR_CONNECTION;
    } else {
        // printf("[OK] Connectivity test passed\r\n");
        diagnosis_result.connectivity_test.status = CAN_BUS_CONNECTED;
    }
}

/**
  * @brief  Test termination resistance configuration
  * @param  None
  * @retval None
  */
static void CAN_Bus_TestTermination(void)
{
    // printf("Testing CAN bus termination...\r\n");
    
    // Infer termination resistance status by sending messages and observing error counter changes
    uint8_t eflg_before = MCP2515_ReadRegister(0x2D);
    
    // Send a series of messages and monitor errors
    MCP2515_CANMessage_t term_test_msg;
    term_test_msg.id = 0x123;
    term_test_msg.ide = 0;
    term_test_msg.rtr = 0;
    term_test_msg.dlc = 1;
    term_test_msg.data[0] = 0xAA;
    
    uint8_t error_count = 0;
    for (int i = 0; i < 5; i++) {
        if (MCP2515_SendMessage(&term_test_msg) != MCP2515_OK) {
            error_count++;
        }
        osDelay(50);
    }
    
    uint8_t eflg_after = MCP2515_ReadRegister(0x2D);
    
    // Analyze termination resistance status
    if (eflg_after & 0x20) {  // Bus-Off
        // printf("[ERROR] Bus-Off detected - Missing termination resistors\r\n");
        // printf("[SOLUTION] Install 120 ohm resistors between CAN_H and CAN_L\r\n");
        diagnosis_result.termination_test.status = CAN_TERM_MISSING;
    } else if (error_count > 3) {
        // printf("[WARN] High error rate - Possible termination issues\r\n");
        // printf("[SOLUTION] Check termination resistor values (should be 120 ohm)\r\n");
        diagnosis_result.termination_test.status = CAN_TERM_INCORRECT;
    } else {
        // printf("[OK] Termination appears correct\r\n");
        diagnosis_result.termination_test.status = CAN_TERM_OK;
    }
    
    diagnosis_result.termination_test.error_flags_before = eflg_before;
    diagnosis_result.termination_test.error_flags_after = eflg_after;
    diagnosis_result.termination_test.send_errors = error_count;
}

/**
  * @brief  Analyze CAN bus load
  * @param  None
  * @retval None
  */
static void CAN_Bus_AnalyzeBusLoad(void)
{
    // printf("Analyzing CAN bus load...\r\n");
    
    uint32_t start_time = HAL_GetTick();
    uint32_t message_count = 0;
    uint32_t analysis_duration = 2000;  // 2 second analysis duration
    
    // Monitor received message count
    while ((HAL_GetTick() - start_time) < analysis_duration) {
        if (MCP2515_CheckReceive()) {
            MCP2515_CANMessage_t temp_msg;
            if (MCP2515_ReceiveMessage(&temp_msg) == MCP2515_OK) {
                message_count++;
            }
        }
        osDelay(10);
    }
    
    // Calculate message rate
    float messages_per_second = (float)message_count / (analysis_duration / 1000.0f);
    
    // printf("Bus load analysis results:\r\n");
    // printf("  Messages received: %lu in %lu ms\r\n", message_count, analysis_duration);
    // printf("  Message rate: %.1f msg/sec\r\n", messages_per_second);
    
    diagnosis_result.bus_load.messages_per_second = messages_per_second;
    diagnosis_result.bus_load.total_messages = message_count;
    
    if (messages_per_second > 100) {
        // printf("[INFO] High bus load detected\r\n");
        diagnosis_result.bus_load.load_level = CAN_LOAD_HIGH;
    } else if (messages_per_second > 10) {
        // printf("[INFO] Medium bus load\r\n");
        diagnosis_result.bus_load.load_level = CAN_LOAD_MEDIUM;
    } else if (messages_per_second > 0) {
        // printf("[INFO] Low bus load\r\n");
        diagnosis_result.bus_load.load_level = CAN_LOAD_LOW;
    } else {
        // printf("[INFO] No bus activity detected\r\n");
        diagnosis_result.bus_load.load_level = CAN_LOAD_NONE;
    }
}

/**
  * @brief  Check CAN bus electrical levels
  * @param  None
  * @retval None
  */
static void CAN_Bus_CheckElectricalLevels(void)
{
    // printf("Checking CAN bus electrical levels...\r\n");
    
    // Read MCP2515 status registers
    uint8_t canstat = MCP2515_ReadRegister(0x0E);
    uint8_t canctrl = MCP2515_ReadRegister(0x0F);
    uint8_t eflg = MCP2515_ReadRegister(0x2D);
    
    // printf("Electrical status registers:\r\n");
    // printf("  CANSTAT: 0x%02X\r\n", canstat);
    // printf("  CANCTRL: 0x%02X\r\n", canctrl);
    // printf("  EFLG: 0x%02X\r\n", eflg);
    
    // Analyze electrical status
    diagnosis_result.electrical.canstat = canstat;
    diagnosis_result.electrical.canctrl = canctrl;
    diagnosis_result.electrical.eflg = eflg;
    
    if (eflg & 0x01) {
        // printf("[WARN] Error warning flag set\r\n");
    }
    if (eflg & 0x02) {
        // printf("[WARN] Receive error warning\r\n");
    }
    if (eflg & 0x04) {
        // printf("[WARN] Transmit error warning\r\n");
    }
    if (eflg & 0x08) {
        // printf("[ERROR] Receive error passive\r\n");
    }
    if (eflg & 0x10) {
        // printf("[ERROR] Transmit error passive\r\n");
    }
    if (eflg & 0x20) {
        // printf("[CRITICAL] Bus-Off state\r\n");
    }
    
    if (eflg == 0) {
        // printf("[OK] No electrical errors detected\r\n");
        diagnosis_result.electrical.status = CAN_ELECTRICAL_OK;
    } else if (eflg & 0x20) {
        diagnosis_result.electrical.status = CAN_ELECTRICAL_BUS_OFF;
    } else if (eflg & 0x18) {
        diagnosis_result.electrical.status = CAN_ELECTRICAL_ERROR_PASSIVE;
    } else {
        diagnosis_result.electrical.status = CAN_ELECTRICAL_WARNING;
    }
}

/**
  * @brief  Generate diagnosis report
  * @param  None
  * @retval None
  */
static void CAN_Bus_GenerateReport(void)
{
    // printf("\r\n=== CAN Bus Diagnosis Report ===\r\n");
    
    // Overall status assessment
    uint8_t critical_issues = 0;
    uint8_t warnings = 0;
    
    // printf("\r\n--- Electrical Status ---\r\n");
    switch (diagnosis_result.electrical.status) {
        case CAN_ELECTRICAL_OK:
            // printf("✓ Electrical: OK\r\n");
            break;
        case CAN_ELECTRICAL_WARNING:
            // printf("⚠ Electrical: Warning\r\n");
            warnings++;
            break;
        case CAN_ELECTRICAL_ERROR_PASSIVE:
            // printf("✗ Electrical: Error Passive\r\n");
            critical_issues++;
            break;
        case CAN_ELECTRICAL_BUS_OFF:
            // printf("✗ Electrical: Bus-Off\r\n");
            critical_issues++;
            break;
    }
    
    // // printf("\r\n--- Connectivity Status ---\r\n");
    switch (diagnosis_result.connectivity_test.status) {
        case CAN_BUS_CONNECTED:
            // printf("✓ Connectivity: Good\r\n");
            break;
        case CAN_BUS_POOR_CONNECTION:
            // printf("⚠ Connectivity: Poor\r\n");
            warnings++;
            break;
        case CAN_BUS_NO_NODES:
            // printf("✗ Connectivity: No other nodes\r\n");
            critical_issues++;
            break;
    }
    
    // printf("\r\n--- Termination Status ---\r\n");
    switch (diagnosis_result.termination_test.status) {
        case CAN_TERM_OK:
            // printf("✓ Termination: OK\r\n");
            break;
        case CAN_TERM_INCORRECT:
            // printf("⚠ Termination: Incorrect\r\n");
            warnings++;
            break;
        case CAN_TERM_MISSING:
            // printf("✗ Termination: Missing\r\n");
            critical_issues++;
            break;
    }
    
    // printf("\r\n--- Bus Load ---\r\n");
    // printf("Message rate: %.1f msg/sec\r\n", diagnosis_result.bus_load.messages_per_second);
    
    // Overall assessment
    // printf("\r\n--- Overall Assessment ---\r\n");
    if (critical_issues > 0) {
        // printf("🔴 CRITICAL: %d critical issue(s) found\r\n", critical_issues);
        diagnosis_result.overall_status = CAN_DIAGNOSIS_CRITICAL;
    } else if (warnings > 0) {
        // printf("🟡 WARNING: %d warning(s) found\r\n", warnings);
        diagnosis_result.overall_status = CAN_DIAGNOSIS_WARNING;
    } else {
        // printf("🟢 GOOD: CAN bus appears healthy\r\n");
        diagnosis_result.overall_status = CAN_DIAGNOSIS_OK;
    }
    
    // Repair recommendations
    // printf("\r\n--- Repair Recommendations ---\r\n");
    if (diagnosis_result.connectivity_test.status == CAN_BUS_NO_NODES) {
        // printf("1. Connect another CAN node or CAN analyzer\r\n");
        // printf("2. Verify CAN_H and CAN_L wiring\r\n");
    }
    if (diagnosis_result.termination_test.status != CAN_TERM_OK) {
        // printf("3. Install/check 120 ohm termination resistors\r\n");
        // printf("4. Ensure resistors are at both ends of the bus\r\n");
    }
    if (diagnosis_result.electrical.status != CAN_ELECTRICAL_OK) {
        // printf("5. Check power supply to CAN transceivers\r\n");
        // printf("6. Verify signal integrity and cable quality\r\n");
    }
    
    // printf("\r\n=== End of Report ===\r\n");
}

/**
  * @brief  Get diagnosis result
  * @param  None
  * @retval CAN_Bus_Diagnosis_t*: Diagnosis result pointer
  */
CAN_Bus_Diagnosis_t* CAN_Bus_GetDiagnosisResult(void)
{
    return &diagnosis_result;
}

/**
  * @brief  Reset diagnosis result
  * @param  None
  * @retval None
  */
void CAN_Bus_ResetDiagnosis(void)
{
    memset(&diagnosis_result, 0, sizeof(CAN_Bus_Diagnosis_t));
    diagnosis_in_progress = 0;
    // printf("[INFO] CAN bus diagnosis reset\r\n");
}