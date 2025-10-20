/**
 * @file can_testbox_peps_filter.c
 * @brief PEPS系统CAN过滤器配置模块
 * @version 1.0
 * @date 2024
 */

#include "can.h"
#include <stdio.h>

/* ========================= 私有宏定义 ========================= */

// PEPS报文ID定义 - 根据SCW1_SCW2 PEPS CAN通讯矩阵
#define PEPS_WAKEUP_TX_ID_SCW1    0x05B   // SCW1 PEPS唤醒帧发送ID
#define PEPS_WAKEUP_RX_ID_SCW1    0x05A   // SCW1 PEPS唤醒帧接收ID
#define PEPS_KEY_POS_ID_SCW1      0x442   // SCW1 钥匙位置信息ID

#define PEPS_WAKEUP_TX_ID_SCW2    0x401   // SCW2 PEPS唤醒帧发送ID
#define PEPS_WAKEUP_RX_ID_SCW2    0x036   // SCW2 PEPS唤醒帧接收ID

// 通用诊断报文ID
#define PEPS_DIAG_REQ_ID         0x7A0   // PEPS诊断请求ID
#define PEPS_DIAG_RESP_ID        0x7A8   // PEPS诊断响应ID

// 自定义报文ID
#define PEPS_VERSION_ID          0x300   // PEPS版本信息ID
#define PEPS_STATUS_ID           0x301   // PEPS状态监控ID
#define PEPS_KEY_LEARN_ID        0x302   // PEPS钥匙学习ID
#define PEPS_SECURITY_ID         0x303   // PEPS网络安全ID

/* ========================= 私有函数定义 ========================= */

/**
 * @brief 配置CAN1过滤器，只接收PEPS相关报文
 * @note 使用ID列表模式配置多个过滤器，每个过滤器可以接收2个16位ID
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
static HAL_StatusTypeDef ConfigureCAN1PepsFilters(void)
{
    CAN_FilterTypeDef sFilterConfig;
    HAL_StatusTypeDef status;
    
    // 使用ID列表模式，16位过滤器
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14; // CAN2从14开始
    
    // 过滤器1: SCW1唤醒帧 (0x05A, 0x05B)
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterIdHigh = PEPS_WAKEUP_TX_ID_SCW1 << 5; // 左移5位，标准ID需要左移5位
    sFilterConfig.FilterIdLow = PEPS_WAKEUP_RX_ID_SCW1 << 5;
    sFilterConfig.FilterMaskIdHigh = PEPS_KEY_POS_ID_SCW1 << 5; // 实际上是第三个ID
    sFilterConfig.FilterMaskIdLow = PEPS_WAKEUP_TX_ID_SCW2 << 5; // 实际上是第四个ID
    
    status = HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
    if (status != HAL_OK) {
        // 不打印错误信息，避免乱码
        return status;
    }
    
    // 过滤器2: SCW2唤醒帧 (0x036, 0x401) + 诊断报文 (0x7A0)
    sFilterConfig.FilterBank = 1;
    sFilterConfig.FilterIdHigh = PEPS_WAKEUP_RX_ID_SCW2 << 5;
    sFilterConfig.FilterIdLow = PEPS_DIAG_REQ_ID << 5;
    sFilterConfig.FilterMaskIdHigh = PEPS_DIAG_RESP_ID << 5;
    sFilterConfig.FilterMaskIdLow = PEPS_VERSION_ID << 5;
    
    status = HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
    if (status != HAL_OK) {
        // 不打印错误信息，避免乱码
        return status;
    }
    
    // 过滤器3: 自定义状态报文 (0x301, 0x302, 0x303)
    sFilterConfig.FilterBank = 2;
    sFilterConfig.FilterIdHigh = PEPS_STATUS_ID << 5;
    sFilterConfig.FilterIdLow = PEPS_KEY_LEARN_ID << 5;
    sFilterConfig.FilterMaskIdHigh = PEPS_SECURITY_ID << 5;
    sFilterConfig.FilterMaskIdLow = 0x0000; // 未使用
    
    status = HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
    if (status != HAL_OK) {
        // 不打印错误信息，避免乱码
        return status;
    }
    
    // 不打印成功信息，避免乱码
    return HAL_OK;
}

/* ========================= 公共函数定义 ========================= */

/**
 * @brief 配置PEPS系统相关的CAN过滤器
 * @note 此函数应在CAN初始化后调用，用于设置只接收PEPS相关报文的过滤器
 * @retval HAL_OK: 成功, HAL_ERROR: 失败
 */
HAL_StatusTypeDef CAN_ConfigurePepsFilters(void)
{
    // 配置CAN1过滤器
    return ConfigureCAN1PepsFilters();
}