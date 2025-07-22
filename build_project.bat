@echo off
echo ========================================
echo STM32F407 CAN Loop Test Build Script
echo ========================================
echo.

echo [INFO] Starting build process...
echo [INFO] Project: CAN_BOX
echo [INFO] Target: STM32F407 + MCP2515 CAN Loop Test
echo.

REM 检查STM32CubeIDE是否安装
if not exist "C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeide.exe" (
    echo [ERROR] STM32CubeIDE not found at default location
    echo [INFO] Please check your STM32CubeIDE installation path
    echo [INFO] Default path: C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeide.exe
    pause
    exit /b 1
)

echo [INFO] STM32CubeIDE found, starting build...
echo.

REM 切换到项目目录
cd /d "%~dp0"

REM 使用STM32CubeIDE命令行编译
echo [BUILD] Compiling project...
"C:\ST\STM32CubeIDE_1.19.0\STM32CubeIDE\stm32cubeide.exe" --launcher.suppressErrors -nosplash -application org.eclipse.cdt.managedbuilder.core.headlessbuild -data . -build CAN_BOX/Debug

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [SUCCESS] Build completed successfully!
    echo [INFO] Output files location: Debug folder
    echo [INFO] Main executable: CAN_BOX.elf
    echo.
    echo [NEXT STEPS]
    echo 1. Connect ST-Link debugger to STM32F407
    echo 2. Connect MCP2515 module via SPI1
    echo 3. Connect CAN1 and MCP2515 CAN with jumper wires
    echo 4. Flash the firmware using STM32CubeIDE or ST-Link Utility
    echo 5. Open serial terminal (115200 baud) to view test logs
    echo.
) else (
    echo.
    echo [ERROR] Build failed with error code %ERRORLEVEL%
    echo [INFO] Please check the error messages above
    echo [INFO] Common issues:
    echo   - Missing source files
    echo   - Compilation errors
    echo   - Missing dependencies
    echo.
)

echo [INFO] Build process completed.
echo ========================================
pause