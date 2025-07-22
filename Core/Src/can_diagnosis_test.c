/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_diagnosis_test.c
  * @brief          : CAN总线诊断功能测试程序
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本文件提供CAN总线诊断功能的测试接口
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "can_diagnosis_test.h"
#include "can_bus_diagnosis.h"
#include "can_app.h"
#include "mcp2515.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void CAN_Diagnosis_PrintResults(CAN_Bus_Diagnosis_t* diagnosis);
static void CAN_Diagnosis_PrintRecommendations(CAN_Bus_Diagnosis_t* diagnosis);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  执行CAN总线诊断测试
  * @param  None
  * @retval None
  */
void CAN_Diagnosis_RunTest(void)
{
    // printf("\r\n=== CAN Bus Diagnosis Test ===\r\n");
    // printf("Starting comprehensive CAN bus diagnosis...\r\n\r\n");
    
    // 执行完整诊断
    CAN_Bus_Diagnosis_t* diagnosis = CAN_Bus_PerformDiagnosis();
    
    if (diagnosis == NULL) {
        // printf("[ERROR] Failed to perform CAN bus diagnosis\r\n");
        return;
    }
    
    // 打印诊断结果
    CAN_Diagnosis_PrintResults(diagnosis);
    
    // 打印修复建议
    CAN_Diagnosis_PrintRecommendations(diagnosis);
    
    // printf("=== Diagnosis Test Completed ===\r\n\r\n");
}

/**
  * @brief  执行快速CAN总线检查测试
  * @param  None
  * @retval None
  */
void CAN_Diagnosis_QuickTest(void)
{
    // printf("\r\n=== CAN Bus Quick Check ===\r\n");
    
    uint8_t result = CAN_Bus_QuickCheck();
    
    if (result) {
        // printf("[PASS] Quick check: CAN bus connection appears normal\r\n");
    } else {
        // printf("[FAIL] Quick check: CAN bus connection issues detected\r\n");
        // printf("Recommendation: Run full diagnosis for detailed analysis\r\n");
    }
    
    // printf("=== Quick Check Completed ===\r\n\r\n");
}

/**
  * @brief  打印详细的诊断结果
  * @param  diagnosis: 诊断结果指针
  * @retval None
  */
static void CAN_Diagnosis_PrintResults(CAN_Bus_Diagnosis_t* diagnosis)
{
    // printf("=== Detailed Diagnosis Results ===\r\n");
    
    // 总体状态
    // printf("Overall Status: ");
    switch (diagnosis->overall_status) {
        case CAN_DIAGNOSIS_OK:
            // printf("OK - No issues detected\r\n");
            break;
        case CAN_DIAGNOSIS_WARNING:
            // printf("WARNING - Minor issues detected\r\n");
            break;
        case CAN_DIAGNOSIS_CRITICAL:
            // printf("CRITICAL - Serious issues detected\r\n");
            break;
        default:
            // printf("UNKNOWN\r\n");
            break;
    }
    
    // 连通性测试结果
    // printf("\r\nConnectivity Test:\r\n");
    // printf("  Status: ");
    switch (diagnosis->connectivity_test.status) {
        case CAN_BUS_CONNECTED:
            // printf("Connected\r\n");
            break;
        case CAN_BUS_POOR_CONNECTION:
            // printf("Poor Connection\r\n");
            break;
        case CAN_BUS_NO_NODES:
            // printf("No Other Nodes\r\n");
            break;
        default:
            // printf("Unknown\r\n");
            break;
    }
    // printf("  Messages Sent: %lu\r\n", diagnosis->connectivity_test.messages_sent);
    // printf("  Messages ACKed: %lu\r\n", diagnosis->connectivity_test.messages_acked);
    // printf("  Timeouts: %lu\r\n", diagnosis->connectivity_test.timeouts);
    // printf("  TEC Increase: %u\r\n", diagnosis->connectivity_test.tec_increase);
    // printf("  REC Increase: %u\r\n", diagnosis->connectivity_test.rec_increase);
    
    // 终端电阻测试结果
    // printf("\r\nTermination Test:\r\n");
    // printf("  Status: ");
    switch (diagnosis->termination_test.status) {
        case CAN_TERM_OK:
            // printf("OK\r\n");
            break;
        case CAN_TERM_INCORRECT:
            // printf("Incorrect\r\n");
            break;
        case CAN_TERM_MISSING:
            // printf("Missing\r\n");
            break;
        default:
            // printf("Unknown\r\n");
            break;
    }
    // printf("  Error Flags Before: 0x%02X\r\n", diagnosis->termination_test.error_flags_before);
    // printf("  Error Flags After: 0x%02X\r\n", diagnosis->termination_test.error_flags_after);
    // printf("  Send Errors: %u\r\n", diagnosis->termination_test.send_errors);
    
    // 总线负载分析
    // printf("\r\nBus Load Analysis:\r\n");
    // printf("  Load Level: ");
    switch (diagnosis->bus_load.load_level) {
        case CAN_LOAD_NONE:
            // printf("None\r\n");
            break;
        case CAN_LOAD_LOW:
            // printf("Low\r\n");
            break;
        case CAN_LOAD_MEDIUM:
            // printf("Medium\r\n");
            break;
        case CAN_LOAD_HIGH:
            // printf("High\r\n");
            break;
        default:
            // printf("Unknown\r\n");
            break;
    }
    // printf("  Messages/Second: %.2f\r\n", diagnosis->bus_load.messages_per_second);
    // printf("  Total Messages: %lu\r\n", diagnosis->bus_load.total_messages);
    
    // 电气状态
    // printf("\r\nElectrical Status:\r\n");
    // printf("  Status: ");
    switch (diagnosis->electrical.status) {
        case CAN_ELECTRICAL_OK:
            // printf("OK\r\n");
            break;
        case CAN_ELECTRICAL_WARNING:
            // printf("Warning\r\n");
            break;
        case CAN_ELECTRICAL_ERROR_PASSIVE:
            // printf("Error Passive\r\n");
            break;
        case CAN_ELECTRICAL_BUS_OFF:
            // printf("Bus Off\r\n");
            break;
        default:
            // printf("Unknown\r\n");
            break;
    }
    // printf("  CANSTAT: 0x%02X\r\n", diagnosis->electrical.canstat);
    // printf("  CANCTRL: 0x%02X\r\n", diagnosis->electrical.canctrl);
    // printf("  EFLG: 0x%02X\r\n", diagnosis->electrical.eflg);
    
    // printf("  Timestamp: %lu ms\r\n", diagnosis->diagnosis_timestamp);
}

/**
  * @brief  打印修复建议
  * @param  diagnosis: 诊断结果指针
  * @retval None
  */
static void CAN_Diagnosis_PrintRecommendations(CAN_Bus_Diagnosis_t* diagnosis)
{
    // printf("\r\n=== Repair Recommendations ===\r\n");
    
    if (diagnosis->overall_status == CAN_DIAGNOSIS_OK) {
        // printf("No issues detected. CAN bus is functioning normally.\r\n");
        return;
    }
    
    // 连接问题建议
    if (diagnosis->connectivity_test.status != CAN_BUS_CONNECTED) {
        // printf("Connection Issues:\r\n");
        if (diagnosis->connectivity_test.status == CAN_BUS_NO_NODES) {
            // printf("  - Connect another CAN node or CAN analyzer\r\n");
            // printf("  - Verify CAN transceiver connections\r\n");
        } else {
            // printf("  - Check CAN_H and CAN_L wiring\r\n");
            // printf("  - Verify connector integrity\r\n");
            // printf("  - Check for loose connections\r\n");
        }
    }
    
    // 终端电阻问题建议
    if (diagnosis->termination_test.status != CAN_TERM_OK) {
        // printf("Termination Issues:\r\n");
        if (diagnosis->termination_test.status == CAN_TERM_MISSING) {
            // printf("  - Install 120 ohm resistors at both ends of CAN bus\r\n");
            // printf("  - Connect resistor between CAN_H and CAN_L\r\n");
        } else {
            // printf("  - Check termination resistor values (should be 120 ohm)\r\n");
            // printf("  - Verify only 2 termination resistors on the bus\r\n");
        }
    }
    
    // 电气问题建议
    if (diagnosis->electrical.status != CAN_ELECTRICAL_OK) {
        // printf("Electrical Issues:\r\n");
        if (diagnosis->electrical.status == CAN_ELECTRICAL_BUS_OFF) {
            // printf("  - Reset CAN controller\r\n");
            // printf("  - Check for short circuits\r\n");
            // printf("  - Verify power supply stability\r\n");
        } else if (diagnosis->electrical.status == CAN_ELECTRICAL_ERROR_PASSIVE) {
            // printf("  - Check for noise on CAN lines\r\n");
            // printf("  - Verify cable shielding\r\n");
            // printf("  - Check ground connections\r\n");
        }
    }
    
    // 总线负载建议
    if (diagnosis->bus_load.load_level == CAN_LOAD_HIGH) {
        // printf("Bus Load Issues:\r\n");
        // printf("  - Reduce message transmission frequency\r\n");
        // printf("  - Optimize message priorities\r\n");
        // printf("  - Consider using higher baud rate\r\n");
    }
}