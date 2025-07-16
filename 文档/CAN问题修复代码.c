/*
 * CAN问题修复代码 - 针对发送失败问题的诊断和修复
 * 作者：AI助手
 * 日期：2024
 * 说明：解决CANINTF=0xA0, EFLG=0x15导致的发送失败问题
 */

#include "mcp2515.h"
#include "main.h"
#include <stdio.h>

// 错误计数器寄存器定义
#define MCP2515_TEC     0x1C    // 发送错误计数器
#define MCP2515_REC     0x1D    // 接收错误计数器

/**
 * @brief 读取MCP2515错误计数器
 * @param tec: 发送错误计数器指针
 * @param rec: 接收错误计数器指针
 * @retval 无
 */
void MCP2515_GetErrorCounters(uint8_t *tec, uint8_t *rec)
{
    *tec = MCP2515_ReadRegister(MCP2515_TEC);
    *rec = MCP2515_ReadRegister(MCP2515_REC);
}

/**
 * @brief 详细的错误状态诊断
 * @param 无
 * @retval 无
 */
void MCP2515_DiagnoseErrors(void)
{
    uint8_t canintf, eflg, tec, rec;
    
    printf("\r\n=== MCP2515 错误诊断 ===\r\n");
    
    // 读取状态寄存器
    canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    MCP2515_GetErrorCounters(&tec, &rec);
    
    printf("CANINTF: 0x%02X\r\n", canintf);
    printf("EFLG: 0x%02X\r\n", eflg);
    printf("发送错误计数器(TEC): %d\r\n", tec);
    printf("接收错误计数器(REC): %d\r\n", rec);
    
    // 分析CANINTF
    printf("\r\n--- CANINTF 分析 ---\r\n");
    if (canintf & 0x80) printf("⚠️ MERRF: 消息错误中断\r\n");
    if (canintf & 0x40) printf("ℹ️ WAKIF: 唤醒中断\r\n");
    if (canintf & 0x20) printf("⚠️ ERRIF: 错误中断\r\n");
    if (canintf & 0x10) printf("✓ TX2IF: 发送缓冲区2中断\r\n");
    if (canintf & 0x08) printf("✓ TX1IF: 发送缓冲区1中断\r\n");
    if (canintf & 0x04) printf("✓ TX0IF: 发送缓冲区0中断\r\n");
    if (canintf & 0x02) printf("ℹ️ RX1IF: 接收缓冲区1中断\r\n");
    if (canintf & 0x01) printf("ℹ️ RX0IF: 接收缓冲区0中断\r\n");
    
    // 分析EFLG
    printf("\r\n--- EFLG 分析 ---\r\n");
    if (eflg & 0x80) printf("❌ RX1OVR: 接收缓冲区1溢出\r\n");
    if (eflg & 0x40) printf("❌ RX0OVR: 接收缓冲区0溢出\r\n");
    if (eflg & 0x20) printf("❌ TXBO: 总线关闭状态\r\n");
    if (eflg & 0x10) printf("⚠️ TXEP: 发送错误被动状态\r\n");
    if (eflg & 0x08) printf("⚠️ RXEP: 接收错误被动状态\r\n");
    if (eflg & 0x04) printf("⚠️ TXWAR: 发送错误警告\r\n");
    if (eflg & 0x02) printf("⚠️ RXWAR: 接收错误警告\r\n");
    if (eflg & 0x01) printf("⚠️ EWARN: 错误警告\r\n");
    
    // 错误级别判断
    printf("\r\n--- 错误级别判断 ---\r\n");
    if (eflg & 0x20) {
        printf("🚨 严重：总线关闭状态，需要重新初始化\r\n");
    } else if (eflg & 0x10) {
        printf("⚠️ 警告：发送错误被动状态，TEC >= 128\r\n");
        printf("   建议：检查总线连接和终端电阻\r\n");
    } else if (eflg & 0x04) {
        printf("ℹ️ 提示：发送错误警告，TEC >= 96\r\n");
    }
    
    printf("========================\r\n");
}

/**
 * @brief 清除所有错误标志和中断标志
 * @param 无
 * @retval 无
 */
void MCP2515_ClearAllErrors(void)
{
    printf("清除错误标志...\r\n");
    
    // 清除中断标志
    MCP2515_WriteRegister(MCP2515_CANINTF, 0x00);
    
    // 清除错误标志（只读寄存器，通过复位清除）
    // EFLG寄存器的某些位会在错误条件消除后自动清除
    
    printf("错误标志已清除\r\n");
}

/**
 * @brief 回环模式测试
 * @param 无
 * @retval 测试结果 (0: 成功, 1: 失败)
 */
uint8_t MCP2515_LoopbackTest(void)
{
    CAN_Message test_msg;
    CAN_Message recv_msg;
    uint8_t result = 1;
    
    printf("\r\n=== 回环模式测试 ===\r\n");
    
    // 切换到回环模式
    printf("切换到回环模式...\r\n");
    if (MCP2515_SetMode(MCP2515_MODE_LOOPBACK) != MCP2515_OK) {
        printf("❌ 切换到回环模式失败\r\n");
        return 1;
    }
    
    HAL_Delay(100);  // 等待模式切换完成
    
    // 准备测试消息
    test_msg.id = 0x123;
    test_msg.dlc = 8;
    test_msg.rtr = 0;
    test_msg.ext = 0;
    for (int i = 0; i < 8; i++) {
        test_msg.data[i] = 0xA0 + i;
    }
    
    printf("发送测试消息 ID:0x%03X...\r\n", test_msg.id);
    
    // 发送消息
    if (MCP2515_SendMessage(&test_msg) == MCP2515_OK) {
        printf("✓ 消息发送成功\r\n");
        
        // 等待一段时间
        HAL_Delay(50);
        
        // 检查是否收到消息
        if (MCP2515_CheckReceive() == MCP2515_OK) {
            if (MCP2515_ReceiveMessage(&recv_msg) == MCP2515_OK) {
                printf("✓ 收到回环消息 ID:0x%03X\r\n", recv_msg.id);
                
                // 验证数据
                if (recv_msg.id == test_msg.id && recv_msg.dlc == test_msg.dlc) {
                    uint8_t data_match = 1;
                    for (int i = 0; i < test_msg.dlc; i++) {
                        if (recv_msg.data[i] != test_msg.data[i]) {
                            data_match = 0;
                            break;
                        }
                    }
                    
                    if (data_match) {
                        printf("✅ 回环测试成功！MCP2515功能正常\r\n");
                        result = 0;
                    } else {
                        printf("❌ 数据不匹配\r\n");
                    }
                } else {
                    printf("❌ ID或DLC不匹配\r\n");
                }
            } else {
                printf("❌ 接收消息失败\r\n");
            }
        } else {
            printf("❌ 未收到回环消息\r\n");
        }
    } else {
        printf("❌ 消息发送失败\r\n");
    }
    
    // 切换回正常模式
    printf("切换回正常模式...\r\n");
    MCP2515_SetMode(MCP2515_MODE_NORMAL);
    HAL_Delay(100);
    
    printf("==================\r\n");
    return result;
}

/**
 * @brief 完整的CAN问题诊断和修复流程
 * @param 无
 * @retval 无
 */
void CAN_DiagnoseAndFix(void)
{
    printf("\r\n🔧 开始CAN问题诊断和修复流程...\r\n");
    
    // 步骤1：诊断当前错误状态
    MCP2515_DiagnoseErrors();
    
    // 步骤2：清除错误标志
    MCP2515_ClearAllErrors();
    
    // 步骤3：回环模式测试
    if (MCP2515_LoopbackTest() == 0) {
        printf("\r\n✅ MCP2515硬件功能正常\r\n");
        printf("💡 问题可能是：\r\n");
        printf("   1. 总线上没有其他CAN节点应答\r\n");
        printf("   2. 终端电阻未正确安装\r\n");
        printf("   3. CAN收发器连接问题\r\n");
        
        printf("\r\n🔧 建议解决方案：\r\n");
        printf("   1. 在CAN_H和CAN_L之间添加120Ω电阻\r\n");
        printf("   2. 连接第二个CAN节点或CAN分析仪\r\n");
        printf("   3. 检查TJA1050收发器连接\r\n");
    } else {
        printf("\r\n❌ MCP2515硬件可能有问题\r\n");
        printf("🔧 建议检查：\r\n");
        printf("   1. SPI连接是否正确\r\n");
        printf("   2. MCP2515供电是否正常\r\n");
        printf("   3. 晶振是否工作正常\r\n");
    }
    
    // 步骤4：重新初始化
    printf("\r\n🔄 重新初始化MCP2515...\r\n");
    if (MCP2515_Init(MCP2515_BAUD_500K) == MCP2515_OK) {
        printf("✓ MCP2515重新初始化成功\r\n");
    } else {
        printf("❌ MCP2515重新初始化失败\r\n");
    }
    
    printf("\r\n🏁 诊断和修复流程完成\r\n");
}

/**
 * @brief 在main.c中调用此函数进行问题诊断
 * 使用方法：在main函数的while循环前添加：
 * 
 * // 添加到USER CODE BEGIN 2区域
 * printf("\r\n🚀 CAN问题诊断启动...\r\n");
 * CAN_DiagnoseAndFix();
 * 
 * 或者在串口命令中调用此函数
 */