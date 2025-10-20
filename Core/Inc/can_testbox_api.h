/**
 * @file can_testbox_api.h
 * @brief CAN测试盒专业API接口定义
 * @version 1.0
 * @date 2024
 * 
 * 本文件定义了CAN测试盒的所有对外API接口，包括：
 * - 单帧事件报文发送接口
 * - 单帧循环报文发送接口  
 * - 连续帧报文发送接口
 * - 报文接收处理接口
 * - 配置管理接口
 * - 统计信息接口
 */

#ifndef __CAN_TESTBOX_API_H
#define __CAN_TESTBOX_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "can.h"
#include <stdint.h>
#include <stdbool.h>

/* ========================= 配置宏定义 ========================= */

// 发送周期配置宏 (单位: ms)
#define CAN_TESTBOX_PERIOD_1MS      1
#define CAN_TESTBOX_PERIOD_5MS      5
#define CAN_TESTBOX_PERIOD_10MS     10
#define CAN_TESTBOX_PERIOD_20MS     20
#define CAN_TESTBOX_PERIOD_50MS     50
#define CAN_TESTBOX_PERIOD_100MS    100
#define CAN_TESTBOX_PERIOD_200MS    200
#define CAN_TESTBOX_PERIOD_500MS    500
#define CAN_TESTBOX_PERIOD_1000MS   1000
#define CAN_TESTBOX_PERIOD_2000MS   2000
#define CAN_TESTBOX_PERIOD_5000MS   5000

// 报文发送间隔配置宏 (单位: ms)
#define CAN_TESTBOX_INTERVAL_0MS    0    // 无间隔
#define CAN_TESTBOX_INTERVAL_1MS    1
#define CAN_TESTBOX_INTERVAL_2MS    2
#define CAN_TESTBOX_INTERVAL_5MS    5
#define CAN_TESTBOX_INTERVAL_10MS   10
#define CAN_TESTBOX_INTERVAL_20MS   20
#define CAN_TESTBOX_INTERVAL_50MS   50
#define CAN_TESTBOX_INTERVAL_100MS  100

// 连续帧发送配置宏
#define CAN_TESTBOX_BURST_COUNT_MAX     1000  // 最大连续发送帧数
#define CAN_TESTBOX_BURST_INTERVAL_MIN  1     // 最小发送间隔(ms)

// 发送队列配置宏
#define CAN_TESTBOX_SEND_QUEUE_SIZE     50    // 发送队列大小
#define CAN_TESTBOX_RECEIVE_QUEUE_SIZE  100   // 接收队列大小
#define CAN_TESTBOX_MAX_PERIODIC_MSGS   20    // 最大周期消息数量

// 过滤器配置宏
#define CAN_TESTBOX_FILTER_COUNT_MAX    14    // 最大过滤器数量

/* ========================= 数据结构定义 ========================= */

/**
 * @brief CAN消息结构体
 */
typedef struct {
    uint32_t id;                    // CAN ID
    uint8_t  dlc;                   // 数据长度
    uint8_t  data[8];              // 数据内容
    bool     is_extended;           // 是否为扩展帧
    bool     is_remote;             // 是否为远程帧
    uint32_t timestamp;             // 时间戳(ms)
} CAN_TestBox_Message_t;

/**
 * @brief 周期消息配置结构体
 */
typedef struct {
    CAN_TestBox_Message_t message;  // 消息内容
    uint32_t period_ms;             // 发送周期(ms)
    bool     enabled;               // 是否启用
    uint32_t send_count;            // 已发送次数
    uint32_t last_send_time;        // 上次发送时间
    uint8_t  handle_id;             // 句柄ID
} CAN_TestBox_PeriodicMsg_t;

/**
 * @brief 连续帧发送配置结构体
 */
typedef struct {
    CAN_TestBox_Message_t message;  // 消息内容
    uint16_t burst_count;           // 连续发送帧数
    uint16_t interval_ms;           // 发送间隔(ms)
    bool     auto_increment_id;     // 是否自动递增ID
    bool     auto_increment_data;   // 是否自动递增数据
} CAN_TestBox_BurstMsg_t;

/**
 * @brief CAN过滤器配置结构体
 */
typedef struct {
    uint32_t filter_id;             // 过滤ID
    uint32_t filter_mask;           // 过滤掩码
    bool     is_extended;           // 是否为扩展帧过滤
    bool     enabled;               // 是否启用
} CAN_TestBox_Filter_t;

/**
 * @brief CAN统计信息结构体
 */
typedef struct {
    uint32_t tx_total_count;        // 总发送帧数
    uint32_t tx_success_count;      // 发送成功帧数
    uint32_t tx_error_count;        // 发送错误帧数
    uint32_t rx_total_count;        // 总接收帧数
    uint32_t rx_valid_count;        // 有效接收帧数
    uint32_t rx_error_count;        // 接收错误帧数
    uint32_t bus_error_count;       // 总线错误次数
    uint32_t last_error_code;       // 最后错误代码
    uint32_t uptime_ms;             // 运行时间(ms)
} CAN_TestBox_Statistics_t;

/**
 * @brief API返回状态枚举
 */
typedef enum {
    CAN_TESTBOX_OK = 0,             // 成功
    CAN_TESTBOX_ERROR,              // 一般错误
    CAN_TESTBOX_BUSY,               // 忙碌
    CAN_TESTBOX_TIMEOUT,            // 超时
    CAN_TESTBOX_INVALID_PARAM,      // 无效参数
    CAN_TESTBOX_QUEUE_FULL,         // 队列满
    CAN_TESTBOX_QUEUE_EMPTY,        // 队列空
    CAN_TESTBOX_NOT_INITIALIZED,    // 未初始化
    CAN_TESTBOX_ALREADY_EXISTS,     // 已存在
    CAN_TESTBOX_NOT_FOUND           // 未找到
} CAN_TestBox_Status_t;

/**
 * @brief CAN接收回调函数类型定义
 * @param message: 接收到的CAN消息指针
 */
typedef void (*CAN_TestBox_RxCallback_t)(const CAN_TestBox_Message_t *message);

/* ========================= API接口声明 ========================= */

/**
 * @brief 初始化CAN测试盒
 * @param hcan: CAN句柄指针
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_Init(CAN_HandleTypeDef *hcan);

/**
 * @brief 反初始化CAN测试盒
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_DeInit(void);

/* ========================= 1. 单帧事件报文发送接口 ========================= */

/**
 * @brief 发送单帧事件报文
 * @param message: 消息指针
 * @return CAN_TestBox_Status_t: 返回状态
 * 
 * 使用示例:
 * CAN_TestBox_Message_t msg = {
 *     .id = 0x123,
 *     .dlc = 8,
 *     .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
 *     .is_extended = false,
 *     .is_remote = false
 * };
 * CAN_TestBox_SendSingleFrame(&msg);
 */
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrame(const CAN_TestBox_Message_t *message);

/**
 * @brief 发送单帧事件报文(快速接口)
 * @param id: CAN ID
 * @param dlc: 数据长度
 * @param data: 数据指针
 * @param is_extended: 是否为扩展帧
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrameQuick(uint32_t id, uint8_t dlc, const uint8_t *data, bool is_extended);

/* ========================= 2. 单帧循环报文发送接口 ========================= */

/**
 * @brief 启动周期性消息发送
 * @param message: 消息指针
 * @param period_ms: 发送周期(ms)，建议使用CAN_TESTBOX_PERIOD_xxx宏
 * @param handle_id: 返回的句柄ID指针
 * @return CAN_TestBox_Status_t: 返回状态
 * 
 * 使用示例:
 * uint8_t handle_id;
 * CAN_TestBox_Message_t msg = {.id = 0x456, .dlc = 4, .data = {0xAA, 0xBB, 0xCC, 0xDD}};
 * CAN_TestBox_StartPeriodicMessage(&msg, CAN_TESTBOX_PERIOD_100MS, &handle_id);
 */
CAN_TestBox_Status_t CAN_TestBox_StartPeriodicMessage(const CAN_TestBox_Message_t *message, uint32_t period_ms, uint8_t *handle_id);

/**
 * @brief 停止周期性消息发送
 * @param handle_id: 句柄ID
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_StopPeriodicMessage(uint8_t handle_id);

/**
 * @brief 修改周期性消息的发送周期
 * @param handle_id: 句柄ID
 * @param new_period_ms: 新的发送周期(ms)
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicPeriod(uint8_t handle_id, uint32_t new_period_ms);

/**
 * @brief 修改周期性消息的数据内容
 * @param handle_id: 句柄ID
 * @param new_data: 新的数据指针
 * @param dlc: 数据长度
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicData(uint8_t handle_id, const uint8_t *new_data, uint8_t dlc);

/**
 * @brief 停止所有周期性消息
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_StopAllPeriodicMessages(void);

/* ========================= 3. 连续帧报文发送接口 ========================= */

/**
 * @brief 发送连续帧报文
 * @param burst_config: 连续帧配置指针
 * @return CAN_TestBox_Status_t: 返回状态
 * 
 * 使用示例:
 * CAN_TestBox_BurstMsg_t burst = {
 *     .message = {.id = 0x789, .dlc = 8, .data = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88}},
 *     .burst_count = 10,
 *     .interval_ms = CAN_TESTBOX_INTERVAL_5MS,
 *     .auto_increment_id = true,
 *     .auto_increment_data = false
 * };
 * CAN_TestBox_SendBurstFrames(&burst);
 */
CAN_TestBox_Status_t CAN_TestBox_SendBurstFrames(const CAN_TestBox_BurstMsg_t *burst_config);

/**
 * @brief 发送连续帧报文(快速接口)
 * @param id: 起始CAN ID
 * @param dlc: 数据长度
 * @param data: 数据指针
 * @param burst_count: 连续发送帧数
 * @param interval_ms: 发送间隔(ms)，建议使用CAN_TESTBOX_INTERVAL_xxx宏
 * @param auto_increment_id: 是否自动递增ID
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_SendBurstFramesQuick(uint32_t id, uint8_t dlc, const uint8_t *data, 
                                                      uint16_t burst_count, uint16_t interval_ms, bool auto_increment_id);

/* ========================= 4. 报文接收处理接口 ========================= */

/**
 * @brief 设置接收回调函数
 * @param callback: 回调函数指针，设置为NULL则禁用回调
 * @return CAN_TestBox_Status_t: 返回状态
 * 
 * 使用示例:
 * void MyRxCallback(const CAN_TestBox_Message_t *message) {
 *     printf("Received CAN message ID: 0x%X\n", message->id);
 * }
 * CAN_TestBox_SetRxCallback(MyRxCallback);
 */
CAN_TestBox_Status_t CAN_TestBox_SetRxCallback(CAN_TestBox_RxCallback_t callback);

/* ========================= 5. 过滤器管理接口 ========================= */

/**
 * @brief 添加CAN过滤器
 * @param filter: 过滤器配置指针
 * @param filter_index: 返回的过滤器索引指针
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_AddFilter(const CAN_TestBox_Filter_t *filter, uint8_t *filter_index);

/**
 * @brief 移除CAN过滤器
 * @param filter_index: 过滤器索引
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_RemoveFilter(uint8_t filter_index);

/**
 * @brief 清除所有过滤器
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_ClearAllFilters(void);

/* ========================= 6. 统计信息接口 ========================= */

/**
 * @brief 获取统计信息
 * @param stats: 统计信息指针
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_GetStatistics(CAN_TestBox_Statistics_t *stats);

/**
 * @brief 重置统计信息
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_ResetStatistics(void);

/* ========================= 7. 配置管理接口 ========================= */

/**
 * @brief 设置CAN波特率
 * @param baudrate: 波特率 (125000, 250000, 500000, 1000000)
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_SetBaudrate(uint32_t baudrate);

/**
 * @brief 设置CAN工作模式
 * @param mode: 工作模式 (Normal, Loopback, Silent, SilentLoopback)
 * @return CAN_TestBox_Status_t: 返回状态
 */
typedef enum {
    CAN_TESTBOX_MODE_NORMAL = 0,
    CAN_TESTBOX_MODE_LOOPBACK,
    CAN_TESTBOX_MODE_SILENT,
    CAN_TESTBOX_MODE_SILENT_LOOPBACK
} CAN_TestBox_Mode_t;
CAN_TestBox_Status_t CAN_TestBox_SetMode(CAN_TestBox_Mode_t mode);

/**
 * @brief 启动/停止CAN测试盒
 * @param enable: true-启动, false-停止
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_Enable(bool enable);

/* ========================= 8. 诊断和调试接口 ========================= */

/**
 * @brief 获取CAN总线状态
 * @return uint32_t: 总线状态寄存器值
 */
uint32_t CAN_TestBox_GetBusStatus(void);

/**
 * @brief 获取最后错误信息
 * @return uint32_t: 错误代码
 */
uint32_t CAN_TestBox_GetLastError(void);

/**
 * @brief 执行CAN总线自检
 * @return CAN_TestBox_Status_t: 返回状态
 */
CAN_TestBox_Status_t CAN_TestBox_SelfTest(void);

/* ========================= 9. 任务管理接口 ========================= */

/**
 * @brief CAN测试盒主任务(需要在主循环或RTOS任务中调用)
 * @return void
 */
void CAN_TestBox_Task(void);

/**
 * @brief 获取任务运行状态
 * @return bool: true-运行中, false-已停止
 */
bool CAN_TestBox_IsRunning(void);

/* ========================= 内部处理函数 ========================= */

/**
 * @brief CAN TestBox接收处理函数
 * @note 在CAN接收中断中调用此函数处理接收到的消息
 * @param hcan CAN句柄指针
 * @param rx_header 接收消息头指针
 * @param rx_data 接收数据指针
 */
void CAN_TestBox_ProcessRxMessage(CAN_HandleTypeDef *hcan, CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);

/**
 * @brief CAN TestBox错误处理函数
 * @note 在CAN错误中断中调用此函数处理错误
 * @param hcan CAN句柄指针
 */
void CAN_TestBox_ProcessError(CAN_HandleTypeDef *hcan);

#ifdef __cplusplus
}
#endif

#endif /* __CAN_TESTBOX_API_H */

/**
 * @example 使用示例
 * 
 * // 1. 初始化
 * CAN_TestBox_Init(&hcan1);
 * 
 * // 2. 发送单帧事件报文
 * CAN_TestBox_SendSingleFrameQuick(0x123, 8, (uint8_t[]){0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08}, false);
 * 
 * // 3. 启动周期性消息
 * uint8_t handle;
 * CAN_TestBox_Message_t periodic_msg = {.id = 0x456, .dlc = 4, .data = {0xAA, 0xBB, 0xCC, 0xDD}};
 * CAN_TestBox_StartPeriodicMessage(&periodic_msg, CAN_TESTBOX_PERIOD_100MS, &handle);
 * 
 * // 4. 发送连续帧
 * CAN_TestBox_SendBurstFramesQuick(0x789, 8, (uint8_t[]){0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88}, 
 *                                  10, CAN_TESTBOX_INTERVAL_5MS, true);
 * 
 * // 5. 在主循环中调用任务
 * while(1) {
 *     CAN_TestBox_Task();
 *     HAL_Delay(1);
 * }
 */