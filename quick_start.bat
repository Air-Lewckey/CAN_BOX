@echo off
chcp 65001 >nul
color 0A
echo.
echo ╔══════════════════════════════════════════════════════════════╗
echo ║              STM32F407 + MCP2515 CAN循环测试                ║
echo ║                     快速启动向导                             ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo [INFO] 欢迎使用STM32F407 + MCP2515双CAN循环测试系统！
echo.
echo 📋 测试概述：
echo    • STM32内置CAN1 发送消息给 MCP2515
echo    • MCP2515 接收后转发回 STM32 CAN1
echo    • 完成一轮循环，1秒后开始下一轮
echo    • 实时显示英文测试日志和统计信息
echo.
echo 🔌 硬件连接检查：
echo    请确认以下连接正确：
echo.
echo    SPI连接：
echo    ├─ STM32 PA5 (SCK)  ←→ MCP2515 SCK
echo    ├─ STM32 PA6 (MISO) ←→ MCP2515 SO
echo    ├─ STM32 PA7 (MOSI) ←→ MCP2515 SI
echo    ├─ STM32 PA4 (CS)   ←→ MCP2515 CS
echo    └─ STM32 PA3 (INT)  ←→ MCP2515 INT
echo.
echo    CAN循环连接（重要！）：
echo    ├─ STM32 PD1 (CAN1_TX) ←→ MCP2515 CAN_H
echo    └─ STM32 PD0 (CAN1_RX) ←→ MCP2515 CAN_L
echo.
echo    串口调试：
echo    ├─ STM32 PA2 (USART2_TX) ←→ USB转串口 RX
echo    └─ STM32 PA3 (USART2_RX) ←→ USB转串口 TX
echo.
pause
cls

echo ╔══════════════════════════════════════════════════════════════╗
echo ║                        编译选项                              ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo 请选择编译方式：
echo.
echo [1] 自动编译项目（推荐）
echo [2] 手动编译指导
echo [3] 跳过编译，直接查看测试指南
echo [4] 退出
echo.
set /p choice="请输入选择 (1-4): "

if "%choice%"=="1" goto auto_build
if "%choice%"=="2" goto manual_build
if "%choice%"=="3" goto test_guide
if "%choice%"=="4" goto exit
echo [ERROR] 无效选择，请重新运行脚本
pause
goto exit

:auto_build
cls
echo ╔══════════════════════════════════════════════════════════════╗
echo ║                        自动编译                              ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo [INFO] 开始自动编译项目...
echo.

REM 检查STM32CubeIDE
if not exist "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeide.exe" (
    echo [ERROR] 未找到STM32CubeIDE，请检查安装路径
    echo [INFO] 默认路径: C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeide.exe
    echo.
    echo 请手动编译或安装STM32CubeIDE
    pause
    goto manual_build
)

echo [BUILD] 正在编译项目，请稍候...
call build_project.bat

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✅ [SUCCESS] 编译完成！
    echo.
    goto flash_guide
) else (
    echo.
    echo ❌ [ERROR] 编译失败，请检查错误信息
    echo.
    pause
    goto manual_build
)

:manual_build
cls
echo ╔══════════════════════════════════════════════════════════════╗
echo ║                        手动编译指导                          ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo 📝 手动编译步骤：
echo.
echo 1. 打开STM32CubeIDE
echo    File → Open Projects from File System
echo.
echo 2. 导入项目
echo    选择当前文件夹: %~dp0
echo    点击 "Finish"
echo.
echo 3. 编译项目
echo    Project → Build All (Ctrl+B)
echo    或点击工具栏的锤子图标
echo.
echo 4. 检查编译结果
echo    确保Console窗口显示 "Build Finished"
echo    无错误和警告信息
echo.
echo 5. 准备烧录
echo    连接ST-Link调试器到STM32F407
echo    Run → Debug As → STM32 MCU C/C++ Application
echo.
pause
goto flash_guide

:flash_guide
cls
echo ╔══════════════════════════════════════════════════════════════╗
echo ║                        烧录和测试指导                        ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo 🔥 烧录步骤：
echo.
echo 1. 连接ST-Link调试器
echo    ST-Link ←→ STM32F407开发板
echo.
echo 2. 在STM32CubeIDE中烧录
echo    Run → Debug As → STM32 MCU C/C++ Application
echo    或按F11键
echo.
echo 3. 等待烧录完成
echo    Console窗口显示 "Download verified successfully"
echo.
echo 🖥️ 串口监控设置：
echo.
echo 1. 打开串口终端软件（如PuTTY、SecureCRT等）
echo 2. 设置串口参数：
echo    • 波特率: 115200
echo    • 数据位: 8
echo    • 停止位: 1  
echo    • 校验位: 无
echo    • 流控: 无
echo.
echo 3. 连接USB转串口到电脑
echo 4. 选择正确的COM端口
echo.
pause
goto test_guide

:test_guide
cls
echo ╔══════════════════════════════════════════════════════════════╗
echo ║                        测试执行指导                          ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo 🚀 开始测试：
echo.
echo 1. 给STM32F407开发板上电
echo 2. 观察串口输出，应该看到：
echo.
echo    === CAN Loop Communication Test Initialization ===
echo    Test Mode: STM32 CAN1 -^> MCP2515 -^> STM32 CAN1
echo    Test Period: 1000 ms
echo    ================================================
echo.
echo 3. 正常循环测试日志示例：
echo.
echo    [LOOP #1] STM32 CAN1 -^> Message sent to MCP2515 (Time: 5000 ms)
echo    [RELAY] MCP2515 received message from STM32 CAN1 (Time: 5001 ms)
echo    [RELAY] MCP2515 -^> Message relayed to STM32 CAN1
echo    [LOOP #1] STM32 CAN1 ^<- Message received from MCP2515 (Loop time: 15 ms)
echo    [SUCCESS] Loop #1 completed successfully
echo.
echo 📊 性能评估标准：
echo.
echo    ✅ 成功率 ≥ 95%%     : 通信稳定
echo    ✅ 循环时间 ^< 50ms   : 响应及时  
echo    ✅ 无连续超时       : 系统可靠
echo    ✅ 数据完整性       : 收发一致
echo.
echo 🔍 故障排除：
echo.
echo    • 无串口输出 → 检查串口连接和波特率
echo    • 循环超时   → 检查CAN线连接
echo    • 数据错误   → 检查硬件和干扰
echo    • MCP2515错误 → 检查SPI连接
echo.
echo 📚 详细测试指南请查看: CAN_Loop_Test_Guide.md
echo.
pause

echo.
echo ╔══════════════════════════════════════════════════════════════╗
echo ║                        测试完成                              ║
echo ╚══════════════════════════════════════════════════════════════╝
echo.
echo 🎉 恭喜！您已完成STM32F407 + MCP2515双CAN循环测试的所有准备工作
echo.
echo 📋 接下来您可以：
echo.
echo [1] 查看详细测试指南 (CAN_Loop_Test_Guide.md)
echo [2] 查看项目README (README.md)
echo [3] 重新运行此向导
echo [4] 退出
echo.
set /p final_choice="请输入选择 (1-4): "

if "%final_choice%"=="1" (
    echo [INFO] 正在打开测试指南...
    start CAN_Loop_Test_Guide.md
)
if "%final_choice%"=="2" (
    echo [INFO] 正在打开项目README...
    start README.md
)
if "%final_choice%"=="3" (
    echo [INFO] 重新启动向导...
    goto :eof
    call "%~f0"
)

:exit
echo.
echo 👋 感谢使用STM32F407 + MCP2515双CAN循环测试系统！
echo 🔧 技术支持: 正点原子技术团队
echo 📧 问题反馈: support@alientek.com
echo.
pause
exit /b 0