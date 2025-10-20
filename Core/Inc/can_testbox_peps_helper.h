/**
  ******************************************************************************
  * @file           : can_testbox_peps_helper.h
  * @brief          : PEPS系统CAN测试辅助模块头文件
  * @version        : 1.0
  * @date           : 2024
  ******************************************************************************
  * @attention
  *
  * 本文件声明了PEPS系统CAN测试辅助功能的接口
  * 1. 提供PEPS系统CAN测试的初始化接口
  * 2. 提供停止所有周期性消息的接口
  * 3. 支持通过串口接收单字节指令控制PEPS报文发送
  *
  ******************************************************************************
  */

#ifndef CAN_TESTBOX_PEPS_HELPER_H
#define CAN_TESTBOX_PEPS_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

/* 包含头文件 */
#include "main.h"
#include "can_testbox_api.h"

/* 导出常量 */

/**
 * @brief PEPS指令类型定义
 * 根据屏幕uart通讯协议.md中的单字节指令与CAN报文矩阵对照表
 */

// PEPS控制指令 (0xA1-0xB4)
#define PEPS_CMD_SCW1_WAKEUP_START  0xA1  // 开启SCW1唤醒
#define PEPS_CMD_SCW1_WAKEUP_STOP   0xB1  // 关闭SCW1唤醒
#define PEPS_CMD_SCW2_WAKEUP_START  0xA2  // 开启SCW2唤醒
#define PEPS_CMD_SCW2_WAKEUP_STOP   0xB2  // 关闭SCW2唤醒
#define PEPS_CMD_KEY_POS_START      0xA3  // 开启钥匙位置
#define PEPS_CMD_KEY_POS_STOP       0xB3  // 关闭钥匙位置
#define PEPS_CMD_BSI_STATUS_START   0xA4  // 开启BSI状态
#define PEPS_CMD_BSI_STATUS_STOP    0xB4  // 关闭BSI状态

// 数据变体指令 (0xC1-0xE4)
#define PEPS_CMD_SCW1_WAKEUP_RESET   0xC1  // SCW1唤醒(REV=0)
#define PEPS_CMD_SCW2_WAKEUP_ACTIVE 0xC2  // SCW2唤醒(激活)
#define PEPS_CMD_KEY_POS_ABSENT     0xC3  // 钥匙不在位
#define PEPS_CMD_BSI_STATUS_ERROR   0xC4  // BSI异常状态
#define PEPS_CMD_SCW1_WAKEUP_CUSTOM1 0xD1  // SCW1唤醒(自定义1)
#define PEPS_CMD_SCW2_WAKEUP_CUSTOM1 0xD2  // SCW2唤醒(自定义1)
#define PEPS_CMD_KEY_POS_INSERTING  0xD3  // 钥匙插入中
#define PEPS_CMD_BSI_STATUS_STANDBY 0xD4  // BSI待机状态
#define PEPS_CMD_SCW1_WAKEUP_CUSTOM2 0xE1  // SCW1唤醒(自定义2)
#define PEPS_CMD_SCW2_WAKEUP_CUSTOM2 0xE2  // SCW2唤醒(自定义2)
#define PEPS_CMD_KEY_POS_REMOVING   0xE3  // 钥匙拔出中
#define PEPS_CMD_BSI_STATUS_INIT    0xE4  // BSI初始化状态

// 测试指令 (0xF1-0xF4)
#define PEPS_CMD_SCW1_FULL_TEST     0xF1  // SCW1完整测试数据
#define PEPS_CMD_SCW2_FULL_TEST     0xF2  // SCW2完整测试数据
#define PEPS_CMD_KEY_POS_FULL_TEST  0xF3  // 钥匙位置完整数据
#define PEPS_CMD_BSI_FULL_TEST      0xF4  // BSI完整测试数据

// 系统控制指令 (0xFF-0x00)
#define PEPS_CMD_STOP_ALL           0xFF  // 停止所有周期报文
#define PEPS_CMD_SYSTEM_RESET       0x00  // 系统复位

/**
 * @brief 初始化PEPS测试辅助模块
 * @note  此函数会启动串口单字符中断接收，用于接收PEPS控制指令
 * @retval HAL状态
 */
HAL_StatusTypeDef PEPS_Helper_Init(void);

/**
 * @brief 停止所有周期性消息
 * @note  此函数会停止所有已启动的PEPS周期性消息
 */
/**
 * @brief 停止所有周期性消息
 * @note 此函数会停止g_peps_periodic_handles数组中的所有周期性消息和g_scw1_handle对应的消息
 */
void PEPS_Helper_StopAllPeriodicMessages(void);

/**
 * @brief 启动周期性发送PEPS唤醒帧
 * @note  此函数会启动周期性发送PEPS唤醒帧，周期为200ms
 */
void PEPS_Helper_StartPeriodicWakeup(void);

/**
 * @brief 启动周期性发送PEPS状态帧
 * @note  此函数会启动周期性发送PEPS状态帧，周期为100ms
 */
void PEPS_Helper_StartPeriodicStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_TESTBOX_PEPS_HELPER_H */