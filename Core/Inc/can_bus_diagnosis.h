/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_bus_diagnosis.h
  * @brief          : CAN bus connection diagnosis function header file
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * This header file defines data structures and function interfaces for CAN bus connection diagnosis
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_BUS_DIAGNOSIS_H
#define __CAN_BUS_DIAGNOSIS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/**
  * @brief  CAN bus connection status enumeration
  */
typedef enum {
    CAN_BUS_CONNECTED = 0,      // Connection normal
    CAN_BUS_POOR_CONNECTION,    // Poor connection
    CAN_BUS_NO_NODES           // No other nodes
} CAN_Bus_Connection_Status_t;

/**
  * @brief  CAN termination resistor status enumeration
  */
typedef enum {
    CAN_TERM_OK = 0,           // Termination resistor normal
    CAN_TERM_INCORRECT,        // Termination resistor incorrect
    CAN_TERM_MISSING           // Missing termination resistor
} CAN_Termination_Status_t;

/**
  * @brief  CAN bus load level enumeration
  */
typedef enum {
    CAN_LOAD_NONE = 0,         // No load
    CAN_LOAD_LOW,              // Low load
    CAN_LOAD_MEDIUM,           // Medium load
    CAN_LOAD_HIGH              // High load
} CAN_Bus_Load_Level_t;

/**
  * @brief  CAN electrical status enumeration
  */
typedef enum {
    CAN_ELECTRICAL_OK = 0,     // Electrical status normal
    CAN_ELECTRICAL_WARNING,    // Electrical warning
    CAN_ELECTRICAL_ERROR_PASSIVE,  // Error passive status
    CAN_ELECTRICAL_BUS_OFF     // Bus off status
} CAN_Electrical_Status_t;

/**
  * @brief  Overall diagnosis status enumeration
  */
typedef enum {
    CAN_DIAGNOSIS_OK = 0,      // Diagnosis normal
    CAN_DIAGNOSIS_WARNING,     // Has warning
    CAN_DIAGNOSIS_CRITICAL     // Has critical issues
} CAN_Diagnosis_Status_t;

/**
  * @brief  Connectivity test result structure
  */
typedef struct {
    CAN_Bus_Connection_Status_t status;  // Connection status
    uint32_t messages_sent;              // Number of messages sent
    uint32_t messages_acked;             // Number of messages acknowledged
    uint32_t timeouts;                   // Number of timeouts
    uint8_t tec_increase;                // TEC increase amount
    uint8_t rec_increase;                // REC increase amount
} CAN_Connectivity_Test_t;

/**
  * @brief  Termination resistor test result structure
  */
typedef struct {
    CAN_Termination_Status_t status;     // Termination resistor status
    uint8_t error_flags_before;          // Error flags before test
    uint8_t error_flags_after;           // Error flags after test
    uint8_t send_errors;                 // Number of transmission errors
} CAN_Termination_Test_t;

/**
  * @brief  Bus load analysis result structure
  */
typedef struct {
    CAN_Bus_Load_Level_t load_level;     // Load level
    float messages_per_second;           // Messages per second
    uint32_t total_messages;             // Total messages
} CAN_Bus_Load_t;

/**
  * @brief  Electrical status check result structure
  */
typedef struct {
    CAN_Electrical_Status_t status;      // Electrical status
    uint8_t canstat;                     // CANSTAT register value
    uint8_t canctrl;                     // CANCTRL register value
    uint8_t eflg;                        // EFLG register value
} CAN_Electrical_Test_t;

/**
  * @brief  CAN bus diagnosis result structure
  */
typedef struct {
    CAN_Diagnosis_Status_t overall_status;       // Overall status
    CAN_Connectivity_Test_t connectivity_test;   // Connectivity test
    CAN_Termination_Test_t termination_test;     // Termination resistor test
    CAN_Bus_Load_t bus_load;                     // Bus load
    CAN_Electrical_Test_t electrical;            // Electrical status
    uint32_t diagnosis_timestamp;                // Diagnosis timestamp
} CAN_Bus_Diagnosis_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  Perform complete CAN bus diagnosis
  * @param  None
  * @retval CAN_Bus_Diagnosis_t*: Diagnosis result pointer
  */
CAN_Bus_Diagnosis_t* CAN_Bus_PerformDiagnosis(void);

/**
  * @brief  Quick CAN bus connection check
  * @param  None
  * @retval uint8_t: 1=connection normal, 0=connection abnormal
  */
uint8_t CAN_Bus_QuickCheck(void);

/**
  * @brief  Get diagnosis result
  * @param  None
  * @retval CAN_Bus_Diagnosis_t*: Diagnosis result pointer
  */
CAN_Bus_Diagnosis_t* CAN_Bus_GetDiagnosisResult(void);

/**
  * @brief  Reset diagnosis result
  * @param  None
  * @retval None
  */
void CAN_Bus_ResetDiagnosis(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_BUS_DIAGNOSIS_H */