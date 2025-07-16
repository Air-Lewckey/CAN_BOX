# STM32F407 + MCP2515 CAN通信系统软件说明书

## 📖 说明书概述

本说明书帮助您快速理解和使用STM32F407与MCP2515 CAN控制器通信系统：
- 理解代码架构和核心函数
- 掌握函数调用方法
- 学会如何修改和扩展功能
- 避免常见的开发陷阱

## 🏗️ 代码架构总览

### 分层架构设计
```
应用层 (can_app.c/h)     ← 您主要修改的地方
    ↓
驱动层 (mcp2515.c/h)     ← 一般不需要修改
    ↓
HAL层 (STM32 HAL库)      ← 系统自动生成
    ↓
硬件层 (STM32F407)       ← 硬件平台
```

### 文件功能说明
| 文件名 | 功能 | 是否可修改 | 修改频率 |
|--------|------|------------|----------|
| `mcp2515.h/c` | MCP2515底层驱动 | ❌ 不建议 | 很少 |
| `can_app.h/c` | CAN应用层逻辑 | ✅ 主要修改 | 经常 |
| `main.c` | 系统初始化和任务创建 | ⚠️ 谨慎修改 | 偶尔 |
| `stm32f4xx_it.c` | 中断服务程序 | ⚠️ 谨慎修改 | 很少 |

## 🔍 核心函数详解

### 1. 驱动层核心函数 (mcp2515.c)

#### 🚀 必须理解的函数

**MCP2515_Init() - 初始化函数**
```c
MCP2515_Result MCP2515_Init(uint32_t baudrate);
```
- **功能**: 初始化MCP2515控制器
- **参数**: `baudrate` - CAN波特率 (如500000表示500kbps)
- **返回值**: `MCP2515_OK`表示成功
- **何时调用**: 系统启动时调用一次
- **是否需要修改**: ❌ 不需要

**MCP2515_SendMessage() - 发送消息**
```c
MCP2515_Result MCP2515_SendMessage(MCP2515_CANMessage* message);
```
- **功能**: 发送CAN消息
- **参数**: `message` - 指向CAN消息结构体的指针
- **返回值**: `MCP2515_OK`表示发送成功
- **何时调用**: 需要发送CAN消息时
- **是否需要修改**: ❌ 不需要

**MCP2515_ReceiveMessage() - 接收消息**
```c
MCP2515_Result MCP2515_ReceiveMessage(MCP2515_CANMessage* message);
```
- **功能**: 接收CAN消息
- **参数**: `message` - 用于存储接收到的消息
- **返回值**: `MCP2515_OK`表示接收成功
- **何时调用**: 检测到有消息时
- **是否需要修改**: ❌ 不需要

#### 🔧 辅助函数 (了解即可)

- `MCP2515_Reset()` - 复位控制器
- `MCP2515_SetMode()` - 设置工作模式
- `MCP2515_ReadRegister()` - 读取寄存器
- `MCP2515_WriteRegister()` - 写入寄存器

### 2. 应用层核心函数 (can_app.c)

#### 🚀 必须理解的函数

**CAN_App_Init() - 应用初始化**
```c
CAN_App_Result CAN_App_Init(void);
```
- **功能**: 初始化CAN应用层
- **参数**: 无
- **返回值**: `CAN_APP_OK`表示成功
- **何时调用**: 系统启动时调用一次
- **是否需要修改**: ⚠️ 可能需要修改配置参数

**CAN_SendTask_Main() - 发送任务主函数**
```c
void CAN_SendTask_Main(void *argument);
```
- **功能**: CAN发送任务的主循环
- **参数**: `argument` - FreeRTOS任务参数
- **返回值**: 无 (无限循环)
- **何时调用**: FreeRTOS自动调用
- **是否需要修改**: ✅ 经常修改 (添加发送逻辑)

**CAN_ReceiveTask_Main() - 接收任务主函数**
```c
void CAN_ReceiveTask_Main(void *argument);
```
- **功能**: CAN接收任务的主循环
- **参数**: `argument` - FreeRTOS任务参数
- **返回值**: 无 (无限循环)
- **何时调用**: FreeRTOS自动调用
- **是否需要修改**: ✅ 经常修改 (添加接收处理逻辑)

#### 🔧 消息构造函数 (重要)

**CAN_App_BuildHeartbeatMessage() - 构造心跳消息**
```c
void CAN_App_BuildHeartbeatMessage(MCP2515_CANMessage* msg, uint8_t node_id);
```
- **功能**: 构造心跳消息
- **参数**: `msg` - 消息结构体, `node_id` - 节点ID
- **何时使用**: 需要发送心跳时
- **是否需要修改**: ⚠️ 可能需要修改消息格式

**CAN_App_BuildDataMessage() - 构造数据消息**
```c
void CAN_App_BuildDataMessage(MCP2515_CANMessage* msg, uint16_t data_id, 
                              uint8_t* data, uint8_t length);
```
- **功能**: 构造数据消息
- **参数**: `msg` - 消息结构体, `data_id` - 数据ID, `data` - 数据, `length` - 长度
- **何时使用**: 需要发送数据时
- **是否需要修改**: ✅ 经常修改 (根据数据格式)

## 📋 数据结构说明

### CAN消息结构体
```c
typedef struct {
    uint32_t id;           // CAN消息ID (11位或29位)
    uint8_t  data[8];      // 数据内容 (最多8字节)
    uint8_t  length;       // 数据长度 (0-8)
    uint8_t  extended;     // 扩展帧标志 (0=标准帧, 1=扩展帧)
    uint8_t  remote;       // 远程帧标志 (0=数据帧, 1=远程帧)
} MCP2515_CANMessage;
```

### 应用层消息类型
```c
// 消息类型定义
#define CAN_MSG_TYPE_HEARTBEAT    0x01  // 心跳消息
#define CAN_MSG_TYPE_DATA         0x02  // 数据消息
#define CAN_MSG_TYPE_STATUS       0x03  // 状态消息
#define CAN_MSG_TYPE_COMMAND      0x04  // 命令消息
#define CAN_MSG_TYPE_RESPONSE     0x05  // 响应消息
```

## 🎯 常用操作指南

### 1. 如何发送自定义消息

**步骤1**: 在发送任务中添加代码
```c
// 在 CAN_SendTask_Main() 函数中添加
void CAN_SendTask_Main(void *argument) {
    MCP2515_CANMessage msg;
    
    while(1) {
        // 构造自定义消息
        msg.id = 0x123;              // 设置消息ID
        msg.extended = 0;            // 标准帧
        msg.remote = 0;              // 数据帧
        msg.length = 4;              // 数据长度
        msg.data[0] = 0x11;          // 数据内容
        msg.data[1] = 0x22;
        msg.data[2] = 0x33;
        msg.data[3] = 0x44;
        
        // 发送消息
        if (MCP2515_SendMessage(&msg) == MCP2515_OK) {
            printf("消息发送成功\r\n");
        }
        
        osDelay(1000);  // 延时1秒
    }
}
```

### 2. 如何处理接收到的消息

**步骤1**: 在接收任务中添加处理逻辑
```c
// 在 CAN_ReceiveTask_Main() 函数中添加
void CAN_ReceiveTask_Main(void *argument) {
    MCP2515_CANMessage msg;
    
    while(1) {
        // 等待接收消息
        if (MCP2515_ReceiveMessage(&msg) == MCP2515_OK) {
            // 根据消息ID进行处理
            switch(msg.id) {
                case 0x100:  // 心跳消息
                    printf("收到心跳消息\r\n");
                    break;
                    
                case 0x200:  // 数据消息
                    printf("收到数据: %02X %02X %02X %02X\r\n", 
                           msg.data[0], msg.data[1], msg.data[2], msg.data[3]);
                    break;
                    
                default:
                    printf("收到未知消息: ID=0x%03X\r\n", msg.id);
                    break;
            }
        }
        
        osDelay(10);  // 短暂延时
    }
}
```

### 3. 如何添加新的消息类型

**步骤1**: 在 `can_app.h` 中定义新的消息ID
```c
// 在消息ID定义区域添加
#define CAN_MSG_ID_SENSOR_DATA    0x300  // 传感器数据
#define CAN_MSG_ID_CONTROL_CMD    0x400  // 控制命令
```

**步骤2**: 在 `can_app.c` 中添加构造函数
```c
// 添加传感器数据消息构造函数
void CAN_App_BuildSensorMessage(MCP2515_CANMessage* msg, 
                                 uint16_t sensor_value) {
    msg->id = CAN_MSG_ID_SENSOR_DATA;
    msg->extended = 0;
    msg->remote = 0;
    msg->length = 2;
    msg->data[0] = (sensor_value >> 8) & 0xFF;  // 高字节
    msg->data[1] = sensor_value & 0xFF;         // 低字节
}
```

**步骤3**: 在任务中使用新消息
```c
// 在发送任务中使用
MCP2515_CANMessage sensor_msg;
CAN_App_BuildSensorMessage(&sensor_msg, 1234);
MCP2515_SendMessage(&sensor_msg);
```

## ⚠️ 修改注意事项

### ✅ 可以安全修改的地方

1. **can_app.c 中的任务函数**
   - `CAN_SendTask_Main()` - 添加发送逻辑
   - `CAN_ReceiveTask_Main()` - 添加接收处理
   - 消息构造函数 - 修改消息格式

2. **can_app.h 中的定义**
   - 消息ID定义
   - 消息类型定义
   - 应用层常量

3. **main.c 中的用户代码区域**
   - `USER CODE BEGIN` 和 `USER CODE END` 之间的代码

### ❌ 不建议修改的地方

1. **mcp2515.c/h 全部内容**
   - 底层驱动代码，修改可能导致通信失败

2. **HAL库生成的代码**
   - STM32CubeMX自动生成的代码

3. **中断服务程序的核心部分**
   - 可能影响系统稳定性

### ⚠️ 谨慎修改的地方

1. **FreeRTOS任务配置**
   - 任务优先级和栈大小
   - 修改前要理解影响

2. **时钟和外设配置**
   - SPI配置参数
   - 中断配置

## 🚀 功能扩展指南

### 1. 添加新的CAN节点

**场景**: 系统中需要支持多个CAN节点通信

**实现步骤**:
1. 定义新的节点ID
```c
#define CAN_NODE_ID_SENSOR    0x01
#define CAN_NODE_ID_ACTUATOR  0x02
#define CAN_NODE_ID_GATEWAY   0x03
```

2. 修改消息ID分配策略
```c
// 消息ID = (节点ID << 8) | 消息类型
#define CAN_BUILD_MSG_ID(node_id, msg_type) \
    (((node_id) << 8) | (msg_type))
```

3. 在接收处理中添加节点识别
```c
uint8_t node_id = (msg.id >> 8) & 0xFF;
uint8_t msg_type = msg.id & 0xFF;
```

### 2. 添加消息过滤功能

**场景**: 只接收特定ID的消息，提高效率

**实现步骤**:
1. 配置硬件过滤器
```c
// 在 CAN_App_Init() 中添加
MCP2515_SetFilter(0, 0x100, 0x700);  // 只接收0x100-0x1FF的消息
```

2. 添加软件过滤
```c
// 在接收任务中添加
if (msg.id < 0x100 || msg.id > 0x1FF) {
    continue;  // 跳过不需要的消息
}
```

### 3. 添加错误处理和重传机制

**场景**: 提高通信可靠性

**实现步骤**:
1. 添加重传计数
```c
typedef struct {
    MCP2515_CANMessage msg;
    uint8_t retry_count;
    uint32_t timestamp;
} CAN_MessageWithRetry;
```

2. 实现重传逻辑
```c
void CAN_SendWithRetry(MCP2515_CANMessage* msg, uint8_t max_retry) {
    for (uint8_t i = 0; i < max_retry; i++) {
        if (MCP2515_SendMessage(msg) == MCP2515_OK) {
            return;  // 发送成功
        }
        osDelay(10);  // 重传间隔
    }
    printf("消息发送失败，已重试%d次\r\n", max_retry);
}
```

### 4. 添加数据记录功能

**场景**: 记录CAN消息用于调试和分析

**实现步骤**:
1. 定义记录结构
```c
typedef struct {
    uint32_t timestamp;
    MCP2515_CANMessage msg;
    uint8_t direction;  // 0=发送, 1=接收
} CAN_LogEntry;
```

2. 实现记录函数
```c
void CAN_LogMessage(MCP2515_CANMessage* msg, uint8_t direction) {
    // 记录到内存或存储设备
    printf("[%lu] %s ID:0x%03X Data:", 
           HAL_GetTick(), 
           direction ? "RX" : "TX", 
           msg->id);
    
    for (int i = 0; i < msg->length; i++) {
        printf(" %02X", msg->data[i]);
    }
    printf("\r\n");
}
```

## 🔧 调试技巧

### 1. 使用串口调试

**添加调试输出**:
```c
// 在关键位置添加调试信息
printf("[DEBUG] 发送消息: ID=0x%03X, 长度=%d\r\n", msg.id, msg.length);
printf("[DEBUG] MCP2515状态: 0x%02X\r\n", MCP2515_ReadStatus());
```

### 2. 检查硬件连接

**SPI通信测试**:
```c
// 读取MCP2515的版本寄存器
uint8_t version = MCP2515_ReadRegister(MCP2515_REG_CANSTAT);
if (version == 0x80) {
    printf("MCP2515连接正常\r\n");
} else {
    printf("MCP2515连接异常: 0x%02X\r\n", version);
}
```

### 3. 监控系统状态

**定期检查状态**:
```c
// 在默认任务中添加
void StartDefaultTask(void *argument) {
    while(1) {
        // 检查MCP2515状态
        uint8_t status = MCP2515_ReadStatus();
        if (status & 0x01) {
            printf("警告: CAN总线错误\r\n");
        }
        
        // 检查任务运行状态
        printf("系统运行正常，时间戳: %lu\r\n", HAL_GetTick());
        
        osDelay(5000);  // 5秒检查一次
    }
}
```

## 📚 学习建议

### 初学者学习路径

1. **第一阶段 (1-2天)**
   - 理解代码架构和文件结构
   - 学会编译和下载程序
   - 观察串口输出，理解程序运行流程

2. **第二阶段 (3-5天)**
   - 修改心跳消息的发送间隔
   - 添加简单的数据发送功能
   - 学会处理接收到的消息

3. **第三阶段 (1-2周)**
   - 添加新的消息类型
   - 实现简单的命令响应机制
   - 学会使用调试工具

4. **第四阶段 (2-4周)**
   - 实现复杂的应用协议
   - 添加错误处理和重传机制
   - 优化性能和可靠性

### 推荐学习资源

1. **STM32官方文档**
   - STM32F407参考手册
   - HAL库用户手册
   - FreeRTOS官方文档

2. **CAN总线相关**
   - CAN总线协议标准
   - MCP2515数据手册
   - CAN总线应用指南

3. **开发工具**
   - STM32CubeIDE使用指南
   - 逻辑分析仪使用方法
   - CAN总线分析工具

## 🆘 常见问题解答

### Q1: 编译时出现"找不到头文件"错误
**A**: 检查Include路径设置，确保 `Core/Inc` 目录在包含路径中。

### Q2: 程序运行但没有CAN消息发送
**A**: 
1. 检查硬件连接是否正确
2. 确认MCP2515初始化是否成功
3. 检查SPI通信是否正常

### Q3: 接收不到CAN消息
**A**:
1. 检查中断配置是否正确
2. 确认消息过滤器设置
3. 检查CAN总线是否有其他设备

### Q4: 系统运行一段时间后死机
**A**:
1. 检查任务栈大小是否足够
2. 确认没有死循环或阻塞操作
3. 检查内存使用情况

### Q5: 如何提高CAN通信速度
**A**:
1. 增加CAN波特率 (注意硬件限制)
2. 优化消息处理逻辑
3. 减少不必要的延时

---

**文档版本**: v1.0.0  
**最后更新**: 2025年1月27日  
**适用工程**: STM32F407 + MCP2515 CAN通信系统  
**作者**: STM32专家团队  

> 💡 **提示**: 本说明书会随着代码更新而更新，建议定期查看最新版本。如有疑问，请参考工程中的其他文档或联系技术支持。