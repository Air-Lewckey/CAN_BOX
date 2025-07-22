/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_bus_diagnosis.h
  * @brief          : CAN总线连接诊断功能头文件
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本头文件定义CAN总线连接诊断功能的数据结构和函数接口
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
  * @brief  CAN总线连接状态枚举
  */
typedef enum {
    CAN_BUS_CONNECTED = 0,      // 连接正常
    CAN_BUS_POOR_CONNECTION,    // 连接不良
    CAN_BUS_NO_NODES           // 无其他节点
} CAN_Bus_Connection_Status_t;

/**
  * @brief  CAN终端电阻状态枚举
  */
typedef enum {
    CAN_TERM_OK = 0,           // 终端电阻正常
    CAN_TERM_INCORRECT,        // 终端电阻不正确
    CAN_TERM_MISSING           // 缺少终端电阻
} CAN_Termination_Status_t;

/**
  * @brief  CAN总线负载等级枚举
  */
typedef enum {
    CAN_LOAD_NONE = 0,         // 无负载
    CAN_LOAD_LOW,              // 低负载
    CAN_LOAD_MEDIUM,           // 中等负载
    CAN_LOAD_HIGH              // 高负载
} CAN_Bus_Load_Level_t;

/**
  * @brief  CAN电气状态枚举
  */
typedef enum {
    CAN_ELECTRICAL_OK = 0,     // 电气状态正常
    CAN_ELECTRICAL_WARNING,    // 电气警告
    CAN_ELECTRICAL_ERROR_PASSIVE,  // 错误被动状态
    CAN_ELECTRICAL_BUS_OFF     // 总线关闭状态
} CAN_Electrical_Status_t;

/**
  * @brief  总体诊断状态枚举
  */
typedef enum {
    CAN_DIAGNOSIS_OK = 0,      // 诊断正常
    CAN_DIAGNOSIS_WARNING,     // 有警告
    CAN_DIAGNOSIS_CRITICAL     // 有严重问题
} CAN_Diagnosis_Status_t;

/**
  * @brief  连通性测试结果结构体
  */
typedef struct {
    CAN_Bus_Connection_Status_t status;  // 连接状态
    uint32_t messages_sent;              // 发送消息数
    uint32_t messages_acked;             // 收到ACK的消息数
    uint32_t timeouts;                   // 超时次数
    uint8_t tec_increase;                // TEC增长量
    uint8_t rec_increase;                // REC增长量
} CAN_Connectivity_Test_t;

/**
  * @brief  终端电阻测试结果结构体
  */
typedef struct {
    CAN_Termination_Status_t status;     // 终端电阻状态
    uint8_t error_flags_before;          // 测试前错误标志
    uint8_t error_flags_after;           // 测试后错误标志
    uint8_t send_errors;                 // 发送错误次数
} CAN_Termination_Test_t;

/**
  * @brief  总线负载分析结果结构体
  */
typedef struct {
    CAN_Bus_Load_Level_t load_level;     // 负载等级
    float messages_per_second;           // 每秒消息数
    uint32_t total_messages;             // 总消息数
} CAN_Bus_Load_t;

/**
  * @brief  电气状态检查结果结构体
  */
typedef struct {
    CAN_Electrical_Status_t status;      // 电气状态
    uint8_t canstat;                     // CANSTAT寄存器值
    uint8_t canctrl;                     // CANCTRL寄存器值
    uint8_t eflg;                        // EFLG寄存器值
} CAN_Electrical_Test_t;

/**
  * @brief  CAN总线诊断结果结构体
  */
typedef struct {
    CAN_Diagnosis_Status_t overall_status;       // 总体状态
    CAN_Connectivity_Test_t connectivity_test;   // 连通性测试
    CAN_Termination_Test_t termination_test;     // 终端电阻测试
    CAN_Bus_Load_t bus_load;                     // 总线负载
    CAN_Electrical_Test_t electrical;            // 电气状态
    uint32_t diagnosis_timestamp;                // 诊断时间戳
} CAN_Bus_Diagnosis_t;

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  执行完整的CAN总线诊断
  * @param  None
  * @retval CAN_Bus_Diagnosis_t*: 诊断结果指针
  */
CAN_Bus_Diagnosis_t* CAN_Bus_PerformDiagnosis(void);

/**
  * @brief  快速CAN总线连接检查
  * @param  None
  * @retval uint8_t: 1=连接正常, 0=连接异常
  */
uint8_t CAN_Bus_QuickCheck(void);

/**
  * @brief  获取诊断结果
  * @param  None
  * @retval CAN_Bus_Diagnosis_t*: 诊断结果指针
  */
CAN_Bus_Diagnosis_t* CAN_Bus_GetDiagnosisResult(void);

/**
  * @brief  重置诊断结果
  * @param  None
  * @retval None
  */
void CAN_Bus_ResetDiagnosis(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_BUS_DIAGNOSIS_H */