# STM32F407 CAN_BOX项目GitHub上传指南

## 前期准备

### 1. 安装Git
- 下载Git：https://git-scm.com/download/win
- 安装时选择默认选项即可
- 验证安装：在命令行运行 `git --version`

### 2. 配置Git用户信息
```bash
git config --global user.name "你的用户名"
git config --global user.email "你的邮箱@example.com"
```

### 3. 创建GitHub仓库
1. 登录GitHub网站：https://github.com
2. 点击右上角的"+"号，选择"New repository"
3. 填写仓库信息：
   - Repository name: `STM32F407_MCP2515_CAN`
   - Description: `STM32F407 + MCP2515 CAN通信系统`
   - 选择Public或Private
   - **不要**勾选"Initialize this repository with a README"
4. 点击"Create repository"

## 项目上传步骤

### 步骤1：在项目目录初始化Git仓库
```bash
# 打开命令行，导航到项目目录
cd d:\STM32CubeIDEworkspace\CAN_BOX

# 初始化Git仓库
git init
```

### 步骤2：创建.gitignore文件
创建`.gitignore`文件，排除不需要上传的文件：

```gitignore
# STM32CubeIDE生成的文件
Debug/
Release/
.metadata/
.settings/
*.launch
*.tmp
*.bak
*.swp
*~.nib
local.properties
.loadpath
.recommenders

# Eclipse项目文件（保留）
# .project
# .cproject

# 编译输出文件
*.o
*.elf
*.bin
*.hex
*.map
*.list
*.su
*.d

# 临时文件
*.log
*.tmp
*.temp

# Windows系统文件
Thumbs.db
ehthumbs.db
Desktop.ini
$RECYCLE.BIN/

# macOS系统文件
.DS_Store
.AppleDouble
.LSOverride

# 备份文件
*.bak
*.backup
*~

# IDE特定文件
.vscode/
.idea/
*.sublime-*

# 用户特定配置
*.user
*.suo
*.userosscache
*.sln.docstates
```

### 步骤3：添加文件到Git
```bash
# 添加所有文件（.gitignore会自动排除不需要的文件）
git add .

# 查看将要提交的文件
git status
```

### 步骤4：提交代码
```bash
# 提交代码
git commit -m "Initial commit: STM32F407 + MCP2515 CAN communication system

- Complete STM32CubeIDE project structure
- MCP2515 SPI driver implementation
- CAN application layer with FreeRTOS
- Hardware abstraction layer
- Comprehensive documentation
- Test guides and troubleshooting"
```

### 步骤5：连接到GitHub仓库
```bash
# 添加远程仓库（替换为你的GitHub用户名和仓库名）
git remote add origin https://github.com/Air-Lewckey/CAN_BOX.git

# 验证远程仓库
git remote -v
```

### 步骤6：推送到GitHub
```bash
# 推送到GitHub（首次推送）
git push -u origin main

# 如果遇到分支名问题，可能需要：
# git branch -M main
# git push -u origin main
```

## 后续更新操作

### 日常提交流程
```bash
# 1. 查看修改状态
git status

# 2. 添加修改的文件
git add .
# 或者添加特定文件
git add Core/Src/main.c

# 3. 提交修改
git commit -m "描述你的修改内容"

# 4. 推送到GitHub
git push
```

### 常用Git命令
```bash
# 查看提交历史
git log --oneline

# 查看文件修改
git diff

# 撤销未提交的修改
git checkout -- 文件名

# 查看分支
git branch

# 创建新分支
git checkout -b 新分支名

# 切换分支
git checkout 分支名
```

## 项目结构说明

上传到GitHub的项目将包含：

```
STM32F407_MCP2515_CAN/
├── Core/                          # STM32核心代码
│   ├── Inc/                       # 头文件
│   ├── Src/                       # 源文件
│   └── Startup/                   # 启动文件
├── Drivers/                       # STM32 HAL驱动
├── Middlewares/                   # FreeRTOS中间件
├── *.md                          # 项目文档
├── *.ioc                         # STM32CubeMX配置文件
├── *.ld                          # 链接脚本
├── .project                      # Eclipse项目文件
├── .cproject                     # C/C++项目配置
└── .gitignore                    # Git忽略文件
```

## 注意事项

### 1. 敏感信息保护
- 不要上传包含密码、密钥的文件
- 检查代码中是否有硬编码的敏感信息

### 2. 文件大小限制
- GitHub单个文件限制100MB
- 仓库总大小建议不超过1GB

### 3. 许可证选择
建议在GitHub仓库中添加许可证文件：
- MIT License（推荐，开源友好）
- Apache License 2.0
- GPL v3.0

### 4. README文件
建议创建一个详细的README.md文件，包含：
- 项目描述
- 硬件要求
- 编译说明
- 使用方法
- 贡献指南

## 故障排除

### 推送失败
```bash
# 如果推送被拒绝，先拉取远程更新
git pull origin main

# 解决冲突后再推送
git push
```

### 认证问题
- 使用Personal Access Token代替密码
- 在GitHub Settings → Developer settings → Personal access tokens中生成

### 大文件问题
```bash
# 如果有大文件，使用Git LFS
git lfs install
git lfs track "*.bin"
git lfs track "*.hex"
git add .gitattributes
```

## 完成后的效果

上传成功后，你的GitHub仓库将包含：
- ✅ 完整的STM32项目源码
- ✅ 详细的项目文档
- ✅ 清晰的项目结构
- ✅ 版本控制历史
- ✅ 便于协作和分享

这样其他开发者就可以通过以下命令克隆你的项目：
```bash
git clone https://github.com/Air-Lewckey/CAN_BOX.git
```

## 推荐的提交信息格式

```
类型(范围): 简短描述

详细描述（可选）

- 具体修改点1
- 具体修改点2
```

类型示例：
- `feat`: 新功能
- `fix`: 修复bug
- `docs`: 文档更新
- `style`: 代码格式
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建过程或辅助工具的变动

现在可以开始上传你的项目到GitHub了！