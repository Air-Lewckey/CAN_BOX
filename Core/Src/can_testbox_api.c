/**
 * @file can_testbox_api.c
 * @brief CAN测试盒专业API接口实现
 * @version 1.0
 * @date 2024
 */

#include "can_testbox_api.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* ========================= 私有变量定义 ========================= */

// CAN句柄
static CAN_HandleTypeDef *g_hcan = NULL;

// 初始化标志
static bool g_initialized = false;

// 运行状态标志
static bool g_running = false;

// 接收队列
static osMessageQueueId_t g_receive_queue = NULL;
static const osMessageQueueAttr_t g_receive_queue_attr = {
    .name = "CANTestBoxReceiveQueue"
};

// 周期性消息数组
static CAN_TestBox_PeriodicMsg_t g_periodic_messages[CAN_TESTBOX_MAX_PERIODIC_MSGS];
static uint8_t g_periodic_msg_count = 0;

// 过滤器数组
static CAN_TestBox_Filter_t g_filters[CAN_TESTBOX_FILTER_COUNT_MAX];
static uint8_t g_filter_count = 0;

// 统计信息
static CAN_TestBox_Statistics_t g_statistics = {0};

// 接收回调函数
static CAN_TestBox_RxCallback_t g_rx_callback = NULL;

// 系统时钟
static uint32_t g_system_start_time = 0;

/* ========================= 私有函数声明 ========================= */

static CAN_TestBox_Status_t CAN_TestBox_SendMessage_Internal(const CAN_TestBox_Message_t *message);
static void CAN_TestBox_ProcessPeriodicMessages(void);
static void CAN_TestBox_UpdateStatistics(void);
static uint32_t CAN_TestBox_GetTick(void);
static CAN_TestBox_Status_t CAN_TestBox_ValidateMessage(const CAN_TestBox_Message_t *message);

/* ========================= 公共API实现 ========================= */

/**
 * @brief 初始化CAN测试盒
 */
CAN_TestBox_Status_t CAN_TestBox_Init(CAN_HandleTypeDef *hcan)
{
    if (hcan == NULL) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    if (g_initialized) {
        return CAN_TESTBOX_ALREADY_EXISTS;
    }
    
    // 保存CAN句柄
    g_hcan = hcan;
    
    // 创建接收队列
    g_receive_queue = osMessageQueueNew(CAN_TESTBOX_RECEIVE_QUEUE_SIZE, sizeof(CAN_TestBox_Message_t), &g_receive_queue_attr);
    if (g_receive_queue == NULL) {
        return CAN_TESTBOX_ERROR;
    }
    
    // 初始化周期性消息数组
    memset(g_periodic_messages, 0, sizeof(g_periodic_messages));
    g_periodic_msg_count = 0;
    
    // 初始化过滤器数组
    memset(g_filters, 0, sizeof(g_filters));
    g_filter_count = 0;
    
    // 重置统计信息
    memset(&g_statistics, 0, sizeof(g_statistics));
    
    // 记录启动时间
    g_system_start_time = CAN_TestBox_GetTick();
    
    // 启动CAN
    if (HAL_CAN_Start(g_hcan) != HAL_OK) {
        osMessageQueueDelete(g_receive_queue);
        return CAN_TESTBOX_ERROR;
    }
    
    // 激活CAN接收中断
    if (HAL_CAN_ActivateNotification(g_hcan, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR) != HAL_OK) {
        HAL_CAN_Stop(g_hcan);
        osMessageQueueDelete(g_receive_queue);
        return CAN_TESTBOX_ERROR;
    }
    
    g_initialized = true;
    g_running = true;
    
    // 不打印初始化成功信息 (Don't print initialization success message)
    return CAN_TESTBOX_OK;
}

/**
 * @brief 反初始化CAN测试盒
 */
CAN_TestBox_Status_t CAN_TestBox_DeInit(void)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    // 停止所有周期性消息
    CAN_TestBox_StopAllPeriodicMessages();
    
    // 停止CAN
    HAL_CAN_Stop(g_hcan);
    
    // 删除队列
    if (g_receive_queue != NULL) {
        osMessageQueueDelete(g_receive_queue);
        g_receive_queue = NULL;
    }
    
    g_initialized = false;
    g_running = false;
    g_hcan = NULL;
    
    // 不打印反初始化信息 (Don't print deinitialization information)
    return CAN_TESTBOX_OK;
}

/* ========================= 1. 单帧事件报文发送接口 ========================= */

/**
 * @brief 发送单帧事件报文
 */
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrame(const CAN_TestBox_Message_t *message)
{
    if (!g_initialized || !g_running) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (message == NULL) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    // 验证消息参数
    CAN_TestBox_Status_t status = CAN_TestBox_ValidateMessage(message);
    if (status != CAN_TESTBOX_OK) {
        return status;
    }
    
    // 直接发送
    return CAN_TestBox_SendMessage_Internal(message);
}

/**
 * @brief 发送单帧事件报文(快速接口)
 */
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrameQuick(uint32_t id, uint8_t dlc, const uint8_t *data, bool is_extended)
{
    if (data == NULL || dlc > 8) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    CAN_TestBox_Message_t message = {
        .id = id,
        .dlc = dlc,
        .is_extended = is_extended,
        .is_remote = false,
        .timestamp = CAN_TestBox_GetTick()
    };
    
    memcpy(message.data, data, dlc);
    
    return CAN_TestBox_SendSingleFrame(&message);
}

/* ========================= 2. 单帧循环报文发送接口 ========================= */

/**
 * @brief 启动周期性消息发送
 */
CAN_TestBox_Status_t CAN_TestBox_StartPeriodicMessage(const CAN_TestBox_Message_t *message, uint32_t period_ms, uint8_t *handle_id)
{
    if (!g_initialized || !g_running) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (message == NULL || handle_id == NULL || period_ms == 0) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    if (g_periodic_msg_count >= CAN_TESTBOX_MAX_PERIODIC_MSGS) {
        return CAN_TESTBOX_QUEUE_FULL;
    }
    
    // 验证消息参数
    CAN_TestBox_Status_t status = CAN_TestBox_ValidateMessage(message);
    if (status != CAN_TESTBOX_OK) {
        return status;
    }
    
    // 查找空闲槽位
    uint8_t index = 0;
    for (index = 0; index < CAN_TESTBOX_MAX_PERIODIC_MSGS; index++) {
        if (!g_periodic_messages[index].enabled) {
            break;
        }
    }
    
    if (index >= CAN_TESTBOX_MAX_PERIODIC_MSGS) {
        return CAN_TESTBOX_QUEUE_FULL;
    }
    
    // 配置周期性消息
    g_periodic_messages[index].message = *message;
    g_periodic_messages[index].period_ms = period_ms;
    g_periodic_messages[index].enabled = true;
    g_periodic_messages[index].send_count = 0;
    g_periodic_messages[index].last_send_time = CAN_TestBox_GetTick();
    g_periodic_messages[index].handle_id = index;
    
    *handle_id = index;
    g_periodic_msg_count++;
    
    // 不打印周期性消息启动信息 (Don't print periodic message start information)
    
    return CAN_TESTBOX_OK;
}

/**
 * @brief 停止周期性消息发送
 */
CAN_TestBox_Status_t CAN_TestBox_StopPeriodicMessage(uint8_t handle_id)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (handle_id >= CAN_TESTBOX_MAX_PERIODIC_MSGS) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    if (!g_periodic_messages[handle_id].enabled) {
        return CAN_TESTBOX_NOT_FOUND;
    }
    
    g_periodic_messages[handle_id].enabled = false;
    g_periodic_msg_count--;
    
    // 不打印周期性消息停止信息 (Don't print periodic message stop information)
    
    return CAN_TESTBOX_OK;
}

/**
 * @brief 修改周期性消息的发送周期
 */
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicPeriod(uint8_t handle_id, uint32_t new_period_ms)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (handle_id >= CAN_TESTBOX_MAX_PERIODIC_MSGS || new_period_ms == 0) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    if (!g_periodic_messages[handle_id].enabled) {
        return CAN_TESTBOX_NOT_FOUND;
    }
    
    g_periodic_messages[handle_id].period_ms = new_period_ms;
    
    return CAN_TESTBOX_OK;
}

/**
 * @brief 修改周期性消息的数据内容
 */
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicData(uint8_t handle_id, const uint8_t *new_data, uint8_t dlc)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (handle_id >= CAN_TESTBOX_MAX_PERIODIC_MSGS || new_data == NULL || dlc > 8) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    if (!g_periodic_messages[handle_id].enabled) {
        return CAN_TESTBOX_NOT_FOUND;
    }
    
    g_periodic_messages[handle_id].message.dlc = dlc;
    memcpy(g_periodic_messages[handle_id].message.data, new_data, dlc);
    
    return CAN_TESTBOX_OK;
}

/**
 * @brief 停止所有周期性消息
 */
CAN_TestBox_Status_t CAN_TestBox_StopAllPeriodicMessages(void)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    for (uint8_t i = 0; i < CAN_TESTBOX_MAX_PERIODIC_MSGS; i++) {
        g_periodic_messages[i].enabled = false;
    }
    
    g_periodic_msg_count = 0;
    
    // 不打印所有周期性消息停止信息 (Don't print all periodic messages stop information)
    
    return CAN_TESTBOX_OK;
}

/* ========================= 3. 连续帧报文发送接口 ========================= */

/**
 * @brief 发送连续帧报文
 */
CAN_TestBox_Status_t CAN_TestBox_SendBurstFrames(const CAN_TestBox_BurstMsg_t *burst_config)
{
    if (!g_initialized || !g_running) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (burst_config == NULL || burst_config->burst_count == 0 || burst_config->burst_count > CAN_TESTBOX_BURST_COUNT_MAX) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    // 验证消息参数
    CAN_TestBox_Status_t status = CAN_TestBox_ValidateMessage(&burst_config->message);
    if (status != CAN_TESTBOX_OK) {
        return status;
    }
    
    CAN_TestBox_Message_t current_msg = burst_config->message;
    
    // 不打印发送连续帧信息 (Don't print burst frames sending information)
    
    for (uint16_t i = 0; i < burst_config->burst_count; i++) {
        // 发送当前消息
        status = CAN_TestBox_SendMessage_Internal(&current_msg);
        if (status != CAN_TESTBOX_OK) {
            // 不打印连续帧发送失败信息 (Don't print burst frame send failure information)
            return status;
        }
        
        // 自动递增ID
        if (burst_config->auto_increment_id) {
            current_msg.id++;
        }
        
        // 自动递增数据
        if (burst_config->auto_increment_data && current_msg.dlc > 0) {
            for (uint8_t j = 0; j < current_msg.dlc; j++) {
                current_msg.data[j]++;
            }
        }
        
        // 发送间隔延时
        if (i < burst_config->burst_count - 1 && burst_config->interval_ms > 0) {
            osDelay(burst_config->interval_ms);
        }
    }
    
    // 不打印连续帧完成信息 (Don't print burst frames completion information)
    
    return CAN_TESTBOX_OK;
}

/**
 * @brief 发送连续帧报文(快速接口)
 */
CAN_TestBox_Status_t CAN_TestBox_SendBurstFramesQuick(uint32_t id, uint8_t dlc, const uint8_t *data, 
                                                      uint16_t burst_count, uint16_t interval_ms, bool auto_increment_id)
{
    if (data == NULL || dlc > 8 || burst_count == 0) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    CAN_TestBox_BurstMsg_t burst_config = {
        .message = {
            .id = id,
            .dlc = dlc,
            .is_extended = false,
            .is_remote = false,
            .timestamp = CAN_TestBox_GetTick()
        },
        .burst_count = burst_count,
        .interval_ms = interval_ms,
        .auto_increment_id = auto_increment_id,
        .auto_increment_data = false
    };
    
    memcpy(burst_config.message.data, data, dlc);
    
    return CAN_TestBox_SendBurstFrames(&burst_config);
}

/* ========================= 4. 报文接收处理接口 ========================= */

/**
 * @brief 设置接收回调函数
 */
CAN_TestBox_Status_t CAN_TestBox_SetRxCallback(CAN_TestBox_RxCallback_t callback)
{
    g_rx_callback = callback;
    return CAN_TESTBOX_OK;
}

/**
 * @brief 从接收队列获取消息
 */
CAN_TestBox_Status_t CAN_TestBox_ReceiveMessage(CAN_TestBox_Message_t *message, uint32_t timeout_ms)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (message == NULL) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    osStatus_t status = osMessageQueueGet(g_receive_queue, message, NULL, timeout_ms);
    
    if (status == osOK) {
        return CAN_TESTBOX_OK;
    } else if (status == osErrorTimeout) {
        return CAN_TESTBOX_TIMEOUT;
    } else {
        return CAN_TESTBOX_ERROR;
    }
}

/**
 * @brief 清空接收队列
 */
CAN_TestBox_Status_t CAN_TestBox_ClearRxQueue(void)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    CAN_TestBox_Message_t dummy_msg;
    while (osMessageQueueGet(g_receive_queue, &dummy_msg, NULL, 0) == osOK) {
        // 清空队列
    }
    
    return CAN_TESTBOX_OK;
}

/* ========================= 6. 统计信息接口 ========================= */

/**
 * @brief 获取统计信息
 */
CAN_TestBox_Status_t CAN_TestBox_GetStatistics(CAN_TestBox_Statistics_t *stats)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    if (stats == NULL) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    // 更新运行时间
    g_statistics.uptime_ms = CAN_TestBox_GetTick() - g_system_start_time;
    
    *stats = g_statistics;
    
    return CAN_TESTBOX_OK;
}

/**
 * @brief 重置统计信息
 */
CAN_TestBox_Status_t CAN_TestBox_ResetStatistics(void)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    memset(&g_statistics, 0, sizeof(g_statistics));
    g_system_start_time = CAN_TestBox_GetTick();
    
    return CAN_TESTBOX_OK;
}

/* ========================= 7. 配置管理接口 ========================= */

/**
 * @brief 启动/停止CAN测试盒
 */
CAN_TestBox_Status_t CAN_TestBox_Enable(bool enable)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    g_running = enable;
    
    if (enable) {
        // 不打印启用信息 (Don't print enable information)
    } else {
        // 不打印禁用信息 (Don't print disable information)
    }
    
    return CAN_TESTBOX_OK;
}

/* ========================= 8. 诊断和调试接口 ========================= */

/**
 * @brief 获取CAN总线状态
 */
uint32_t CAN_TestBox_GetBusStatus(void)
{
    if (!g_initialized || g_hcan == NULL) {
        return 0xFFFFFFFF;
    }
    
    return g_hcan->Instance->ESR;
}

/**
 * @brief 获取最后错误信息
 */
uint32_t CAN_TestBox_GetLastError(void)
{
    return g_statistics.last_error_code;
}

/**
 * @brief 执行CAN总线自检
 */
CAN_TestBox_Status_t CAN_TestBox_SelfTest(void)
{
    if (!g_initialized) {
        return CAN_TESTBOX_NOT_INITIALIZED;
    }
    
    // 不打印自检开始信息 (Don't print self test start information)
    
    // 发送自检消息
    CAN_TestBox_Message_t test_msg = {
        .id = 0x7FF,
        .dlc = 8,
        .data = {0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA},
        .is_extended = false,
        .is_remote = false
    };
    
    CAN_TestBox_Status_t status = CAN_TestBox_SendMessage_Internal(&test_msg);
    
    if (status == CAN_TESTBOX_OK) {
        // 不打印自检通过信息 (Don't print self test pass information)
    } else {
        // 不打印自检失败信息 (Don't print self test fail information)
    }
    
    return status;
}

/* ========================= 9. 任务管理接口 ========================= */

/**
 * @brief CAN测试盒主任务
 */
void CAN_TestBox_Task(void)
{
    if (!g_initialized || !g_running) {
        return;
    }
    
    // 处理周期性消息
    CAN_TestBox_ProcessPeriodicMessages();
    
    // 更新统计信息
    CAN_TestBox_UpdateStatistics();
}

/**
 * @brief 获取任务运行状态
 */
bool CAN_TestBox_IsRunning(void)
{
    return g_running;
}

/* ========================= 私有函数实现 ========================= */

/**
 * @brief 内部消息发送函数
 */
static CAN_TestBox_Status_t CAN_TestBox_SendMessage_Internal(const CAN_TestBox_Message_t *message)
{
    CAN_TxHeaderTypeDef tx_header;
    uint32_t tx_mailbox;
    
    // 配置发送头
    if (message->is_extended) {
        tx_header.IDE = CAN_ID_EXT;
        tx_header.ExtId = message->id;
    } else {
        tx_header.IDE = CAN_ID_STD;
        tx_header.StdId = message->id;
    }
    
    tx_header.RTR = message->is_remote ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    tx_header.DLC = message->dlc;
    tx_header.TransmitGlobalTime = DISABLE;
    
    // 发送消息
    HAL_StatusTypeDef hal_status = HAL_CAN_AddTxMessage(g_hcan, &tx_header, (uint8_t*)message->data, &tx_mailbox);
    
    if (hal_status == HAL_OK) {
        g_statistics.tx_total_count++;
        g_statistics.tx_success_count++;
        
        // 按照用户要求的格式打印发送日志
        printf("[TX] ID:0x%03X, Data:", (unsigned int)message->id);
        
        if (!message->is_remote) {
            for (int i = 0; i < message->dlc; i++) {
                printf("%02X", message->data[i]);
                if (i < message->dlc - 1) printf(" ");
            }
        } else {
            printf("RTR");
        }
        
        printf(" [END]\r\n");
        
        return CAN_TESTBOX_OK;
    } else {
        g_statistics.tx_total_count++;
        g_statistics.tx_error_count++;
        g_statistics.last_error_code = hal_status;
        
        // 打印发送错误信息
        printf("[CAN-ERROR] Failed to send message - ID:0x%03X, Error:%d\r\n", 
               (unsigned int)message->id, 
               hal_status);
        
        return CAN_TESTBOX_ERROR;
    }
}

/**
 * @brief 处理周期性消息
 */
static void CAN_TestBox_ProcessPeriodicMessages(void)
{
    uint32_t current_time = CAN_TestBox_GetTick();
    
    for (uint8_t i = 0; i < CAN_TESTBOX_MAX_PERIODIC_MSGS; i++) {
        if (!g_periodic_messages[i].enabled) {
            continue;
        }
        
        // 检查是否到达发送时间
        if (current_time - g_periodic_messages[i].last_send_time >= g_periodic_messages[i].period_ms) {
            // 发送消息
            CAN_TestBox_Status_t status = CAN_TestBox_SendMessage_Internal(&g_periodic_messages[i].message);
            
            if (status == CAN_TESTBOX_OK) {
                g_periodic_messages[i].send_count++;
                g_periodic_messages[i].last_send_time = current_time;
            }
        }
    }
}

/**
 * @brief 更新统计信息
 */
static void CAN_TestBox_UpdateStatistics(void)
{
    // 更新运行时间
    g_statistics.uptime_ms = CAN_TestBox_GetTick() - g_system_start_time;
    
    // 检查CAN错误状态
    if (g_hcan != NULL) {
        uint32_t esr = g_hcan->Instance->ESR;
        if (esr & CAN_ESR_BOFF) {
            g_statistics.bus_error_count++;
        }
    }
}

/**
 * @brief 获取系统时钟
 */
static uint32_t CAN_TestBox_GetTick(void)
{
    return HAL_GetTick();
}

/**
 * @brief 验证消息参数
 */
static CAN_TestBox_Status_t CAN_TestBox_ValidateMessage(const CAN_TestBox_Message_t *message)
{
    if (message == NULL) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    if (message->dlc > 8) {
        return CAN_TESTBOX_INVALID_PARAM;
    }
    
    if (message->is_extended) {
        if (message->id > 0x1FFFFFFF) {
            return CAN_TESTBOX_INVALID_PARAM;
        }
    } else {
        if (message->id > 0x7FF) {
            return CAN_TESTBOX_INVALID_PARAM;
        }
    }
    
    return CAN_TESTBOX_OK;
}

/* ========================= CAN中断回调函数 ========================= */

/**
 * @brief CAN TestBox接收处理函数
 * @note 在CAN接收中断中调用此函数处理接收到的消息
 */
void CAN_TestBox_ProcessRxMessage(CAN_HandleTypeDef *hcan, CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data)
{
    if (hcan != g_hcan || !g_initialized) {
        return;
    }
    
    CAN_TestBox_Message_t rx_message;
    
    // 填充消息结构体
    if (rx_header->IDE == CAN_ID_EXT) {
        rx_message.id = rx_header->ExtId;
        rx_message.is_extended = true;
    } else {
        rx_message.id = rx_header->StdId;
        rx_message.is_extended = false;
    }
    
    rx_message.dlc = rx_header->DLC;
    rx_message.is_remote = (rx_header->RTR == CAN_RTR_REMOTE);
    rx_message.timestamp = CAN_TestBox_GetTick();
    
    // 复制数据
    for (uint8_t i = 0; i < rx_message.dlc && i < 8; i++) {
        rx_message.data[i] = rx_data[i];
    }
    
    // 更新统计信息
    g_statistics.rx_total_count++;
    g_statistics.rx_valid_count++;
    
    // 仅当没有设置回调时才添加到接收队列
    if (g_rx_callback == NULL) {
        if (osMessageQueuePut(g_receive_queue, &rx_message, 0, 0) != osOK) {
            // 队列满，丢弃消息
        }
    }
    
    // 调用回调函数
    if (g_rx_callback != NULL) {
        g_rx_callback(&rx_message);
    }
    
    // 不打印接收信息 (Don't print reception information)
}

/**
 * @brief CAN TestBox错误处理函数
 * @note 在CAN错误中断中调用此函数处理错误
 */
void CAN_TestBox_ProcessError(CAN_HandleTypeDef *hcan)
{
    if (hcan != g_hcan || !g_initialized) {
        return;
    }
    
    g_statistics.bus_error_count++;
    g_statistics.last_error_code = HAL_CAN_GetError(hcan);
    
    // 不打印CAN错误信息 (Don't print CAN error information)
}

/* 注意：以下回调函数已移至can_dual_node.c中统一处理，避免重复定义 */
/* 如需单独使用CAN TestBox API，请在can_dual_node.c中调用CAN_TestBox_ProcessRxMessage和CAN_TestBox_ProcessError函数 */

#if 0 // 已禁用，避免与can_dual_node.c中的回调函数冲突
/**
 * @brief CAN接收中断回调函数
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    // 已移至can_dual_node.c中统一处理
}

/**
 * @brief CAN错误回调函数
 */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    // 已移至can_dual_node.c中统一处理
}
#endif