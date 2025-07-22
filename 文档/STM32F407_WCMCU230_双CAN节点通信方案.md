# STM32F407 + WCMCU-230 双CAN节点通信方案

## 🎯 **项目目标**

在现有STM32F407+MCP2515 CAN通信系统基础上，增加WCMCU-230模块作为第二个CAN节点，实现双CAN节点之间的通信验证。

### **系统架构**
```
┌─────────────────────┐    CAN总线    ┌─────────────────────┐
│   STM32F407ZGT6     │◄──────────────►│    WCMCU-230        │
│   (主控节点)        │               │   (从控节点)        │
│                     │               │                     │
│ ┌─────────────────┐ │               │ ┌─────────────────┐ │
│ │   内置CAN1      │ │               │ │   SN65HVD230    │ │
│ │   控制器        │ │               │ │   CAN收发器     │ │
│ └─────────────────┘ │               │ └─────────────────┘ │
│          │          │               │          │          │
│ ┌─────────────────┐ │               │ ┌─────────────────┐ │
│ │   TJA1050       │ │               │ │   微控制器      │ │
│ │   CAN收发器     │ │               │ │   (STM32/其他)  │ │
│ └─────────────────┘ │               │ └─────────────────┘ │
└─────────────────────┘               └─────────────────────┘
```

## 🔧 **硬件配置方案**

### **1. STM32F407ZGT6 CAN1配置**

#### **方案A: 使用PA11/PA12引脚（推荐）**
```
STM32F407ZGT6 内置CAN1    <-->    TJA1050收发器
─────────────────────────────────────────────────
PA11 (CAN1_RX)           <-->    RXD (Pin 4)
PA12 (CAN1_TX)           <-->    TXD (Pin 1)
3.3V                     <-->    VCC (Pin 3)
GND                      <-->    GND (Pin 2)
                                  S   (Pin 8) --> GND (高速模式)
```

#### **方案B: 使用PB8/PB9引脚（备选）**
```
STM32F407ZGT6 内置CAN1    <-->    TJA1050收发器
─────────────────────────────────────────────────
PB8  (CAN1_RX)           <-->    RXD (Pin 4)
PB9  (CAN1_TX)           <-->    TXD (Pin 1)
3.3V                     <-->    VCC (Pin 3)
GND                      <-->    GND (Pin 2)
                                  S   (Pin 8) --> GND (高速模式)
```

### **2. WCMCU-230模块配置**

#### **WCMCU-230基于SN65HVD230的引脚定义**
```
WCMCU-230模块引脚:
┌─────────────────────────────────────┐
│  Pin 1: TXD  (连接MCU的CAN_TX)      │
│  Pin 2: GND  (地线)                 │
│  Pin 3: VCC  (3.3V电源)            │
│  Pin 4: RXD  (连接MCU的CAN_RX)      │
│  Pin 5: Vref (参考电压输出,可悬空)  │
│  Pin 6: CANL (CAN总线低电平)        │
│  Pin 7: CANH (CAN总线高电平)        │
│  Pin 8: RS   (模式选择,接GND高速)   │
└─────────────────────────────────────┘
```

### **3. CAN总线连接**

```
STM32F407端(TJA1050)     CAN总线      WCMCU-230端(SN65HVD230)
─────────────────────────────────────────────────────────────
CANH (Pin 7)            ◄────────────► CANH (Pin 7)
CANL (Pin 6)            ◄────────────► CANL (Pin 6)

终端电阻配置:
┌─────────────────────────────────────────────────────────────┐
│  STM32端: CANH ──[120Ω]── CANL                             │
│  WCMCU-230端: CANH ──[120Ω]── CANL                         │
└─────────────────────────────────────────────────────────────┘
```

## ⚙️ **软件配置**

### **1. STM32CubeIDE配置步骤**

#### **步骤1: 启用CAN1外设**
```
1. 打开CAN_BOX.ioc文件
2. 在左侧面板找到 Connectivity → CAN1
3. 设置Mode为 "Activated"
4. 配置参数:
   - Prescaler: 6
   - Time Quanta in Bit Segment 1: 13 Times
   - Time Quanta in Bit Segment 2: 2 Times
   - ReSynchronization Jump Width: 1 Times
   - 计算波特率: 84MHz/(6*(13+2+1)) = 875Kbps
```

#### **步骤2: 配置CAN引脚**

**方案A (PA11/PA12):**
```
1. 在Pinout视图中:
   - PA11设置为CAN1_RX
   - PA12设置为CAN1_TX
2. GPIO设置:
   - PA11: 输入模式,上拉
   - PA12: 复用推挽输出,高速
```

**方案B (PB8/PB9):**
```
1. 在Pinout视图中:
   - PB8设置为CAN1_RX  
   - PB9设置为CAN1_TX
2. 需要启用引脚重映射:
   - 在System Core → SYS中启用
   - 在代码中添加: __HAL_AFIO_REMAP_CAN1_2();
```

#### **步骤3: 配置CAN中断**
```
1. 在NVIC Settings中启用:
   - CAN1 RX0 interrupts
   - CAN1 TX interrupts (可选)
2. 设置中断优先级:
   - Preemption Priority: 1
   - Sub Priority: 0
```

### **2. 代码实现**

#### **CAN初始化代码**
```c
/* can_dual_node.h */
#ifndef __CAN_DUAL_NODE_H
#define __CAN_DUAL_NODE_H

#include "main.h"

// CAN消息ID定义
#define CAN_STM32_TO_WCMCU_ID    0x123  // STM32发送给WCMCU-230
#define CAN_WCMCU_TO_STM32_ID    0x456  // WCMCU-230发送给STM32
#define CAN_HEARTBEAT_ID         0x100  // 心跳消息
#define CAN_DATA_REQUEST_ID      0x200  // 数据请求
#define CAN_DATA_RESPONSE_ID     0x300  // 数据响应

// 函数声明
void CAN_DualNode_Init(void);
void CAN_SendToWCMCU(uint32_t id, uint8_t* data, uint8_t len);
void CAN_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data);
void CAN_SendHeartbeat(void);
void CAN_RequestData(void);

#endif
```

```c
/* can_dual_node.c */
#include "can_dual_node.h"
#include "can.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

CAN_TxHeaderTypeDef TxHeader;
CAN_RxHeaderTypeDef RxHeader;
uint8_t TxData[8];
uint8_t RxData[8];
uint32_t TxMailbox;

/**
 * @brief CAN双节点通信初始化
 */
void CAN_DualNode_Init(void)
{
    CAN_FilterTypeDef sFilterConfig;
    
    // 配置CAN过滤器
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;
    
    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    
    // 启动CAN
    if (HAL_CAN_Start(&hcan1) != HAL_OK)
    {
        Error_Handler();
    }
    
    // 激活CAN接收中断
    if (HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK)
    {
        Error_Handler();
    }
    
    printf("CAN双节点通信初始化完成\r\n");
}

/**
 * @brief 向WCMCU-230发送CAN消息
 */
void CAN_SendToWCMCU(uint32_t id, uint8_t* data, uint8_t len)
{
    TxHeader.StdId = id;
    TxHeader.ExtId = 0x00;
    TxHeader.RTR = CAN_RTR_DATA;
    TxHeader.IDE = CAN_ID_STD;
    TxHeader.DLC = len;
    TxHeader.TransmitGlobalTime = DISABLE;
    
    memcpy(TxData, data, len);
    
    if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox) == HAL_OK)
    {
        printf("发送CAN消息: ID=0x%03X, 数据=", (unsigned int)id);
        for(int i = 0; i < len; i++)
        {
            printf("%02X ", data[i]);
        }
        printf("\r\n");
    }
    else
    {
        printf("CAN消息发送失败\r\n");
    }
}

/**
 * @brief 处理接收到的CAN消息
 */
void CAN_ProcessReceivedMessage(CAN_RxHeaderTypeDef* header, uint8_t* data)
{
    printf("接收CAN消息: ID=0x%03X, 长度=%d, 数据=", 
           (unsigned int)header->StdId, header->DLC);
    
    for(int i = 0; i < header->DLC; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\r\n");
    
    // 根据消息ID处理不同类型的消息
    switch(header->StdId)
    {
        case CAN_WCMCU_TO_STM32_ID:
            printf("收到WCMCU-230数据消息\r\n");
            // 处理数据消息
            break;
            
        case CAN_HEARTBEAT_ID:
            printf("收到心跳消息\r\n");
            break;
            
        case CAN_DATA_REQUEST_ID:
            printf("收到数据请求,发送响应\r\n");
            // 发送数据响应
            uint8_t response[] = {0xAA, 0x55, 0x12, 0x34};
            CAN_SendToWCMCU(CAN_DATA_RESPONSE_ID, response, 4);
            break;
            
        default:
            printf("未知消息类型\r\n");
            break;
    }
}

/**
 * @brief 发送心跳消息
 */
void CAN_SendHeartbeat(void)
{
    static uint32_t counter = 0;
    uint8_t heartbeat_data[4];
    
    heartbeat_data[0] = 0xAA;  // 心跳标识
    heartbeat_data[1] = 0x55;
    heartbeat_data[2] = (counter >> 8) & 0xFF;
    heartbeat_data[3] = counter & 0xFF;
    
    CAN_SendToWCMCU(CAN_HEARTBEAT_ID, heartbeat_data, 4);
    counter++;
}

/**
 * @brief 请求WCMCU-230数据
 */
void CAN_RequestData(void)
{
    uint8_t request_data[2] = {0x01, 0x02};  // 请求命令
    CAN_SendToWCMCU(CAN_DATA_REQUEST_ID, request_data, 2);
}

/**
 * @brief CAN接收中断回调函数
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    if (hcan->Instance == CAN1)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK)
        {
            CAN_ProcessReceivedMessage(&RxHeader, RxData);
        }
    }
}
```

#### **主任务集成**
```c
/* 在main.c或freertos.c中添加 */

// 在任务中添加周期性通信
void CAN_DualNodeTask(void const * argument)
{
    uint32_t last_heartbeat = 0;
    uint32_t last_data_request = 0;
    
    // 初始化CAN双节点通信
    CAN_DualNode_Init();
    
    for(;;)
    {
        uint32_t current_time = HAL_GetTick();
        
        // 每1秒发送心跳
        if(current_time - last_heartbeat >= 1000)
        {
            CAN_SendHeartbeat();
            last_heartbeat = current_time;
        }
        
        // 每3秒请求数据
        if(current_time - last_data_request >= 3000)
        {
            CAN_RequestData();
            last_data_request = current_time;
        }
        
        // 发送测试数据
        if(current_time % 5000 == 0)  // 每5秒
        {
            uint8_t test_data[] = {0x12, 0x34, 0x56, 0x78};
            CAN_SendToWCMCU(CAN_STM32_TO_WCMCU_ID, test_data, 4);
        }
        
        osDelay(100);  // 100ms周期
    }
}
```

## 🧪 **测试验证方案**

### **1. 基础连接测试**

#### **测试步骤:**
1. **硬件连接检查**
   - 确认CAN_H和CAN_L正确连接
   - 检查120Ω终端电阻安装
   - 验证电源和地线连接

2. **软件配置验证**
   - 编译并下载程序到STM32F407
   - 检查串口输出的初始化信息
   - 确认CAN控制器启动成功

#### **预期结果:**
```
CAN双节点通信初始化完成
发送CAN消息: ID=0x100, 数据=AA 55 00 00
发送CAN消息: ID=0x200, 数据=01 02
```

### **2. 回环测试**

#### **测试配置:**
```c
// 在CAN初始化中临时启用回环模式
hcan1.Init.Mode = CAN_MODE_LOOPBACK;
```

#### **预期结果:**
- STM32发送的消息能够被自己接收
- 串口输出显示发送和接收的消息

### **3. WCMCU-230通信测试**

#### **WCMCU-230端配置示例:**
```c
// WCMCU-230端的简单回显程序
void WCMCU_CAN_EchoTest(void)
{
    // 接收STM32的消息并回显
    if(CAN_MessageReceived())
    {
        uint32_t rx_id = CAN_GetReceivedID();
        uint8_t rx_data[8];
        uint8_t rx_len = CAN_GetReceivedData(rx_data);
        
        // 回显消息,ID+1
        CAN_SendMessage(rx_id + 1, rx_data, rx_len);
    }
}
```

### **4. 性能测试**

#### **测试指标:**
- **通信成功率**: >99%
- **消息延迟**: <10ms
- **总线负载**: <30%
- **错误率**: <0.1%

#### **测试方法:**
```c
// 性能统计
typedef struct {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t start_time;
} CAN_Stats_t;

void CAN_PrintStats(CAN_Stats_t* stats)
{
    uint32_t elapsed = HAL_GetTick() - stats->start_time;
    float success_rate = (float)stats->rx_count / stats->tx_count * 100;
    
    printf("=== CAN通信统计 ===\r\n");
    printf("运行时间: %lu ms\r\n", elapsed);
    printf("发送消息: %lu\r\n", stats->tx_count);
    printf("接收消息: %lu\r\n", stats->rx_count);
    printf("错误次数: %lu\r\n", stats->error_count);
    printf("成功率: %.2f%%\r\n", success_rate);
}
```

## 🔧 **故障排除**

### **常见问题及解决方案**

#### **1. 无法发送消息**
**症状:** 发送函数返回错误
**解决方案:**
- 检查CAN控制器是否正确初始化
- 确认CAN总线未处于Bus-Off状态
- 验证发送邮箱是否可用

#### **2. 无法接收消息**
**症状:** 接收中断不触发
**解决方案:**
- 检查CAN过滤器配置
- 确认中断使能正确
- 验证NVIC配置

#### **3. 总线错误**
**症状:** 错误计数器增加
**解决方案:**
- 检查终端电阻(120Ω)
- 确认CAN_H/CAN_L连接正确
- 验证波特率设置一致

#### **4. WCMCU-230无响应**
**症状:** 发送消息但无回应
**解决方案:**
- 检查WCMCU-230电源供电
- 确认SN65HVD230的RS引脚接地
- 验证WCMCU-230程序正确运行

## 📊 **扩展功能**

### **1. 多节点网络**
```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│  STM32F407  │    │  WCMCU-230  │    │   节点3     │
│   (主控)    │◄──►│   (从控1)   │◄──►│  (从控2)    │
└─────────────┘    └─────────────┘    └─────────────┘
       │                   │                   │
       └───────────────────┼───────────────────┘
                          CAN总线
```

### **2. CANopen协议支持**
- 实现CANopen协议栈
- 支持PDO/SDO通信
- 网络管理功能

### **3. 诊断功能**
- 总线状态监控
- 错误统计和报告
- 性能分析工具

## 📋 **项目清单**

### **硬件清单**
- [x] STM32F407ZGT6开发板
- [x] WCMCU-230模块
- [x] TJA1050 CAN收发器(如果STM32端没有)
- [x] 120Ω终端电阻 x2
- [x] 连接线缆
- [x] 示波器(调试用)

### **软件清单**
- [x] STM32CubeIDE
- [x] CAN双节点通信代码
- [x] 测试程序
- [x] 调试工具

### **文档清单**
- [x] 硬件连接图
- [x] 软件配置说明
- [x] 测试验证报告
- [x] 故障排除指南

## 🎯 **项目里程碑**

### **阶段1: 基础通信 (1-2天)**
- [x] 硬件连接完成
- [x] 软件配置完成
- [x] 基础CAN通信实现

### **阶段2: 功能验证 (2-3天)**
- [ ] 双向通信测试
- [ ] 性能测试
- [ ] 稳定性测试

### **阶段3: 优化完善 (1-2天)**
- [ ] 错误处理优化
- [ ] 性能调优
- [ ] 文档完善

---

## 💡 **技术要点总结**

1. **STM32F407内置CAN控制器**具有强大的功能和灵活的配置选项
2. **WCMCU-230基于SN65HVD230**，提供可靠的CAN物理层接口
3. **双节点通信**可以验证CAN网络的基本功能和性能
4. **终端电阻配置**对信号质量至关重要
5. **软件架构设计**应考虑扩展性和维护性

通过这个方案，您可以成功实现STM32F407与WCMCU-230之间的CAN通信，为后续的多节点网络和复杂应用奠定基础！