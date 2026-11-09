# RetroDiscOS
# RetroDisc OS 产品需求文档（PRD）

**项目名称：** RetroDisc OS  
**产品名称：** RetroDisc CD Player  
**文档版本：** V0.1  
**文档状态：** 初始方案  
**项目类型：** 基于 Debian 的定制化桌面 CD 播放系统

---

# 第一部分：产品概述

## 1.1 项目背景

本项目旨在利用现有 x86 笔记本电脑、USB 外置 CD 光驱及可选 USB DAC，开发一套具有复古 Hi-Fi 风格的桌面 CD 播放系统。

系统以 Debian Minimal 为底层，通过自主开发的播放器程序实现 CD 识别、音频播放、显示交互、播放控制及硬件管理。

最终设备不再表现为普通笔记本电脑，而是作为一台专用桌面 CD 播放机使用。

## 1.2 产品定位

RetroDisc OS 是一套专注于 CD-DA 播放的轻量级 Linux 系统。

核心定位：

- 复古 Hi-Fi 桌面 CD 播放机。
- 支持实体 CD 自动识别与播放。
- 支持 USB DAC 音频输出。
- 具有多种复古显示风格。
- 具有传统 CD 播放机的完整控制功能。
- 支持离线使用。
- 支持开机自动进入播放器。
- 支持系统诊断和硬件状态监测。

## 1.3 产品设计原则

1. 专用化：开机直接进入播放器，不显示普通桌面。
2. 轻量化：使用 Debian Minimal，减少不必要的后台服务。
3. 稳定性：优先保证 CD 播放稳定，不因界面或其他功能异常中断播放。
4. 低功耗：减少不必要的 CPU、显示和磁盘活动。
5. 低写入：减少系统日志和配置文件的频繁写入。
6. 可维护：保留 SSH、日志、诊断及系统恢复能力。
7. 可扩展：支持后续增加 DAC、显示主题及播放功能。

## 1.4 目标硬件

| 硬件 | 目标配置 | 说明 |
|---|---|---|
| 主机 | Intel x86-64 笔记本 | 最终运行设备 |
| 操作系统 | Debian Stable | 定制系统底层 |
| 存储 | 笔记本 SSD | 与 Windows 双系统共存 |
| 光驱 | USB 外置 CD-ROM | 读取实体 CD |
| 音频设备 | USB DAC（可选） | 外接音频输出 |
| 显示设备 | 笔记本内置屏幕 | 播放信息及交互 |
| 输入设备 | 笔记本键盘 | 主要控制方式 |
| 其他输入 | USB 鼠标、键盘 | 允许使用 |
| 无线设备 | Wi-Fi、蓝牙 | 不作为系统功能，优先关闭 |

---

# 第二部分：功能需求

## 2.1 系统启动与运行

| 编号 | 功能 | 需求描述 | 优先级 |
|---|---|---|---|
| SYS-001 | 自动启动 | 开机后自动启动 RetroDisc Player | P0 |
| SYS-002 | 全屏运行 | 播放器以全屏专用界面运行 | P0 |
| SYS-003 | 离线运行 | 不依赖互联网完成 CD 播放 | P0 |
| SYS-004 | 双系统 | 与 Windows 共存，保留原有系统 | P0 |
| SYS-005 | 异常恢复 | 播放器异常退出后可自动重启 | P1 |
| SYS-006 | 系统退出 | 支持安全关机和重启 | P1 |
| SYS-007 | 维护入口 | 支持进入系统维护模式 | P1 |

## 2.2 CD 光驱管理

| 编号 | 功能 | 需求描述 | 优先级 |
|---|---|---|---|
| CD-001 | 光盘检测 | 插入 CD 后自动识别 | P0 |
| CD-002 | TOC 读取 | 读取曲目数量、起始位置和时长 | P0 |
| CD-003 | CD-Text | 支持读取光盘中的 CD-Text | P1 |
| CD-004 | 默认命名 | 无曲目名称时显示 TRACK 01 等 | P0 |
| CD-005 | 自动播放 | 支持插入后自动播放或等待用户操作 | P0 |
| CD-006 | 光盘停止 | 停止播放后尽可能停止光盘旋转 | P1 |
| CD-007 | 光驱释放 | 弹出光盘前停止播放并释放设备 | P0 |
| CD-008 | 光盘更换 | 识别光盘更换并更新曲目信息 | P0 |
| CD-009 | 光盘信息 | 显示曲目数量、总时长、当前曲目 | P0 |
| CD-010 | 错误处理 | 光盘读取失败时显示错误信息 | P0 |

注：光驱是否支持停止旋转、弹出及相关控制，需要以实际设备能力为准。

## 2.3 音频播放

| 编号 | 功能 | 需求描述 | 优先级 |
|---|---|---|---|
| AUD-001 | 播放 | 播放当前曲目 | P0 |
| AUD-002 | 暂停 | 暂停及恢复播放 | P0 |
| AUD-003 | 停止 | 停止当前播放 | P0 |
| AUD-004 | 上一曲 | 切换到上一曲 | P0 |
| AUD-005 | 下一曲 | 切换到下一曲 | P0 |
| AUD-006 | 快进 | 支持曲目内快速定位 | P1 |
| AUD-007 | 快退 | 支持曲目内快速定位 | P1 |
| AUD-008 | 音量 | 支持音量调节 | P0 |
| AUD-009 | 静音 | 支持快速静音 | P1 |
| AUD-010 | DAC | 支持 USB DAC 输出 | P0 |
| AUD-011 | 输出选择 | 支持选择可用音频设备 | P1 |
| AUD-012 | 播放进度 | 显示当前曲目进度及剩余时间 | P0 |

音频播放应优先考虑数字音频读取与输出链路的可靠性。音量调节应明确是软件数字音量还是 DAC 硬件音量，不应默认保证 Bit-Perfect。

## 2.4 播放模式

| 编号 | 功能 | 需求描述 | 优先级 |
|---|---|---|---|
| PLAY-001 | 单曲循环 | 重复播放当前曲目 | P1 |
| PLAY-002 | 整碟循环 | 重复播放整张 CD | P1 |
| PLAY-003 | 随机播放 | 随机选择曲目 | P1 |
| PLAY-004 | 顺序播放 | 按光盘曲目顺序播放 | P0 |
| PLAY-005 | A-B 循环 | 循环播放指定时间段 | P2 |
| PLAY-006 | 编程播放 | 自定义曲目播放顺序 | P2 |
| PLAY-007 | INTRO | 每首曲目试听指定时长 | P2 |
| PLAY-008 | 断点续播 | 记录上次播放曲目和位置 | P1 |

断点续播应使用光盘 TOC 等信息辅助判断是否为同一张光盘，避免换盘后错误恢复。

## 2.5 显示系统

### 显示主题

| 主题 | 设计方向 | 优先级 |
|---|---|---|
| Classic VFD | 经典绿色荧光屏风格 | P0 |
| Amber | 琥珀色复古显示风格 | P1 |
| Digital Hi-Fi | 现代数字 Hi-Fi 风格 | P0 |
| Vintage Analog | 指针、模拟仪表风格 | P1 |
| Display Off | 关闭显示内容及背光 | P1 |

### 显示内容

主界面至少包含：

- 当前曲目编号。
- 当前曲目名称。
- 总曲目数量。
- 当前播放时间。
- 曲目总时长。
- 播放状态。
- 播放模式。
- 音量状态。
- 光驱状态。
- 音频输出设备状态。

### 亮度管理

支持以下亮度档位：

100%、75%、50%、25%、0%。

需要区分：

1. UI 视觉亮度：界面元素的亮度。
2. 硬件背光亮度：通过笔记本背光接口调节实际屏幕亮度。

0% 档位应尽可能关闭背光，但保留播放器运行及键盘唤醒能力。

## 2.6 按键管理

所有按键映射均应支持配置文件修改。

默认按键方案：

| 按键 | 功能 |
|---|---|
| Space | 播放 / 暂停 |
| Enter | 确认 |
| Esc | 返回 |
| ← / → | 上一曲 / 下一曲 |
| ↑ / ↓ | 音量增减 |
| F1 | 播放模式 |
| F2 | 随机播放 |
| F3 | 循环模式 |
| F4 | 专辑 / 曲目信息 |
| F5 | 显示主题切换 |
| F6 / F7 | 亮度减 / 增 |
| F8 | 显示开关 |
| F9 | 频谱显示 |
| F10 | 静音 |
| F11 / F12 | 快退 / 快进 |

电源键：

- 短按：息屏或唤醒，具体行为需要真机验证。
- 长按：保留固件硬件强制关机能力。
- 不应通过软件设置破坏硬件过热保护。

## 2.7 HELP 离线帮助

播放器内置离线帮助页面。

至少包含：

1. 快速入门。
2. 按键说明。
3. CD 播放控制。
4. 播放模式说明。
5. 显示主题设置。
6. 亮度设置。
7. 光驱管理。
8. DAC 音频设置。
9. 系统设置。
10. 常见故障排查。

## 2.8 SERVICE MODE 诊断模式

提供系统和硬件状态查看界面。

| 诊断项目 | 显示内容 |
|---|---|
| CD-ROM | 设备识别、光盘状态、读取状态 |
| DAC | 音频设备识别、输出状态 |
| Keyboard | 按键事件检测 |
| Display | 分辨率、显示状态 |
| Backlight | 背光设备及亮度 |
| ACPI | 电源事件状态 |
| CPU | CPU 型号、频率、温度 |
| Fan | 风扇状态，仅监测 |
| Storage | SSD 容量和剩余空间 |
| System | 系统版本、播放器版本 |
| Log | 查看及导出诊断日志 |

SERVICE MODE 不应默认开放风扇控制、内核修改、分区修改等高风险操作。

---

# 第三部分：非功能需求

## 3.1 稳定性

- 播放器长时间运行不应出现持续性内存泄漏。
- 光驱拔出或读取失败时，播放器不得崩溃。
- DAC 断开后应显示异常状态。
- 播放器异常退出后应允许重新启动。
- 播放状态和光驱状态应保持一致。

## 3.2 性能

- 系统开机后自动进入播放器。
- 界面操作应保持流畅。
- 播放期间 CPU 占用应尽量低。
- 不运行不必要的后台服务。
- 不需要专辑封面数据库或大型媒体索引。

## 3.3 功耗

- 支持 CPUFreq / CPUIdle 等常规节能机制。
- 优先关闭不需要的 Wi-Fi 和蓝牙。
- 支持背光调节。
- 避免不必要的 USB 唤醒和设备活动。
- 不安装主动风扇控制程序。
- 保留 BIOS / EC 风扇管理和硬件过热保护。

## 3.4 存储写入

- 减少不必要的系统日志写入。
- 限制 journald 日志容量。
- 临时文件优先使用 tmpfs。
- 播放位置仅在必要节点保存。
- 不建立大型媒体数据库。
- 不进行无意义的后台扫描。

## 3.5 可维护性

- 提供日志导出。
- 提供版本信息。
- 支持 SSH 维护。
- 配置文件与程序分离。
- 重要功能具有独立测试。
- 支持回滚至已知稳定版本。

---

# 第四部分：产品验收标准

| 验收编号 | 验收内容 | 合格标准 |
|---|---|---|
| AC-001 | 系统启动 | 自动进入播放器 |
| AC-002 | CD 识别 | 正确识别实体 CD |
| AC-003 | 曲目读取 | 曲目数量及时间正确 |
| AC-004 | 基础播放 | 播放、暂停、停止正常 |
| AC-005 | 曲目控制 | 上一曲、下一曲正常 |
| AC-006 | DAC 输出 | 能够正常输出音频 |
| AC-007 | 播放模式 | 已实现模式运行正确 |
| AC-008 | 显示主题 | 主题切换正常 |
| AC-009 | 亮度控制 | 支持实际设备可实现的亮度档位 |
| AC-010 | 光驱管理 | 弹出前正确停止和释放设备 |
| AC-011 | 断点续播 | 同一张盘能够恢复播放位置 |
| AC-012 | 离线运行 | 无网络仍可正常播放 |
| AC-013 | 诊断模式 | 能够识别主要硬件状态 |
| AC-014 | 稳定性 | 连续播放测试无异常中断 |
| AC-015 | 系统安全 | 不破坏原 Windows 启动及硬件热保护 |

具体测试时长、性能指标和可接受误差应在真实硬件确定后补充。

---

# 第五部分：项目实施阶段

| 阶段 | 工作内容 | 交付物 |
|---|---|---|
| M0 | 需求确认、架构设计 | PRD、TDD |
| M1 | 播放器核心及 Mock 后端 | 可运行的软件原型 |
| M2 | 显示界面及交互 | 复古播放器界面 |
| M3 | Linux 系统集成 | Debian 自动启动系统 |
| M4 | 真实光驱及 DAC 适配 | 真机播放器 |
| M5 | 电源、背光、功耗优化 | 硬件适配版本 |
| M6 | 稳定性及验收 | RetroDisc OS V1.0 |

---

# RetroDisc OS 技术开发文档（TDD）

**文档版本：** V0.1  
**技术路线：** AMD 台式机开发 + Debian 虚拟机验证 + Intel 笔记本部署

# 第一部分：总体技术架构

## 1.1 系统架构

系统分为四层：

1. Linux 系统层：Debian、内核、systemd、设备驱动。
2. 硬件抽象层：光驱、音频、输入、显示、电源。
3. 播放器核心层：播放状态、曲目管理、事件处理。
4. 用户界面层：主界面、设置、帮助、诊断。

架构关系：

```text
                 RetroDisc UI
                      |
                 Player Core
                      |
                 Event Bus
                      |
       +--------------+--------------+
       |              |              |
    CD Module      Audio Module    Input Module
       |              |              |
    libcdio          ALSA          evdev
       |              |              |
    USB CD          USB DAC       Keyboard

       +--------------+--------------+
                      |
                 System Module
                      |
             systemd / ACPI / sysfs
                      |
                 Debian Linux
```

## 1.2 技术选型

| 模块 | 技术 | 选择理由 |
|---|---|---|
| 操作系统 | Debian Stable | 稳定、轻量、适合定制 |
| 开发语言 | C++17 | 适合长期维护的系统程序 |
| 构建系统 | CMake | 跨环境构建 |
| 编译器 | GCC | Linux 原生支持 |
| 图形界面 | SDL2 | 轻量、适合全屏界面 |
| 音频 | ALSA | 直接管理音频设备 |
| CD 访问 | libcdio | 读取光驱及 TOC |
| CD 数字读取 | libcdio-paranoia | 支持数字音轨读取及校验 |
| 输入设备 | evdev | 读取键盘事件 |
| 服务管理 | systemd | 开机自动启动和异常恢复 |
| 硬件监控 | sysfs / ACPI / lm-sensors | 读取硬件状态 |
| 版本管理 | Git | 代码管理及回滚 |
| 单元测试 | CTest | 自动化测试 |

技术选型原则：优先使用成熟 Linux 系统接口，不重复实现操作系统已经提供的功能。

## 1.3 运行模式

播放器支持两种运行模式：

**Mock 模式**

- 不依赖真实光驱。
- 模拟光盘 TOC。
- 使用测试音频数据。
- 使用 ALSA null 或其他虚拟音频设备。
- 模拟键盘事件。
- 模拟硬件状态。

**Real 模式**

- 使用真实 USB 光驱。
- 使用真实 ALSA 音频设备。
- 使用真实键盘事件。
- 访问实际背光、ACPI 和硬件监控接口。

两种模式使用统一接口，禁止将硬件访问逻辑直接写入播放器核心。

---

# 第二部分：模块设计

## 2.1 Player Core

负责：

- 播放状态管理。
- 曲目切换。
- 播放模式。
- 播放进度。
- 断点续播。
- 事件调度。
- 异常恢复。

建议状态：

```text
NO_DISC
    |
DISC_DETECTED
    |
DISC_READY
    |
PLAYING <----> PAUSED
    |
STOPPED
    |
EJECTING
```

状态切换必须经过统一状态管理，避免 UI 和硬件模块各自维护冲突的播放状态。

## 2.2 CD Module

建议接口：

```cpp
class ICDBackend {
public:
    virtual ~ICDBackend() = default;

    virtual bool open() = 0;
    virtual bool detectDisc() = 0;
    virtual DiscInfo readTOC() = 0;
    virtual bool playTrack(int track) = 0;
    virtual bool stop() = 0;
    virtual bool eject() = 0;
};
```

实现：

- RealCDBackend
- MockCDBackend

注意：

CD-DA 不是普通数据文件播放。

应采用真实光驱数字音频读取方式进行验证。不要使用普通 ISO 文件代替 Audio CD。

## 2.3 Audio Module

建议接口：

```cpp
class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    virtual bool open() = 0;
    virtual bool start() = 0;
    virtual bool pause() = 0;
    virtual bool stop() = 0;
    virtual bool setVolume(float volume) = 0;
};
```

实现：

- ALSAAudioBackend
- MockAudioBackend

重点验证：

- 44.1 kHz 音频输出。
- 双声道输出。
- DAC 设备识别。
- 播放期间设备断开处理。
- 音频缓冲和连续播放。
- 软件音量与硬件音量的区别。

注意：应明确采用哪一种 CD 播放路径：

1. 光驱模拟音频输出。
2. 光驱数字音频提取，再通过 ALSA 输出。

建议优先研究数字音频提取方案，以便统一管理 DAC 输出，但必须在目标光驱上验证性能、稳定性和读取兼容性。

## 2.4 Input Module

负责：

- 键盘事件读取。
- 按键映射。
- 长按与短按识别。
- 按键组合。
- 按键重复事件处理。

键位映射放在独立配置文件中。

电源键不应默认通过普通 evdev 键盘事件处理，应单独研究 ACPI 和 logind 事件。

## 2.5 Display Module

负责：

- SDL2 界面。
- 显示主题。
- 播放信息。
- 亮度设置。
- 频谱显示。
- 显示关闭与唤醒。

硬件背光通过独立 BacklightManager 实现。

UI 亮度和硬件背光必须分离。

## 2.6 System Module

负责：

- 系统状态读取。
- 温度监控。
- ACPI 事件。
- 背光接口。
- 日志管理。
- 系统版本。
- 服务状态。

禁止主动修改 EC 风扇参数。

风扇仅监测，不控制。不得关闭 BIOS / EC 的硬件热保护。

---

# 第三部分：工程目录

```text
RetroDisc/
│
├── AGENTS.md
├── CMakeLists.txt
├── README.md
│
├── src/
│   ├── core/
│   │   ├── player.cpp
│   │   ├── player_state.cpp
│   │   └── event_bus.cpp
│   │
│   ├── cd/
│   │   ├── cd_interface.cpp
│   │   ├── real_cd.cpp
│   │   └── mock_cd.cpp
│   │
│   ├── audio/
│   │   ├── audio_interface.cpp
│   │   ├── alsa_backend.cpp
│   │   └── mock_audio.cpp
│   │
│   ├── input/
│   │   ├── keyboard.cpp
│   │   └── key_mapping.cpp
│   │
│   ├── display/
│   │   ├── display.cpp
│   │   ├── brightness.cpp
│   │   └── themes/
│   │
│   ├── system/
│   │   ├── power_manager.cpp
│   │   ├── hardware_monitor.cpp
│   │   └── log_manager.cpp
│   │
│   └── ui/
│       ├── main_screen.cpp
│       ├── settings.cpp
│       ├── help.cpp
│       └── service_mode.cpp
│
├── config/
│   ├── player.conf
│   ├── keys.conf
│   └── display.conf
│
├── tests/
│   ├── test_player.cpp
│   ├── test_cd.cpp
│   ├── test_audio.cpp
│   ├── test_keyboard.cpp
│   └── test_resume.cpp
│
├── scripts/
│   ├── build.sh
│   ├── test.sh
│   └── deploy.sh
│
├── system/
│   └── retrodisc.service
│
└── docs/
    ├── architecture.md
    ├── user_manual.md
    ├── hardware_test.md
    └── development_plan.md
```

---

# 第四部分：开发环境配置

## 4.1 AMD 台式机

当前已有 Windows + Ubuntu 双系统。

开发环境直接使用 Ubuntu 原生系统，不需要重新安装操作系统。

安装：

```bash
sudo apt update

sudo apt install -y \
  build-essential \
  cmake \
  git \
  gdb \
  pkg-config \
  libsdl2-dev \
  libasound2-dev \
  libcdio-dev \
  libcdio-paranoia-dev \
  libudev-dev \
  libevdev-dev \
  libsystemd-dev
```

安装 Codex 并配置 VS Code 开发环境。

项目工作目录：

```text
~/Projects/RetroDisc/
```

代码和构建文件放在 Ubuntu 的 Linux 文件系统中。

## 4.2 Debian 虚拟机

用途：

- 测试 Debian 启动。
- 测试 systemd 服务。
- 测试播放器部署。
- 测试 SDL2 界面。
- 测试 Mock 硬件后端。

建议配置：

| 项目 | 配置 |
|---|---|
| CPU | 2～4 核 |
| 内存 | 4 GB |
| 磁盘 | 32 GB 动态分配 |
| 网络 | NAT |
| 系统 | Debian Stable 64 位 |
| 桌面 | Openbox + Xorg（仅测试界面） |

## 4.3 Intel 笔记本

作为最终目标设备。

要求：

- Debian Stable。
- x86-64 通用编译目标。
- 保留 Windows 原有分区。
- 使用现有 EFI 分区，不格式化。
- Debian 初期预留约 20 GB。
- 使用 SSH 或 Git 部署。
- 真实验证光驱、DAC、背光、ACPI 和热管理。

不得在未确认分区结构和备份情况前执行安装或磁盘修改。

---

# 第五部分：编译与调试流程

## 5.1 基础编译

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

## 5.2 单元测试

```bash
ctest --test-dir build --output-on-failure
```

## 5.3 GDB 调试

```bash
gdb ./build/retrodisc
```

建议 Debug 构建开启：

- 调试符号。
- AddressSanitizer。
- UndefinedBehaviorSanitizer。

## 5.4 Git 管理

每个可用功能完成后创建独立提交。

提交内容应包含：

- 修改内容。
- 影响模块。
- 测试结果。
- 未解决问题。

Codex 修改后，应执行编译、测试和代码审查。

## 5.5 Codex 工作规则

Codex 应遵循：

1. 先读取 AGENTS.md 和项目文档。
2. 修改前先理解模块结构。
3. 不一次性生成整个工程。
4. 每次只完成一个明确任务。
5. 每次修改后编译。
6. 每次修改后运行相关测试。
7. 不允许随意修改其他模块。
8. 不允许自动执行磁盘分区和 EFI 修改。
9. 不允许擅自修改内核和风扇控制策略。
10. 修改系统服务、电源管理及启动配置前，先说明风险和回滚方案。

---

# 第六部分：测试计划

## 6.1 软件测试

| 测试项目 | 测试方式 |
|---|---|
| 播放状态机 | 单元测试 |
| 曲目切换 | Mock CD |
| 光盘识别 | Mock CD |
| 播放模式 | 单元测试 |
| 断点续播 | 模拟 TOC |
| 按键映射 | 模拟输入事件 |
| 显示主题 | SDL2 界面测试 |
| 配置文件 | 加载和异常输入测试 |
| 异常恢复 | 模拟设备断开 |
| systemd | Debian VM |

## 6.2 真实硬件测试

| 测试项目 | 测试设备 |
|---|---|
| CD 插入识别 | USB 外置光驱 |
| CD-DA 播放 | 实体音频 CD |
| 曲目切换 | 实体音频 CD |
| DAC 输出 | USB DAC |
| 光驱弹出 | USB 外置光驱 |
| 键盘控制 | 笔记本内置键盘 |
| 屏幕亮度 | 笔记本内置屏幕 |
| 电源键 | 笔记本 ACPI |
| 温度 | Intel CPU |
| 风扇 | 笔记本 EC / BIOS |
| 功耗 | 外接功率计 |

## 6.3 测试阶段

M1：Mock 后端测试。

M2：Debian 虚拟机运行测试。

M3：笔记本真实光驱和 DAC 测试。

M4：背光、ACPI、电源管理测试。

M5：连续播放和功耗测试。

M6：最终版本验收。

---

# 第七部分：部署与系统集成

## 7.1 部署原则

采用先软件、后系统集成的方式。

不在开发初期直接制作定制 ISO，也不直接裁剪 Linux 内核。

第一阶段先在标准 Debian 上安装播放器并验证。

## 7.2 systemd 服务

服务目标：

- 系统启动后自动运行播放器。
- 播放器崩溃后自动重启。
- 正常关机时安全保存状态。
- 播放器退出时释放光驱和音频设备。

示例服务文件：

```ini
[Unit]
Description=RetroDisc CD Player
After=systemd-user-sessions.service sound.target
Wants=sound.target

[Service]
Type=simple
User=retrodisc
ExecStart=/opt/retrodisc/bin/retrodisc
Restart=on-failure
RestartSec=3

[Install]
WantedBy=multi-user.target
```

说明：这是初始服务模板。正式部署时还需要配置 SDL2 显示会话、设备访问权限、光驱访问权限和安全策略。不能直接假定普通 systemd system service 能自动获得图形显示环境。

## 7.3 安装方式

第一阶段：

标准 Debian 安装 + RetroDisc Player 软件部署。

第二阶段：

制作 RetroDisc OS 安装介质或安装脚本。

第三阶段：

形成可恢复、可重复部署的专用系统镜像。

## 7.4 系统恢复

必须提供：

- Windows 启动恢复方案。
- Debian 恢复介质。
- RetroDisc 程序回滚。
- 系统日志导出。
- 关键配置备份。
- 已知可用的系统版本。

---

# 第八部分：性能与功耗优化

优化顺序：

1. 禁用不需要的后台服务。
2. 关闭不需要的无线设备。
3. 限制日志写入。
4. 优化 CPU 调频和空闲状态。
5. 优化 USB 电源管理。
6. 优化屏幕背光。
7. 测量不同运行状态下的整机功耗。
8. 根据实测结果决定是否需要进一步系统精简。

USB autosuspend 不得导致光驱或 DAC 在使用期间掉线。

风扇管理仍由 BIOS / EC 负责，系统仅监测状态，不强制关闭风扇。

---

# 第九部分：交付物

项目最终需要交付：

| 编号 | 交付物 |
|---|---|
| DEL-001 | 产品需求文档 PRD |
| DEL-002 | 技术开发文档 TDD |
| DEL-003 | 完整 C++ 源代码 |
| DEL-004 | CMake 构建工程 |
| DEL-005 | Mock 硬件测试模块 |
| DEL-006 | Debian 安装与部署脚本 |
| DEL-007 | systemd 服务配置 |
| DEL-008 | RetroDisc OS 系统镜像或安装介质 |
| DEL-009 | 用户使用说明书 |
| DEL-010 | 硬件调试说明书 |
| DEL-011 | 软件测试报告 |
| DEL-012 | 最终验收报告 |

---

# 第十部分：待确认事项

以下事项在进入正式开发前需要确定：

1. Intel 笔记本具体型号、内存、SSD 容量。
2. 外置 USB 光驱型号及 CD-DA 读取能力。
3. USB DAC 型号及支持的采样率。
4. 是否要求严格 Bit-Perfect 输出。
5. 是否需要系统休眠，还是只支持息屏。
6. 是否需要鼠标操作，还是完全键盘控制。
7. 播放器是否需要保存播放历史。
8. 最终是否制作独立可安装 ISO。

**技术决策原则：先保证 CD 读取和音频输出可靠，再实现复古 UI，最后进行系统精简和功耗优化。**

文档版本：V0.1  
项目：RetroDisc OS
