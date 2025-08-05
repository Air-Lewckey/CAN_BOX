/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : mcp2515.h
  * @brief          : MCP2515 CAN controller driver header file
  * @author         : lewckey
  * @version        : V1.0
  * @date           : 2025-01-XX
  ******************************************************************************
  * @attention
  *
  * This driver is suitable for STM32F407ZGT6 + MCP2515 + TJA1050 CAN communication solution
  * Hardware connections:
  * PB3  -> MCP2515 SCK  (SPI1 clock)
  * PB4  -> MCP2515 SO   (SPI1 data input MISO)
  * PB5  -> MCP2515 SI   (SPI1 data output MOSI)
  * PB10 -> MCP2515 INT  (interrupt signal)
  * PB12 -> MCP2515 CS   (chip select signal)
  * 3.3V -> MCP2515 VCC  (power positive)
  * GND  -> MCP2515 GND  (power negative)
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

/* MCP2515 register address definitions --------------------------------------------------------*/

/* Control registers */
#define MCP2515_CANCTRL     0x0F    // CAN control register
#define MCP2515_CANSTAT     0x0E    // CAN status register
#define MCP2515_CNF1        0x2A    // Configuration register 1 (baud rate configuration)
#define MCP2515_CNF2        0x29    // Configuration register 2 (baud rate configuration)
#define MCP2515_CNF3        0x28    // Configuration register 3 (baud rate configuration)

/* Interrupt registers */
#define MCP2515_CANINTE     0x2B    // Interrupt enable register
#define MCP2515_CANINTF     0x2C    // Interrupt flag register

/* Error registers */
#define MCP2515_EFLG        0x2D    // Error flag register
#define MCP2515_TEC         0x1C    // Transmit error counter
#define MCP2515_REC         0x1D    // Receive error counter

/* Transmit buffer registers (3 transmit buffers) */
#define MCP2515_TXB0CTRL    0x30    // Transmit buffer 0 control register
#define MCP2515_TXB0SIDH    0x31    // Transmit buffer 0 standard ID high byte
#define MCP2515_TXB0SIDL    0x32    // Transmit buffer 0 standard ID low byte
#define MCP2515_TXB0EID8    0x33    // Transmit buffer 0 extended ID high byte
#define MCP2515_TXB0EID0    0x34    // Transmit buffer 0 extended ID low byte
#define MCP2515_TXB0DLC     0x35    // Transmit buffer 0 data length code
#define MCP2515_TXB0D0      0x36    // Transmit buffer 0 data byte 0

#define MCP2515_TXB1CTRL    0x40    // Transmit buffer 1 control register
#define MCP2515_TXB1SIDH    0x41    // Transmit buffer 1 standard ID high byte
#define MCP2515_TXB1SIDL    0x42    // Transmit buffer 1 standard ID low byte
#define MCP2515_TXB1EID8    0x43    // Transmit buffer 1 extended ID high byte
#define MCP2515_TXB1EID0    0x44    // Transmit buffer 1 extended ID low byte
#define MCP2515_TXB1DLC     0x45    // Transmit buffer 1 data length code
#define MCP2515_TXB1D0      0x46    // Transmit buffer 1 data byte 0

#define MCP2515_TXB2CTRL    0x50    // Transmit buffer 2 control register
#define MCP2515_TXB2SIDH    0x51    // Transmit buffer 2 standard ID high byte
#define MCP2515_TXB2SIDL    0x52    // Transmit buffer 2 standard ID low byte
#define MCP2515_TXB2EID8    0x53    // Transmit buffer 2 extended ID high byte
#define MCP2515_TXB2EID0    0x54    // Transmit buffer 2 extended ID low byte
#define MCP2515_TXB2DLC     0x55    // Transmit buffer 2 data length code
#define MCP2515_TXB2D0      0x56    // Transmit buffer 2 data byte 0

/* Receive buffer registers (2 receive buffers) */
#define MCP2515_RXB0CTRL    0x60    // Receive buffer 0 control register
#define MCP2515_RXB0SIDH    0x61    // Receive buffer 0 standard ID high byte
#define MCP2515_RXB0SIDL    0x62    // Receive buffer 0 standard ID low byte
#define MCP2515_RXB0EID8    0x63    // Receive buffer 0 extended ID high byte
#define MCP2515_RXB0EID0    0x64    // Receive buffer 0 extended ID low byte
#define MCP2515_RXB0DLC     0x65    // Receive buffer 0 data length code
#define MCP2515_RXB0D0      0x66    // Receive buffer 0 data byte 0

#define MCP2515_RXB1CTRL    0x70    // Receive buffer 1 control register
#define MCP2515_RXB1SIDH    0x71    // Receive buffer 1 standard ID high byte
#define MCP2515_RXB1SIDL    0x72    // Receive buffer 1 standard ID low byte
#define MCP2515_RXB1EID8    0x73    // Receive buffer 1 extended ID high byte
#define MCP2515_RXB1EID0    0x74    // Receive buffer 1 extended ID low byte
#define MCP2515_RXB1DLC     0x75    // Receive buffer 1 data length code
#define MCP2515_RXB1D0      0x76    // Receive buffer 1 data byte 0

/* Receive filter registers (6 filters) */
#define MCP2515_RXF0SIDH    0x00    // Receive filter 0 standard ID high byte
#define MCP2515_RXF0SIDL    0x01    // Receive filter 0 standard ID low byte
#define MCP2515_RXF0EID8    0x02    // Receive filter 0 extended ID high byte
#define MCP2515_RXF0EID0    0x03    // Receive filter 0 extended ID low byte

#define MCP2515_RXF1SIDH    0x04    // Receive filter 1 standard ID high byte
#define MCP2515_RXF1SIDL    0x05    // Receive filter 1 standard ID low byte
#define MCP2515_RXF1EID8    0x06    // Receive filter 1 extended ID high byte
#define MCP2515_RXF1EID0    0x07    // Receive filter 1 extended ID low byte

#define MCP2515_RXF2SIDH    0x08    // Receive filter 2 standard ID high byte
#define MCP2515_RXF2SIDL    0x09    // Receive filter 2 standard ID low byte
#define MCP2515_RXF2EID8    0x0A    // Receive filter 2 extended ID high byte
#define MCP2515_RXF2EID0    0x0B    // Receive filter 2 extended ID low byte

#define MCP2515_RXF3SIDH    0x10    // Receive filter 3 standard ID high byte
#define MCP2515_RXF3SIDL    0x11    // Receive filter 3 standard ID low byte
#define MCP2515_RXF3EID8    0x12    // Receive filter 3 extended ID high byte
#define MCP2515_RXF3EID0    0x13    // Receive filter 3 extended ID low byte

#define MCP2515_RXF4SIDH    0x14    // Receive filter 4 standard ID high byte
#define MCP2515_RXF4SIDL    0x15    // Receive filter 4 standard ID low byte
#define MCP2515_RXF4EID8    0x16    // Receive filter 4 extended ID high byte
#define MCP2515_RXF4EID0    0x17    // Receive filter 4 extended ID low byte

#define MCP2515_RXF5SIDH    0x18    // Receive filter 5 standard ID high byte
#define MCP2515_RXF5SIDL    0x19    // Receive filter 5 standard ID low byte
#define MCP2515_RXF5EID8    0x1A    // Receive filter 5 extended ID high byte
#define MCP2515_RXF5EID0    0x1B    // Receive filter 5 extended ID low byte

/* Receive mask registers (2 masks) */
#define MCP2515_RXM0SIDH    0x20    // Receive mask 0 standard ID high byte
#define MCP2515_RXM0SIDL    0x21    // Receive mask 0 standard ID low byte
#define MCP2515_RXM0EID8    0x22    // Receive mask 0 extended ID high byte
#define MCP2515_RXM0EID0    0x23    // Receive mask 0 extended ID low byte

#define MCP2515_RXM1SIDH    0x24    // Receive mask 1 standard ID high byte
#define MCP2515_RXM1SIDL    0x25    // Receive mask 1 standard ID low byte
#define MCP2515_RXM1EID8    0x26    // Receive mask 1 extended ID high byte
#define MCP2515_RXM1EID0    0x27    // Receive mask 1 extended ID low byte

/* MCP2515 SPI command definitions --------------------------------------------------------*/
#define MCP2515_CMD_RESET       0xC0    // Reset command
#define MCP2515_CMD_READ        0x03    // Read register command
#define MCP2515_CMD_WRITE       0x02    // Write register command
#define MCP2515_CMD_RTS         0x80    // Request to send command
#define MCP2515_CMD_READ_STATUS 0xA0    // Read status command
#define MCP2515_CMD_BIT_MODIFY  0x05    // Bit modify command
#define MCP2515_CMD_LOAD_TX0    0x40    // Load transmit buffer 0 command
#define MCP2515_CMD_LOAD_TX1    0x42    // Load transmit buffer 1 command
#define MCP2515_CMD_LOAD_TX2    0x44    // Load transmit buffer 2 command
#define MCP2515_CMD_READ_RX0    0x90    // Read receive buffer 0 command
#define MCP2515_CMD_READ_RX1    0x94    // Read receive buffer 1 command

/* MCP2515 operating mode definitions --------------------------------------------------------*/
#define MCP2515_MODE_NORMAL     0x00    // Normal mode
#define MCP2515_MODE_SLEEP      0x20    // Sleep mode
#define MCP2515_MODE_LOOPBACK   0x40    // Loopback mode
#define MCP2515_MODE_LISTENONLY 0x60    // Listen only mode
#define MCP2515_MODE_CONFIG     0x80    // Configuration mode

/* CAN baud rate configuration definitions ----------------------------------------------------------*/
#define MCP2515_BAUD_125K       0       // 125Kbps
#define MCP2515_BAUD_250K       1       // 250Kbps
#define MCP2515_BAUD_500K       2       // 500Kbps
#define MCP2515_BAUD_1000K      3       // 1Mbps

/* Interrupt flag bit definitions ------------------------------------------------------------*/
#define MCP2515_INT_RX0IF       0x01    // Receive buffer 0 full interrupt
#define MCP2515_INT_RX1IF       0x02    // Receive buffer 1 full interrupt
#define MCP2515_INT_TX0IF       0x04    // Transmit buffer 0 empty interrupt
#define MCP2515_INT_TX1IF       0x08    // Transmit buffer 1 empty interrupt
#define MCP2515_INT_TX2IF       0x10    // Transmit buffer 2 empty interrupt
#define MCP2515_INT_ERRIF       0x20    // Error interrupt
#define MCP2515_INT_WAKIF       0x40    // Wake-up interrupt
#define MCP2515_INT_MERRF       0x80    // Message error interrupt

/* Function return value definitions ------------------------------------------------------------*/
#define MCP2515_OK              0       // Operation successful
#define MCP2515_ERROR           1       // Operation failed
#define MCP2515_TIMEOUT         2       // Operation timeout

/* Timeout definitions ------------------------------------------------------------------*/
#define MCP2515_SPI_TIMEOUT     100     // SPI communication timeout (ms)
#define MCP2515_MODE_TIMEOUT    100     // Mode switch timeout (ms)

/* CAN message structure definition ---------------------------------------------------------*/
typedef struct {
    uint32_t id;                        // CAN ID (11-bit standard ID or 29-bit extended ID)
    uint8_t  ide;                       // ID type: 0=standard frame, 1=extended frame
    uint8_t  rtr;                       // Frame type: 0=data frame, 1=remote frame
    uint8_t  dlc;                       // Data length (0-8 bytes)
    uint8_t  data[8];                   // Data content
} MCP2515_CANMessage_t;

/* External variable declarations --------------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;         // SPI1 handle (defined in main.c)

/* Function declarations ------------------------------------------------------------------*/

/* Low-level SPI communication functions */
uint8_t MCP2515_SPI_ReadWrite(uint8_t data);
void MCP2515_CS_Low(void);
void MCP2515_CS_High(void);

/* Register read/write functions */
uint8_t MCP2515_ReadRegister(uint8_t address);
void MCP2515_WriteRegister(uint8_t address, uint8_t data);
void MCP2515_ModifyRegister(uint8_t address, uint8_t mask, uint8_t data);
void MCP2515_BitModify(uint8_t address, uint8_t mask, uint8_t data);
void MCP2515_ReadMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length);
void MCP2515_WriteMultipleRegisters(uint8_t address, uint8_t *buffer, uint8_t length);

/* Basic control functions */
void MCP2515_Reset(void);
uint8_t MCP2515_SetMode(uint8_t mode);
uint8_t MCP2515_GetMode(void);
uint8_t MCP2515_SetBaudRate(uint8_t baudrate);
uint8_t MCP2515_WaitForMode(uint8_t mode, uint32_t timeout);

/* Initialization and configuration functions */
uint8_t MCP2515_Init(uint8_t baudrate);
uint8_t MCP2515_ConfigureInterrupts(uint8_t interrupts);
uint8_t MCP2515_SetFilter(uint8_t filter_num, uint32_t filter_id, uint8_t extended);
uint8_t MCP2515_SetMask(uint8_t mask_num, uint32_t mask_value, uint8_t extended);

/* CAN message transmit/receive functions */
uint8_t MCP2515_SendMessage(MCP2515_CANMessage_t *message);
uint8_t MCP2515_ReceiveMessage(MCP2515_CANMessage_t *message);
uint8_t MCP2515_CheckReceive(void);
uint8_t MCP2515_CheckTransmit(void);

/* Interrupt handling functions */
uint8_t MCP2515_GetInterruptFlags(void);
uint8_t MCP2515_GetInterruptFlags_Debug(void);
void MCP2515_ClearInterruptFlags(uint8_t flags);
void MCP2515_IRQHandler(void);
uint8_t MCP2515_ProcessPendingInterrupt(void);

/* Status query functions */
uint8_t MCP2515_GetStatus(void);
uint8_t MCP2515_GetErrorFlags(void);
void MCP2515_ClearErrorFlags(void);

/* Debug and test functions */
void MCP2515_SetFilterForAll(void);        // Configure filter to receive all messages
void MCP2515_ModeNormal(void);             // Ensure in normal operating mode
void MCP2515_Test500KConfigs(void);        // Test different baud rate configurations
uint8_t MCP2515_SelfTest(void);
uint8_t MCP2515_HardwareTest(void);
void MCP2515_PrintStatus(void);
void Simple_CS_Test(void);

/* Error diagnosis and recovery functions */
void MCP2515_GetErrorCounters(uint8_t *tec, uint8_t *rec);
void MCP2515_DiagnoseErrors(void);
void MCP2515_ClearAllErrors(void);
uint8_t MCP2515_RecoverFromBusOff(void);
uint8_t MCP2515_LoopbackTest(void);
void CAN_DiagnoseAndFix(void);
void MCP2515_CANOETest(void);
void MCP2515_InitFailureDiagnosis(void);
void MCP2515_HardwareDiagnosis(void);
void MCP2515_VerifyInitialization(void);
#ifdef __cplusplus
}
#endif

#endif /* __MCP2515_H */