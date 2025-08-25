# CAN测试盒专业API使用指南

## 概述

本文档详细介绍了CAN测试盒专业API的使用方法，包括接口位置、使用示例和最佳实践。该API提供了完整的CAN报文收发功能，适用于专业的CAN总线测试和验证工作。

## 文件结构

### 核心文件位置

```
Core/
├── Inc/
│   └── can_testbox_api.h          # API接口头文件
└── Src/
    ├── can_testbox_api.c           # API接口实现文件
    └── can_testbox_example.c       # 使用示例文件
```

### 文件说明

- **can_testbox_api.h**: 包含所有API接口声明、数据结构定义和配置宏
- **can_testbox_api.c**: API接口的具体实现代码
- **can_testbox_example.c**: 详细的使用示例和最佳实践

## API接口分类

### 1. 单帧事件报文发送接口

#### 接口位置
```c
// 文件: Core/Inc/can_testbox_api.h (第133-155行)
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrame(const CAN_TestBox_Message_t *message);
CAN_TestBox_Status_t CAN_TestBox_SendSingleFrameQuick(uint32_t id, uint8_t dlc, const uint8_t *data, bool is_extended);
```

#### 使用方法
```c
// 方法1: 使用完整结构体
CAN_TestBox_Message_t msg = {
    .id = 0x123,
    .dlc = 8,
    .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08},
    .is_extended = false,
    .is_remote = false
};
CAN_TestBox_SendSingleFrame(&msg);

// 方法2: 快速接口
uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD};
CAN_TestBox_SendSingleFrameQuick(0x456, 4, data, false);
```

### 2. 单帧循环报文发送接口

#### 接口位置
```c
// 文件: Core/Inc/can_testbox_api.h (第157-200行)
CAN_TestBox_Status_t CAN_TestBox_StartPeriodicMessage(const CAN_TestBox_Message_t *message, uint32_t period_ms, uint8_t *handle_id);
CAN_TestBox_Status_t CAN_TestBox_StopPeriodicMessage(uint8_t handle_id);
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicPeriod(uint8_t handle_id, uint32_t new_period_ms);
CAN_TestBox_Status_t CAN_TestBox_ModifyPeriodicData(uint8_t handle_id, const uint8_t *new_data, uint8_t dlc);
CAN_TestBox_Status_t CAN_TestBox_StopAllPeriodicMessages(void);
```

#### 使用方法
```c
// 启动周期性消息
uint8_t handle_id;
CAN_TestBox_Message_t periodic_msg = {
    .id = 0x100,
    .dlc = 8,
    .data = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80}
};
CAN_TestBox_StartPeriodicMessage(&periodic_msg, CAN_TESTBOX_PERIOD_100MS, &handle_id);

// 修改发送周期
CAN_TestBox_ModifyPeriodicPeriod(handle_id, CAN_TESTBOX_PERIOD_200MS);

// 修改数据内容
uint8_t new_data[] = {0x99, 0x88, 0x77, 0x66};
CAN_TestBox_ModifyPeriodicData(handle_id, new_data, 4);

// 停止周期性消息
CAN_TestBox_StopPeriodicMessage(handle_id);
```

#### 可用周期配置宏
```c
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
```

### 3. 连续帧报文发送接口

#### 接口位置
```c
// 文件: Core/Inc/can_testbox_api.h (第202-230行)
CAN_TestBox_Status_t CAN_TestBox_SendBurstFrames(const CAN_TestBox_BurstMsg_t *burst_config);
CAN_TestBox_Status_t CAN_TestBox_SendBurstFramesQuick(uint32_t id, uint8_t dlc, const uint8_t *data, 
                                                      uint16_t burst_count, uint16_t interval_ms, bool auto_increment_id);
```

#### 使用方法
```c
// 方法1: 使用完整配置
CAN_TestBox_BurstMsg_t burst_config = {
    .message = {
        .id = 0x400,
        .dlc = 8,
        .data = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08}
    },
    .burst_count = 10,
    .interval_ms = CAN_TESTBOX_INTERVAL_5MS,
    .auto_increment_id = true,
    .auto_increment_data = false
};
CAN_TestBox_SendBurstFrames(&burst_config);

// 方法2: 快速接口
uint8_t burst_data[] = {0xAA, 0xBB, 0xCC, 0xDD};
CAN_TestBox_SendBurstFramesQuick(0x500, 4, burst_data, 5, CAN_TESTBOX_INTERVAL_10MS, true);
```

#### 可用间隔配置宏
```c
#define CAN_TESTBOX_INTERVAL_0MS    0    // 无间隔
#define CAN_TESTBOX_INTERVAL_1MS    1
#define CAN_TESTBOX_INTERVAL_2MS    2
#define CAN_TESTBOX_INTERVAL_5MS    5
#define CAN_TESTBOX_INTERVAL_10MS   10
#define CAN_TESTBOX_INTERVAL_20MS   20
#define CAN_TESTBOX_INTERVAL_50MS   50
#define CAN_TESTBOX_INTERVAL_100MS  100
```

## 报文发送队列设计

### 队列配置
```c
// 文件: Core/Inc/can_testbox_api.h (第45-49行)
#define CAN_TESTBOX_SEND_QUEUE_SIZE     50    // 发送队列大小
#define CAN_TESTBOX_RECEIVE_QUEUE_SIZE  100   // 接收队列大小
#define CAN_TESTBOX_MAX_PERIODIC_MSGS   20    // 最大周期消息数量
```

### 队列管理特性
- **发送队列**: 支持50个待发送消息的缓存
- **接收队列**: 支持100个接收消息的缓存
- **周期性消息**: 最多支持20个并发的周期性消息
- **自动管理**: 队列满时自动丢弃最旧的消息

## 完整功能列表

### 已实现的核心功能

1. **消息发送功能**
   - 单帧事件报文发送
   - 周期性报文发送
   - 连续帧报文发送
   - 扩展帧和标准帧支持
   - 远程帧支持

2. **消息接收功能**
   - 中断驱动接收
   - 接收队列管理
   - 回调函数支持
   - 消息过滤

3. **配置管理功能**
   - 波特率配置
   - 工作模式配置
   - 过滤器管理
   - 启动/停止控制

4. **统计和诊断功能**
   - 发送/接收统计
   - 错误统计
   - 总线状态监控
   - 自检功能

### 建议补充的专业功能

#### 1. 高级测试功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_StartStressTest(uint32_t duration_ms, uint32_t load_percentage);
CAN_TestBox_Status_t CAN_TestBox_StartLatencyTest(uint32_t test_id, uint32_t timeout_ms);
CAN_TestBox_Status_t CAN_TestBox_StartThroughputTest(uint32_t duration_ms);
```

#### 2. 错误注入功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_InjectBusError(CAN_ErrorType_t error_type);
CAN_TestBox_Status_t CAN_TestBox_InjectFrameError(uint32_t frame_id, CAN_FrameErrorType_t error_type);
CAN_TestBox_Status_t CAN_TestBox_SimulateBusOff(uint32_t duration_ms);
```

#### 3. 数据记录功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_StartDataLogging(const char* filename);
CAN_TestBox_Status_t CAN_TestBox_StopDataLogging(void);
CAN_TestBox_Status_t CAN_TestBox_ExportStatistics(const char* filename);
```

#### 4. 网络层支持
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_SendISOTP(uint32_t id, const uint8_t* data, uint16_t length);
CAN_TestBox_Status_t CAN_TestBox_ReceiveISOTP(uint32_t id, uint8_t* data, uint16_t* length, uint32_t timeout_ms);
CAN_TestBox_Status_t CAN_TestBox_SendJ1939(uint32_t pgn, uint8_t sa, uint8_t da, const uint8_t* data, uint8_t length);
```

#### 5. 时间同步功能
```c
// 建议添加的接口
CAN_TestBox_Status_t CAN_TestBox_EnableTimeSync(bool enable);
CAN_TestBox_Status_t CAN_TestBox_SetTimeReference(uint64_t reference_time_us);
CAN_TestBox_Status_t CAN_TestBox_GetPreciseTimestamp(uint64_t* timestamp_us);
```

## 使用示例

### 基本初始化
```c
#include "can_testbox_api.h"

void CAN_TestBox_Setup(void)
{
    // 1. 初始化CAN测试盒
    CAN_TestBox_Status_t status = CAN_TestBox_Init(&hcan1);
    if (status != CAN_TESTBOX_OK) {
        printf("CAN TestBox initialization failed: %d\r\n", status);
        return;
    }
    
    // 2. 设置接收回调
    CAN_TestBox_SetRxCallback(My_RxCallback);
    
    // 3. 启动测试盒
    CAN_TestBox_Enable(true);
    
    printf("CAN TestBox ready\r\n");
}
```

### 主循环集成
```c
void Main_Loop(void)
{
    while (1) {
        // 调用CAN测试盒主任务
        CAN_TestBox_Task();
        
        // 其他应用逻辑
        // ...
        
        // 延时
        HAL_Delay(1);
    }
}
```

### FreeRTOS任务集成
```c
void CAN_TestBox_Task_Thread(void *argument)
{
    // 初始化
    CAN_TestBox_Setup();
    
    for (;;) {
        // 调用CAN测试盒主任务
        CAN_TestBox_Task();
        
        // 任务延时
        osDelay(1);
    }
}
```

## 配置建议

### 1. 内存配置
- 建议堆栈大小: 至少2KB
- FreeRTOS任务优先级: osPriorityNormal或更高
- 队列内存: 约8KB (发送队列50 + 接收队列100) × 消息大小

### 2. 中断配置
- CAN接收中断优先级: 建议设置为较高优先级
- 确保CAN中断不被其他中断长时间阻塞

### 3. 时钟配置
- 确保CAN时钟配置正确
- 建议使用标准波特率: 125K, 250K, 500K, 1M

## 错误处理

### 常见错误码
```c
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
```

### 错误处理建议
```c
CAN_TestBox_Status_t status = CAN_TestBox_SendSingleFrame(&msg);
switch (status) {
    case CAN_TESTBOX_OK:
        // 发送成功
        break;
    case CAN_TESTBOX_BUSY:
        // 总线忙，稍后重试
        HAL_Delay(1);
        break;
    case CAN_TESTBOX_INVALID_PARAM:
        // 参数错误，检查消息内容
        printf("Invalid message parameters\r\n");
        break;
    default:
        // 其他错误
        printf("Send failed: %d\r\n", status);
        break;
}
```

## 性能优化建议

1. **批量操作**: 使用连续帧接口而不是多次单帧发送
2. **队列管理**: 定期清理接收队列，避免内存泄漏
3. **中断优化**: 在接收回调中避免长时间操作
4. **统计监控**: 定期检查统计信息，及时发现问题

## 总结

CAN测试盒API提供了完整的专业级CAN总线测试功能，支持：

- ✅ 单帧事件报文发送
- ✅ 周期性报文发送
- ✅ 连续帧报文发送
- ✅ 完整的配置宏支持
- ✅ 专业的队列管理
- ✅ 统计和诊断功能
- ✅ 详细的使用示例

通过合理使用这些接口，可以构建功能强大的CAN总线测试和验证系统。建议根据具体需求，逐步添加高级功能如错误注入、网络层支持等。