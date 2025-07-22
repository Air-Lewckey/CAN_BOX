/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : can_diagnosis_test.h
  * @brief          : CAN总线诊断功能测试程序头文件
  * @author         : Assistant
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本头文件定义CAN总线诊断功能测试接口
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_DIAGNOSIS_TEST_H
#define __CAN_DIAGNOSIS_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

/**
  * @brief  执行CAN总线诊断测试
  * @param  None
  * @retval None
  */
void CAN_Diagnosis_RunTest(void);

/**
  * @brief  执行快速CAN总线检查测试
  * @param  None
  * @retval None
  */
void CAN_Diagnosis_QuickTest(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_DIAGNOSIS_TEST_H */