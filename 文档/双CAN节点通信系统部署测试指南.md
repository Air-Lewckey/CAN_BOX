# 双CAN节点通信系统部署测试指南

## 🎯 **系统概述**

本系统实现STM32F407ZGT6内置CAN1控制器与WCMCU-230模块之间的双CAN节点通信，形成一个完整的CAN网络测试环境。

### **系统架构**
```
┌─────────────────┐    CAN总线    ┌─────────────────┐
│  STM32F407ZGT6  │◄──────────────►│   WCMCU-230     │
│                 │               │                 │
│ • 内置CAN1     │               │ • SN65HVD230   │
│ • MCP2515      │               │ • CAN收发器    │
│ • 双CAN控制器  │               │ • 独立CAN节点  │
└─────────────────┘               └─────────────────┘
        │                                   │
        ▼                                   ▼
   CAN1: 875Kbps                    CAN: 875Kbps
   引脚: PA11/PA12                  终端电阻: 120Ω
```

---

## 📋 **部署前检查清单**

### **硬件准备**
- [ ] STM32F407ZGT6最小系统板
- [ ] WCMCU-230 CAN模块
- [ ] CAN总线连接线（CANH、CANL、GND）
- [ ] 120Ω终端电阻（2个）
- [ ] 5V电源（为WCMCU-230供电）
- [ ] USB转串口模块（调试用）
- [ ] ST-Link调试器

### **软件准备**
- [ ] STM32CubeIDE（1.13.x或更高版本）
- [ ] 串口调试助手（SSCOM、PuTTY等）
- [ ] CAN分析工具（可选：CANoe、PCAN-View）

### **文档准备**
- [ ] STM32F407_CAN1外设配置指南.md
- [ ] STM32F407_WCMCU230_双CAN节点通信方案.md
- [ ] CANOE_VN1640_CAN通信验证指南.md

---

## ⚙️ **第一步：STM32CubeIDE配置**

### **1.1 打开配置文件**
```bash
1. 启动STM32CubeIDE
2. 打开CAN_BOX工程
3. 双击CAN_BOX.ioc文件
4. 等待STM32CubeMX界面加载
```

### **1.2 配置CAN1外设**
```
步骤：
1. Connectivity → CAN1 → Mode: Activated
2. 参数配置：
   - Prescaler: 6
   - Time Quanta in Bit Segment 1: 13 Times
   - Time Quanta in Bit Segment 2: 2 Times
   - ReSynchronization Jump Width: 1 Times
   - 波特率: 875Kbps
```

### **1.3 配置CAN1引脚**
```
方案A（推荐）：
- PA11 → CAN1_RX
- PA12 → CAN1_TX

方案B（备选）：
- PB8 → CAN1_RX  
- PB9 → CAN1_TX
```

### **1.4 配置CAN1中断**
```
NVIC Settings：
- CAN1 RX0 interrupts: Enabled (优先级1-0)
- CAN1 TX interrupts: Enabled (优先级2-0)
- CAN1 SCE interrupts: Enabled (优先级1-1)
```

### **1.5 生成代码**
```bash
1. 保存配置：Ctrl+S
2. 生成代码：点击Generate Code按钮
3. 等待代码生成完成
4. 检查生成的can.c和can.h文件
```

---

## 🔧 **第二步：硬件连接**

### **2.1 STM32F407连接**
```
STM32F407ZGT6引脚连接：
┌─────────────┬─────────────┬─────────────┐
│    功能     │    引脚     │    连接     │
├─────────────┼─────────────┼─────────────┤
│ CAN1_RX     │ PA11        │ CAN收发器RX │
│ CAN1_TX     │ PA12        │ CAN收发器TX │
│ 电源        │ 5V/3.3V     │ 模块电源    │
│ 地线        │ GND         │ 公共地      │
│ 串口调试    │ PA2/PA3     │ USB转串口   │
└─────────────┴─────────────┴─────────────┘
```

### **2.2 WCMCU-230连接**
```
WCMCU-230模块连接：
┌─────────────┬─────────────┬─────────────┐
│    功能     │    引脚     │    连接     │
├─────────────┼─────────────┼─────────────┤
│ CANH        │ CANH        │ CAN总线H    │
│ CANL        │ CANL        │ CAN总线L    │
│ 电源        │ VCC         │ 5V电源      │
│ 地线        │ GND         │ 公共地      │
│ 终端电阻    │ 120Ω       │ CANH-CANL   │
└─────────────┴─────────────┴─────────────┘
```

### **2.3 CAN总线连接**
```
CAN总线拓扑：

STM32F407 ──┬── CANH ──┬── WCMCU-230
            │          │
            └── CANL ──┘
            │          │
          120Ω        120Ω
        (终端电阻)  (终端电阻)
```

---

## 💻 **第三步：程序编译下载**

### **3.1 编译项目**
```bash
1. 在STM32CubeIDE中：
   - 右键项目 → Build Project
   - 或按Ctrl+B
   - 检查编译输出无错误

2. 检查编译结果：
   - 0 errors, 0 warnings
   - 生成.elf和.bin文件
```

### **3.2 下载程序**
```bash
1. 连接ST-Link到STM32F407
2. 点击Debug按钮（F11）
3. 选择调试配置
4. 等待程序下载完成
5. 点击Resume继续运行
```

### **3.3 验证下载**
```bash
串口输出应显示：
=== STM32F407 + MCP2515 CAN Communication System Startup ===
System Clock: 168 MHz
SPI1 Clock: 2 MHz
CAN application initialization successful!
=== 双CAN节点通信系统 ===
STM32F407 内置CAN1 + WCMCU-230模块
```

---

## 🧪 **第四步：功能测试**

### **4.1 基础连接测试**

**测试目标**：验证CAN1外设配置正确

```bash
预期串口输出：
=== 启动双CAN节点通信任务 ===
检测到CAN1外设配置，启动双节点通信...
开始初始化CAN双节点通信...
CAN双节点通信初始化完成
支持的消息类型：
  - 心跳消息 (ID: 0x100)
  - 数据请求 (ID: 0x200)
  - 数据响应 (ID: 0x201)
  - 状态消息 (ID: 0x300)
  - 控制指令 (ID: 0x400)
```

**如果显示警告**：
```bash
警告：未检测到CAN1外设配置！
请按照以下步骤配置CAN1外设：
1. 打开CAN_BOX.ioc文件
2. 在Connectivity中启用CAN1
...
```

**解决方案**：按照STM32F407_CAN1外设配置指南.md重新配置

### **4.2 心跳消息测试**

**测试目标**：验证周期性心跳消息发送

```bash
预期串口输出（每2秒）：
[发送] ID:0x100 长度:4 数据:AB CD 00 01
[发送] ID:0x100 长度:4 数据:AB CD 00 02
[发送] ID:0x100 长度:4 数据:AB CD 00 03
```

**验证要点**：
- 消息ID为0x100
- 数据长度为4字节
- 魔数为0xABCD
- 计数器递增

### **4.3 数据请求测试**

**测试目标**：验证数据请求消息发送

```bash
预期串口输出（每5秒）：
[发送] ID:0x200 长度:2 数据:01 00
[发送] ID:0x200 长度:2 数据:02 00
[发送] ID:0x200 长度:2 数据:01 00
```

**验证要点**：
- 消息ID为0x200
- 请求类型交替（01/02）
- 周期性发送

### **4.4 状态消息测试**

**测试目标**：验证状态消息发送

```bash
预期串口输出（每10秒）：
[发送] ID:0x300 长度:6 数据:12 34 00 00 01 XX
```

**验证要点**：
- 消息ID为0x300
- 魔数为0x1234
- 状态和计数器正确

### **4.5 WCMCU-230响应测试**

**测试目标**：验证WCMCU-230模块响应

```bash
预期串口输出：
[接收] ID:0x201 长度:8 数据:01 00 XX XX XX XX XX XX
收到WCMCU数据响应: 01 00 XX XX XX XX XX XX
WCMCU状态: 0, TX:XX, RX:XX, ERR:XX
```

**如果没有接收消息**：
1. 检查WCMCU-230电源
2. 检查CAN总线连接
3. 检查终端电阻
4. 检查波特率配置

---

## 📊 **第五步：性能监控**

### **5.1 通信统计**

**每30秒自动打印**：
```bash
=== CAN双节点通信统计 ===
运行时间: 30000 ms (30.0 秒)
发送消息: 25
接收消息: 15
错误次数: 0
心跳消息: 15
数据请求: 6
数据响应: 15
通信成功率: 60.00%
最后接收时间: 29500 ms
========================
```

### **5.2 节点状态监控**

```bash
=== 节点状态信息 ===
WCMCU-230状态: 在线
最后心跳时间: 29800 ms
最后发送时间: 29900 ms
当前时间: 30000 ms
==================
```

### **5.3 性能指标**

**正常运行指标**：
- 通信成功率：> 90%
- 心跳响应时间：< 100ms
- 错误次数：< 1%
- 节点状态：在线

**异常情况处理**：
- 成功率 < 50%：检查硬件连接
- 无接收消息：检查WCMCU-230配置
- 高错误率：检查总线负载和干扰

---

## 🔧 **第六步：故障排除**

### **6.1 常见问题及解决方案**

#### **问题1：编译错误**
```
错误信息：'hcan1' undeclared
解决方案：
1. 检查CAN1外设是否已启用
2. 重新生成代码
3. 检查can.h文件是否包含
```

#### **问题2：无CAN消息发送**
```
现象：串口无[发送]消息
解决方案：
1. 检查CAN1初始化是否成功
2. 检查CAN1引脚配置
3. 检查时钟配置
4. 检查中断配置
```

#### **问题3：无WCMCU-230响应**
```
现象：只有发送，无接收消息
解决方案：
1. 检查WCMCU-230电源（5V）
2. 检查CAN总线连接（CANH/CANL）
3. 检查终端电阻（120Ω）
4. 检查波特率匹配（875Kbps）
5. 检查WCMCU-230配置
```

#### **问题4：通信不稳定**
```
现象：间歇性通信失败
解决方案：
1. 检查电源稳定性
2. 检查总线干扰
3. 调整发送周期
4. 检查终端电阻质量
5. 优化中断优先级
```

### **6.2 调试工具使用**

#### **串口调试**
```bash
1. 波特率：115200
2. 数据位：8
3. 停止位：1
4. 校验位：无
5. 流控：无
```

#### **CAN分析仪**
```bash
如果有CAN分析工具：
1. 连接到CAN总线
2. 设置波特率875Kbps
3. 监控消息流量
4. 分析错误帧
5. 验证消息格式
```

---

## ✅ **第七步：验收测试**

### **7.1 功能验收清单**

- [ ] **基础功能**
  - [ ] CAN1外设正常初始化
  - [ ] 心跳消息周期性发送（2秒）
  - [ ] 数据请求周期性发送（5秒）
  - [ ] 状态消息周期性发送（10秒）

- [ ] **通信功能**
  - [ ] WCMCU-230正常响应
  - [ ] 双向数据交换正常
  - [ ] 消息格式正确
  - [ ] 错误处理正常

- [ ] **性能指标**
  - [ ] 通信成功率 > 90%
  - [ ] 响应时间 < 100ms
  - [ ] 系统稳定运行 > 1小时
  - [ ] 无内存泄漏

### **7.2 压力测试**

```bash
测试方案：
1. 连续运行24小时
2. 监控通信统计
3. 记录错误次数
4. 验证系统稳定性

通过标准：
- 系统无崩溃
- 通信成功率 > 95%
- 内存使用稳定
- 响应时间稳定
```

---

## 📈 **第八步：性能优化**

### **8.1 通信优化**

```c
// 优化发送周期
#define CAN_HEARTBEAT_PERIOD    1000   // 1秒心跳
#define CAN_DATA_REQUEST_PERIOD 3000   // 3秒数据请求
#define CAN_STATUS_PERIOD       5000   // 5秒状态

// 优化缓冲区大小
#define CAN_TX_BUFFER_SIZE      16
#define CAN_RX_BUFFER_SIZE      32
```

### **8.2 错误处理优化**

```c
// 增加重试机制
#define CAN_MAX_RETRY_COUNT     3
#define CAN_RETRY_DELAY         100

// 增加错误恢复
void CAN_ErrorRecovery(void)
{
    // 重置CAN控制器
    // 重新初始化
    // 清除错误标志
}
```

### **8.3 监控优化**

```c
// 增加详细统计
typedef struct {
    uint32_t tx_success;
    uint32_t tx_failed;
    uint32_t rx_success;
    uint32_t rx_timeout;
    uint32_t error_passive;
    uint32_t error_busoff;
    float    avg_response_time;
} CAN_DetailedStats_t;
```

---

## 📚 **附录**

### **A. 消息ID定义**

```c
#define CAN_HEARTBEAT_ID        0x100  // 心跳消息
#define CAN_DATA_REQUEST_ID     0x200  // 数据请求
#define CAN_DATA_RESPONSE_ID    0x201  // 数据响应
#define CAN_STATUS_ID           0x300  // 状态消息
#define CAN_CONTROL_ID          0x400  // 控制指令
#define CAN_ERROR_ID            0x500  // 错误消息
#define CAN_WCMCU_TO_STM32_ID   0x600  // WCMCU发送给STM32
```

### **B. 错误代码定义**

```c
#define CAN_ERROR_NONE          0x00   // 无错误
#define CAN_ERROR_INIT          0x01   // 初始化错误
#define CAN_ERROR_CONFIG        0x02   // 配置错误
#define CAN_ERROR_SEND          0x03   // 发送错误
#define CAN_ERROR_RECEIVE       0x04   // 接收错误
#define CAN_ERROR_TIMEOUT       0x05   // 超时错误
#define CAN_ERROR_BUSOFF        0x06   // 总线关闭
#define CAN_ERROR_PASSIVE       0x07   // 错误被动
```

### **C. 技术支持**

**文档参考**：
- STM32F407参考手册
- STM32 HAL库用户手册
- SN65HVD230数据手册
- CAN总线规范ISO11898

**在线资源**：
- STM32官方社区
- 正点原子技术论坛
- CAN总线技术交流群

---

*本指南基于STM32F407ZGT6和WCMCU-230模块编写，适用于STM32CubeIDE 1.13.x版本*

**版本信息**：
- 文档版本：V1.0
- 创建日期：2024-12-19
- 最后更新：2024-12-19
- 作者：正点原子技术专家