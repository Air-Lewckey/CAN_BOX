/**
 * @file can_testbox_peps_filter.h
 * @brief PEPS系统CAN过滤器配置模块头文件
 * @version 1.0
 * @date 2024
 */

#ifndef __CAN_TESTBOX_PEPS_FILTER_H
#define __CAN_TESTBOX_PEPS_FILTER_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含头文件 */
#include "stm32f4xx_hal.h"

/**
 * @brief 配置PEPS系统相关的CAN过滤器
 * @note 此函数应在CAN初始化后调用，用于设置只接收PEPS相关报文的过滤器
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef CAN_ConfigurePepsFilters(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_TESTBOX_PEPS_FILTER_H */