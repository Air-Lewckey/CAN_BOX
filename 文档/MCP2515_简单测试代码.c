/* MCP2515简单测试代码 - 用于故障排除 */
/* 将此代码添加到main.c的USER CODE BEGIN 2部分，替换现有的CAN_App_Init()调用 */

// 在main.c中添加以下头文件
// #include "mcp2515.h"

void MCP2515_SimpleTest(void)
{
    printf("\r\n=== MCP2515 Simple Connection Test ===\r\n");
    
    // 步骤1：基础连接测试
    printf("Step 1: Basic SPI Communication Test\r\n");
    
    // 复位MCP2515
    printf("Resetting MCP2515...\r\n");
    MCP2515_Reset();
    HAL_Delay(20);  // 等待复位完成
    
    // 读取CANSTAT寄存器（复位后应该是0x80 - Configuration模式）
    uint8_t canstat = MCP2515_ReadRegister(MCP2515_CANSTAT);
    printf("CANSTAT after reset: 0x%02X (Expected: 0x80)\r\n", canstat);
    
    if (canstat == 0x80) {
        printf("✓ SPI communication working - MCP2515 detected\r\n");
    } else if (canstat == 0xFF) {
        printf("✗ SPI communication failed - Check connections\r\n");
        printf("  Possible issues:\r\n");
        printf("  - MOSI/MISO/SCK/CS pin connections\r\n");
        printf("  - Power supply (VCC/GND)\r\n");
        printf("  - SPI configuration\r\n");
        return;
    } else {
        printf("? Unexpected CANSTAT value - Possible communication issue\r\n");
    }
    
    // 步骤2：寄存器读写测试
    printf("\r\nStep 2: Register Read/Write Test\r\n");
    
    // 测试CNF1寄存器（可读写）
    uint8_t test_values[] = {0xAA, 0x55, 0x00, 0xFF};
    uint8_t test_passed = 1;
    
    for (int i = 0; i < 4; i++) {
        MCP2515_WriteRegister(MCP2515_CNF1, test_values[i]);
        HAL_Delay(1);
        uint8_t read_back = MCP2515_ReadRegister(MCP2515_CNF1);
        
        printf("Write: 0x%02X, Read: 0x%02X - %s\r\n", 
               test_values[i], read_back, 
               (read_back == test_values[i]) ? "PASS" : "FAIL");
        
        if (read_back != test_values[i]) {
            test_passed = 0;
        }
    }
    
    if (test_passed) {
        printf("✓ Register read/write test PASSED\r\n");
    } else {
        printf("✗ Register read/write test FAILED\r\n");
        printf("  Possible issues:\r\n");
        printf("  - SPI timing issues (try lower SPI speed)\r\n");
        printf("  - Noisy connections\r\n");
        printf("  - Power supply instability\r\n");
        return;
    }
    
    // 步骤3：模式切换测试
    printf("\r\nStep 3: Mode Switch Test\r\n");
    
    // 尝试切换到Normal模式
    printf("Switching to Normal mode...\r\n");
    MCP2515_ModifyRegister(MCP2515_CANCTRL, 0xE0, MCP2515_MODE_NORMAL);
    HAL_Delay(10);
    
    uint8_t current_mode = MCP2515_ReadRegister(MCP2515_CANSTAT) & 0xE0;
    printf("Current mode: 0x%02X (Expected: 0x00 for Normal)\r\n", current_mode);
    
    if (current_mode == MCP2515_MODE_NORMAL) {
        printf("✓ Mode switch to Normal SUCCESSFUL\r\n");
        printf("✓ MCP2515 is working correctly!\r\n");
    } else {
        printf("✗ Mode switch FAILED - Still in Configuration mode\r\n");
        printf("  Possible issues:\r\n");
        printf("  - Crystal oscillator not working (check 8MHz crystal)\r\n");
        printf("  - CAN bus termination issues\r\n");
        printf("  - Hardware fault in MCP2515\r\n");
    }
    
    // 步骤4：详细状态信息
    printf("\r\nStep 4: Detailed Status Information\r\n");
    uint8_t canctrl = MCP2515_ReadRegister(MCP2515_CANCTRL);
    uint8_t canintf = MCP2515_ReadRegister(MCP2515_CANINTF);
    uint8_t eflg = MCP2515_ReadRegister(MCP2515_EFLG);
    
    printf("CANSTAT: 0x%02X\r\n", canstat);
    printf("CANCTRL: 0x%02X\r\n", canctrl);
    printf("CANINTF: 0x%02X\r\n", canintf);
    printf("EFLG: 0x%02X\r\n", eflg);
    
    // 解析错误标志
    if (eflg != 0) {
        printf("Error flags detected:\r\n");
        if (eflg & 0x01) printf("  - EWARN: Error warning\r\n");
        if (eflg & 0x02) printf("  - RXWAR: Receive error warning\r\n");
        if (eflg & 0x04) printf("  - TXWAR: Transmit error warning\r\n");
        if (eflg & 0x08) printf("  - RXEP: Receive error passive\r\n");
        if (eflg & 0x10) printf("  - TXEP: Transmit error passive\r\n");
        if (eflg & 0x20) printf("  - TXBO: Transmit bus-off\r\n");
    }
    
    printf("\r\n=== Test Complete ===\r\n");
    
    // 根据测试结果给出建议
    if (canstat == 0x80 && !test_passed) {
        printf("\r\nRECOMMENDATION: SPI communication issue\r\n");
        printf("1. Check all SPI pin connections\r\n");
        printf("2. Try reducing SPI speed (increase prescaler)\r\n");
        printf("3. Check power supply stability\r\n");
    } else if (canstat == 0x80 && test_passed && current_mode != MCP2515_MODE_NORMAL) {
        printf("\r\nRECOMMENDATION: Crystal oscillator issue\r\n");
        printf("1. Check 8MHz crystal on MCP2515 module\r\n");
        printf("2. Verify crystal is soldered properly\r\n");
        printf("3. If using 16MHz crystal, update baud rate config\r\n");
    } else if (canstat != 0x80) {
        printf("\r\nRECOMMENDATION: Hardware connection issue\r\n");
        printf("1. Check all pin connections with multimeter\r\n");
        printf("2. Verify power supply voltage (3.3V or 5V)\r\n");
        printf("3. Check ground connections\r\n");
        printf("4. Try different MCP2515 module if available\r\n");
    } else {
        printf("\r\nSUCCESS: MCP2515 is working correctly!\r\n");
        printf("You can now proceed with CAN communication.\r\n");
    }
}

/* 使用方法：
 * 1. 在main.c的USER CODE BEGIN 2部分，注释掉现有的CAN_App_Init()调用
 * 2. 添加调用：MCP2515_SimpleTest();
 * 3. 编译并烧录程序
 * 4. 查看串口输出，根据测试结果进行故障排除
 * 
 * 示例代码位置：
 * 
 * // USER CODE BEGIN 2
 * // 注释掉原来的代码
 * // if (CAN_App_Init() == CAN_APP_OK) { ... }
 * 
 * // 添加测试代码
 * MCP2515_SimpleTest();
 * // USER CODE END 2
 */