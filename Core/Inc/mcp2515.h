/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mcp2515.h
  * @brief          : MCP2515 CAN控制器驱动头文件
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * 本驱动适用于STM32F407ZGT6 + MCP2515 + TJA1050的CAN通信方案
  * 硬件连接：
  * PB3  -> MCP2515 SCK  (SPI1时钟)
  * PB4  -> MCP2515 SO   (SPI1数据输入MISO)
  * PB5  -> MCP2515 SI   (SPI1数据输出MOSI)
  * PB10 -> MCP2515 INT  (中断信号)
  * PB12 -> MCP2515 CS   (片选信号)
  * 3.3V -> MCP2515 VCC  (电源正极)
  * GND  -> MCP2515 GND  (电源负极)
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MCP2515_H
#define __MCP2515_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

/* MCP2515寄存器地址定义 --------------------------------------------------------*/

/* 控制寄存器 */
#define MCP2515_CANCTRL     0x0F    // CAN控制寄存器
#define MCP2515_CANSTAT     0x0E    // CAN状态寄存器
#define MCP2515_CNF1        0x2A    // 配置寄存器1 (波特率配置)
#define MCP2515_CNF2        0x29    // 配置寄存器2 (波特率配置)
#define MCP2515_CNF3        0x28    // 配置寄存器3 (波特率配置)

/* 中断寄存器 */
#define MCP2515_CANINTE     0x2B    // 中断使能寄存器
#define MCP2515_CANINTF     0x2C    // 中断标志寄存器

/* 错误寄存器 */
#define MCP2515_EFLG        0x2D    // 错误标志寄存器
#define MCP2515_TEC         0x1C    // 发送错误计数器
#define MCP2515_REC         0x1D    // 接收错误计数器

/* 发送缓冲区寄存器 (3个发送缓冲区) */
#define MCP2515_TXB0CTRL    0x30    // 发送缓冲区0控制寄存器
#define MCP2515_TXB0SIDH    0x31    // 发送缓冲区0标准ID高字节
#define MCP2515_TXB0SIDL    0x32    // 发送缓冲区0标准ID低字节
#define MCP2515_TXB0EID8    0x33    // 发送缓冲区0扩展ID高字节
#define MCP2515_TXB0EID0    0x34    // 发送缓冲区0扩展ID低字节
#define MCP2515_TXB0DLC     0x35    // 发送缓冲区0数据长度代码
#define MCP2515_TXB0D0      0x36    // 发送缓冲区0数据字节0

#define MCP2515_TXB1CTRL    0x40    // 发送缓冲区1控制寄存器
#define MCP2515_TXB1SIDH    0x41    // 发送缓冲区1标准ID高字节
#define MCP2515_TXB1SIDL    0x42    // 发送缓冲区1标准ID低字节
#define MCP2515_TXB1EID8    0x43    // 发送缓冲区1扩展ID高字节
#define MCP2515_TXB1EID0    0x44    // 发送缓冲区1扩展ID低字节
#define MCP2515_TXB1DLC     0x45    // 发送缓冲区1数据长度代码
#define MCP2515_TXB1D0      0x46    // 发送缓冲区1数据字节0

#define MCP2515_TXB2CTRL    0x50    // 发送缓冲区2控制寄存器
#define MCP2515_TXB2SIDH    0x51    // 发送缓冲区2标准ID高字节
#define MCP2515_TXB2SIDL    0x52    // 发送缓冲区2标准ID低字节
#define MCP2515_TXB2EID8    0x53    // 发送缓冲区2扩展ID高字节
#define MCP2515_TXB2EID0    0x54    // 发送缓冲区2扩展ID低字节
#define MCP2515_TXB2DLC     0x55    // 发送缓冲区2数据长度代码
#define MCP2515_TXB2D0      0x56    // 发送缓冲区2数据字节0

/* 接收缓冲区寄存器 (2个接收缓冲区) */
#define MCP2515_RXB0CTRL    0x60    // 接收缓冲区0控制寄存器
#define MCP2515_RXB0SIDH    0x61    // 接收缓冲区0标准ID高字节
#define MCP2515_RXB0SIDL    0x62    // 接收缓冲区0标准ID低字节
#define MCP2515_RXB0EID8    0x63    // 接收缓冲区0扩展ID高字节
#define MCP2515_RXB0EID0    0x64    // 接收缓冲区0扩展ID低字节
#define MCP2515_RXB0DLC     0x65    // 接收缓冲区0数据长度代码
#define MCP2515_RXB0D0      0x66    // 接收缓冲区0数据字节0

#define MCP2515_RXB1CTRL    0x70    // 接收缓冲区1控制寄存器
#define MCP2515_RXB1SIDH    0x71    // 接收缓冲区1标准ID高字节
#define MCP2515_RXB1SIDL    0x72    // 接收缓冲区1标准ID低字节
#define MCP2515_RXB1EID8    0x73    // 接收缓冲区1扩展ID高字节
#define MCP2515_RXB1EID0    0x74    // 接收缓冲区1扩展ID低字节
#define MCP2515_RXB1DLC     0x75    // 接收缓冲区1数据长度代码
#define MCP2515_RXB1D0      0x76    // 接收缓冲区1数据字节0

/* MCP2515 SPI指令定义 --------------------------------------------------------*/
#define MCP2515_CMD_RESET       0xC0    // 复位指令
#define MCP2515_CMD_READ        0x03    // 读寄存器指令
#define MCP2515_CMD_WRITE       0x02    // 写寄存器指令
#define MCP2515_CMD_RTS         0x80    // 请求发送指令
#define MCP2515_CMD_READ_STATUS 0xA0    // 读状态指令
#define MCP2515_CMD_BIT_MODIFY  0x05    // 位修改指令
#define MCP2515_CMD_LOAD_TX0    0x40    // 加载发送缓冲区0指令
#define MCP2515_CMD_LOAD_TX1    0x42    // 加载发送缓冲区1指令
#define MCP2515_CMD_LOAD_TX2    0x44    // 加载发送缓冲区2指令
#define MCP2515_CMD_READ_RX0    0x90    // 读接收缓冲区0指令
#define MCP2515_CMD_READ_RX1    0x94    // 读接收缓冲区1指令

/* MCP2515工作模式定义 --------------------------------------------------------*/
#define MCP2515_MODE_NORMAL     0x00    // 正常模式
#define MCP2515_MODE_SLEEP      0x20    // 睡眠模式
#define MCP2515_MODE_LOOPBACK   0x40    // 回环模式
#define MCP2515_MODE_LISTENONLY 0x60    // 只听模式
#define MCP2515_MODE_CONFIG     0x80    // 配置模式

/* CAN波特率配置定义 ----------------------------------------------------------*/
#define MCP2515_BAUD_125K       0       // 125Kbps
#define MCP2515_BAUD_250K       1       // 250Kbps
#define MCP2515_BAUD_500K       2       // 500Kbps
#define MCP2515_BAUD_1000K      3       // 1Mbps

/* 中断标志位定义 ------------------------------------------------------------*/
#define MCP2515_INT_RX0IF       0x01    // 接收缓冲区0满中断
#define MCP2515_INT_RX1IF       0x02    // 接收缓冲区1满中断
#define MCP2515_INT_TX0IF       0x04    // 发送缓冲区0空中断
#define MCP2515_INT_TX1IF       0x08    // 发送缓冲区1空中断
#define MCP2515_INT_TX2IF       0x10    // 发送缓冲区2空中断
#define MCP2515_INT_ERRIF       0x20    // 错误中断
#define MCP2515_INT_WAKIF       0x40    // 唤醒中断
#define MCP2515_INT_MERRF       0x80    // 消息错误中断

/* 函数返回值定义 ------------------------------------------------------------*/
#define MCP2515_OK              0       // 操作成功
#define MCP2515_ERROR           1       // 操作失败
#define MCP2515_TIMEOUT         2       // 操作超时

/* CAN消息结构体定义 ---------------------------------------------------------*/
typedef struct {
    uint32_t id;                        // CAN ID (11位标准ID或29位扩展ID)
    uint8_t  ide;                       // ID类型: 0=标准帧, 1=扩展帧
    uint8_t  rtr;                       // 帧类型: 0=数据帧, 1=远程帧
    uint8_t  dlc;                       // 数据长度 (0-8字节)
    uint8_t  data[8];                   // 数据内容
} MCP2515_CANMessage_t;

/* 外部变量声明 --------------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;         // SPI1句柄 (在main.c中定义)

/* 函数声明 ------------------------------------------------------------------*/

/* 底层SPI通信函数 */
uint8_t MCP2515_SPI_ReadWrite(uint8_t data);
void MCP2515_CS_Low(void);
void MCP2515_CS_High(void);

/* 寄存器读写函数 */
uint8_t MCP2515_ReadRegister(uint8_t address);
void MCP2515_WriteRegister(uint8_t address, uint8_t data);
void MCP2515_ModifyRegister(uint8_t address, uint8_t mask, uint8_t data);
void MCP2515_ReadMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length);
void MCP2515_WriteMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length);

/* 基本控制函数 */
void MCP2515_Reset(void);
uint8_t MCP2515_SetMode(uint8_t mode);
uint8_t MCP2515_GetMode(void);
uint8_t MCP2515_SetBaudRate(uint8_t baudrate);

/* 初始化和配置函数 */
uint8_t MCP2515_Init(uint8_t baudrate);
uint8_t MCP2515_ConfigureInterrupts(uint8_t interrupts);
uint8_t MCP2515_SetFilter(uint8_t filter_num, uint32_t filter_id, uint8_t extended);
uint8_t MCP2515_SetMask(uint8_t mask_num, uint32_t mask_value, uint8_t extended);

/* CAN消息收发函数 */
uint8_t MCP2515_SendMessage(MCP2515_CANMessage_t *message);
uint8_t MCP2515_ReceiveMessage(MCP2515_CANMessage_t *message);
uint8_t MCP2515_CheckReceive(void);
uint8_t MCP2515_CheckTransmit(void);

/* 中断处理函数 */
uint8_t MCP2515_GetInterruptFlags(void);
void MCP2515_ClearInterruptFlags(uint8_t flags);
void MCP2515_IRQHandler(void);

/* 状态查询函数 */
uint8_t MCP2515_GetStatus(void);
uint8_t MCP2515_GetErrorFlags(void);
void MCP2515_ClearErrorFlags(void);

/* 调试和测试函数 */
uint8_t MCP2515_SelfTest(void);
void MCP2515_PrintStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* __MCP2515_H */