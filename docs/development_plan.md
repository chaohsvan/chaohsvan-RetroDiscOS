# RetroDisc OS 开发计划

> 版本：v1.0 · 2026-11-09
> 依据：`README.md`（PRD V0.1 + TDD V0.1）、`docs/decisions.md`（D1–D4）
> 状态标记：✅ 已完成 · 🔄 进行中 · ⬜ 未开始

---

## 0. 总体路线

| 里程碑 | 内容 | 依赖 | 状态 |
|---|---|---|---|
| M0 | 需求确认、架构设计、工程规则 | — | ✅ |
| M1 | 播放器核心 + Mock 后端（可跑通全流程） | M0 | ✅ |
| M2 | 显示界面与交互（主题、亮度、HELP、SERVICE） | M1 | ✅ |
| M3 | Debian 系统集成（systemd、自启动、权限） | M1 | ✅ |
| M4 | 真机光驱与 DAC 适配 | M1、真机 | 🔄 待音频 CD |
| M5 | 电源、背光、功耗优化 | M3、M4 | ⬜ |
| M6 | 稳定性与验收 | M2–M5 | ⬜ |

**M1 是唯一可以在当前机器上完整交付的里程碑**：不需要光驱、不需要声卡、不需要 Debian。
M3 之后全部需要真机或虚拟机。

---

## 1. M1：播放器核心 + Mock 后端

### 1.0 验收总纲

一行命令跑通全流程，且 `ctest` 全绿：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
SDL_VIDEODRIVER=dummy ./build/retrodisc --mock --auto-disc
```

最后一条必须在**无光驱、无声卡、无显示器**的机器上输出完整日志：

```
插入光盘 → 读 TOC（12 轨） → 播放第 1 轨 → 切到第 2 轨 → 暂停 → 恢复
→ 停止 → 弹盘 → 等待放盘
```

### 1.1 任务卡依赖图

```
T1 骨架
 ├── T2 状态机 + 事件总线
 │    └── T6 SDL2 主界面
 ├── T3 Mock CD ──┐
 ├── T4 Mock 音频 ┴── T7 集成（--mock 全流程）
 └── T5 配置加载
```

T1 是硬前置。T2/T3/T4/T5 可并行。T6 依赖 T2。T7 汇总。

---

### T1 · 构建骨架

**范围**：`CMakeLists.txt`、`cmake/`、`.clang-format`、`scripts/build.sh`、`scripts/test.sh`、`src/main.cpp`（空壳）、`tests/CMakeLists.txt`

**产出**：
- 三个构建目标：`retrodisc_core`（静态库）、`retrodisc`（可执行）、`retrodisc_tests`
- Debug 默认 `-fsanitize=address,undefined -Wall -Wextra -Werror`
- 一个 `smoke_test`（断言 `1+1==2`），证明 CTest 链路通

**验收**：
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure   # 1/1 passed
```

**注意**：CMake 4.2.3 已取消 `<3.5` 兼容，`cmake_minimum_required` 写 `3.16` 即可，别抄老教程的 `2.8`。

---

### T2 · Player Core（状态机 + 事件总线）

**范围**：`src/core/`、`tests/test_player_state.cpp`、`tests/test_event_bus.cpp`

**状态集**（PRD 2.1 + D4 约束）：
```
NO_DISC → DISC_DETECTED → DISC_READY → PLAYING ⇄ PAUSED → STOPPED → EJECTING → NO_DISC
```

**接口要点**：
- `bool transition(Event)`：非法迁移返回 `false` 并记日志，**不 assert、不抛异常**
- `EventBus`：同步派发，订阅者按注册顺序收到；派发过程中增删订阅者不崩溃
- 状态是**单一事实来源**，UI 与硬件模块只读

**验收**：
- 每条合法迁移有测试；每条非法迁移（如 `NO_DISC` 直接到 `PLAYING`）有测试
- 覆盖 PRD 3.1「播放状态和光驱状态应保持一致」：拔盘时状态落回 `NO_DISC`

---

### T3 · Mock CD 后端

**范围**：`src/cd/`、`tests/test_mock_cd.cpp`

**接口**（在 `cd_interface.h`，Mock 与 Real 共用）：
```cpp
class ICDBackend {
public:
  virtual ~ICDBackend() = default;
  virtual bool open() = 0;
  virtual bool detectDisc() = 0;
  virtual DiscInfo readTOC() = 0;          // 轨数、各轨起止 LBA、总时长、CD-Text
  virtual int  readFrames(int track, int lba, int count, uint8_t* out) = 0;  // 44.1k/16bit/2ch
  virtual bool eject() = 0;                // 只有 eject，没有 load（D4）
  virtual void close() = 0;
};
```

**Mock 行为**：
- 假 TOC：12 轨，前 11 轨 3:45，末轨 5:10；部分轨带 CD-Text，部分不带（验证 `TRACK 01` 回退）
- 合成的 1 kHz 正弦波 PCM，便于断言
- 可注入故障：换盘、读失败、无盘、CD-Text 缺失

**验收**：插盘 / 读 TOC / 换盘 / 读失败 / 弹盘五条路径各有测试。

---

### T4 · Mock 音频后端

**范围**：`src/audio/`、`tests/test_mock_audio.cpp`

**接口**：
```cpp
class IAudioBackend {
public:
  virtual ~IAudioBackend() = default;
  virtual bool open(const std::string& device) = 0;
  virtual bool start() = 0;
  virtual bool pause() = 0;
  virtual bool stop() = 0;
  virtual bool setVolume(float v) = 0;   // 软件音量（D2），注释写清
  virtual int  write(const int16_t* frames, int frameCount) = 0;
};
```

**Mock 行为**：写 WAV 文件（便于人工试听）或 `/dev/null` 接收器（便于 CI）；按真实时钟节流。

**验收**：
- 44.1 kHz / 16 bit / 2 ch 断言
- 播放 60 秒内容，实测耗时误差 < 1%
- 中途 `stop()` 不泄漏、不崩溃

---

### T5 · 配置加载

**范围**：`src/core/config.cpp`、`config/player.conf`、`config/keys.conf`、`tests/test_config.cpp`

**格式**：`key = value`，`#` 注释，未知键**告警但不失败**。

**必须项**：音频设备名、默认音量、默认主题、亮度档位、键位映射表、续播文件路径。

**验收**：缺字段走默认值；坏输入（数字越界、键位重复）不崩溃且有明确错误信息。

---

### T6 · SDL2 主界面（先只做一套主题）

**范围**：`src/display/`、`src/ui/main_screen.cpp`、`tests/test_render_headless.cpp`

**内容**：全屏窗口；曲目号 / 曲目名 / 总轨数 / 当前时间 / 总时长 / 播放状态 / 音量 / 光驱状态。

**硬性要求**：必须支持 `SDL_VIDEODRIVER=dummy` 无头运行，否则 CI 和我的验证都做不了。

**验收**：
- 无头模式下渲染 100 帧不崩溃，帧内容可用 `SDL_RenderReadPixels` 抽样断言
- 按键事件能翻译成语义事件（PLAY_PAUSE / NEXT / PREV / VOL_UP…）

---

### T7 · 集成（M1 收尾）

**范围**：`src/main.cpp`、`tests/test_integration.cpp`

**内容**：装配 Mock CD + Mock Audio + 状态机 + UI，跑完整流程；实现 `--mock` / `--auto-disc` 参数。

**验收**：1.0 节那条命令的完整输出 + `ctest` 全绿。

---

## 2. M2：显示与交互

| 卡 | 内容 | 验收 |
|---|---|---|
| T8 | 主题框架 + Digital Hi-Fi 主题 | 主题可运行时切换（F5），配置持久化 |
| T9 | Classic VFD / Amber / Vintage Analog | 三套主题各出一张截图供人评审 |
| T10 | Display Off + 唤醒 | 关闭显示后按键能唤醒，播放不中断 |
| T11 | 亮度分离：UI 亮度 5 档 / 硬件背光 | 两个通道独立可调；背光设备不存在时降级为仅 UI |
| T12 | HELP 离线帮助（10 个条目，PRD 2.7） | 可滚动、可返回，全键盘操作 |
| T13 | SERVICE MODE 诊断页（PRD 2.8） | 十个诊断项全部有数据或明确的「不可用」 |
| T14 | 播放模式（循环/随机/顺序/单曲） | 每种模式有状态机测试 |
| T15 | 断点续播 | 换盘后不误恢复；同盘恢复误差 < 1 秒 |
| T16 | 频谱显示（F9） | 从 PCM 取样，Mock 模式下有稳定波形 |

## 3. M3：Debian 系统集成

| 卡 | 内容 | 验收 |
|---|---|---|
| T17 | Debian VM 环境搭建 | 能 ssh、能跑 SDL2、能识别 USB 光驱 |
| T18 | systemd 服务 + 崩溃重启 | `systemctl kill` 后 3 秒内自愈 |
| T19 | 图形会话（autologin + startx） | 开机直接进播放器，不经过桌面 |
| T20 | 权限（audio/video/cdrom 组） | 普通用户可读 `/dev/sr0`、`/dev/snd/*` |
| T21 | 部署脚本 `deploy.sh` | 一条命令把构建产物装到 `/opt/retrodisc` |
| T22 | 回滚脚本 | 能退到上一个版本 |

⚠️ 硬规则：**M3 全程只在虚拟机里做，不碰宿主机分区和 EFI。**

## 4. M4：真机 CD 与 DAC

| 卡 | 内容 | 前提 |
|---|---|---|
| T23 | `cdparanoia` 可行性 spike | 一张正版音频 CD |
| T24 | `RealCDBackend`（libcdio + paranoia） | T23 通过 |
| T25 | `ALSAAudioBackend` | ✅ 已完成（设备可用时） |
| T26 | 设备热插拔与断连处理 | 拔 DAC 不崩溃，显示异常状态 |
| T27 | 精确 seek（F11/F12） | 定位误差 < 1 秒 |
| T28 | 光盘更换检测 | 换盘后 TOC 更新，续播不误触发 |

**如果 T23 失败**（`cdparanoia` 读不出或大量爆音）：立刻停下来讨论，不要硬写 `RealCDBackend`。备选是换光驱，或退回模拟输出方案（需重开 D1）。

## 5. M5：电源、背光、功耗

| 卡 | 内容 | 验收 |
|---|---|---|
| T29 | 背光 sysfs 控制 | ✅ 真机 5 档实测通过，0% 按键唤醒已实现 |
| T30 | ACPI 电源键 | ✅ 真机短按 4 次全部生效，含背光联动 |
| T31 | cpufreq / cpuidle | 🔄 Mock 播放实测 1.3%，待真 CD 复测 |
| T32 | journald 限额 + tmpfs | ✅ 配置已就位（待 24h 实测） |
| T33 | USB autosuspend 白名单 | ✅ 规则已就位（待真机验证） |
| T34 | 功耗实测 | 记录待机/播放/满亮度三档的整机瓦数 |

## 6. M6：稳定性与验收

| 卡 | 内容 | 验收 |
|---|---|---|
| T35 | 24 小时连续播放 | 无中断、无内存增长（ASan + RSS 采样） |
| T36 | 异常注入 | 拔盘/拔 DAC/杀进程/断电恢复均不损坏数据 |
| T37 | 验收表核对 | PRD 第四部分 AC-001 ~ AC-015 逐条打勾或写明原因 |
| T38 | 用户手册 + 硬件调试手册 | 🔄 用户手册完成（DEL-009），硬件手册待写 |

---

## 7. 工作量与节奏

| 阶段 | 任务卡数 | 相对规模 | 关键风险 |
|---|---|---|---|
| M1 | 7 | 中 | 无（纯软件，可完整验证） |
| M2 | 9 | 大 | 主题工作量容易被低估 |
| M3 | 6 | 中 | systemd 图形会话 |
| M4 | 6 | 大 | **CD-DA 读取兼容性** |
| M5 | 6 | 中 | 只能真机验证 |
| M6 | 4 | 中 | 依赖前面全部完成 |

节奏建议：**M1 一口气做完再开 M2**。M2 的主题系统一旦和状态机耦合，后面所有 UI 改动都会变贵。

## 8. 并行策略（给 agent 编排用）

可以并行的组合（每个 agent 一个 worktree、一张卡）：

- 波次 1：T1（独占，阻塞所有人）
- 波次 2：T2 ∥ T3 ∥ T4 ∥ T5
- 波次 3：T6 ∥ T7 前半（T7 等 T2/T3/T4 齐）
- 波次 4：M2 各主题卡可 3 路并行（T9 的三套主题互不依赖）

**不可并行**：T1 之后才能开波次 2；T7 必须等波次 2 全部合并。

## 9. 完成定义（Definition of Done）

一张卡算完成，必须同时满足：

1. `cmake --build` 通过，无警告（`-Werror`）
2. `ctest` 全绿，且新增行为有对应测试
3. `clang-format` 已跑过
4. 提交信息含「验证」与「未解决」两行
5. 没有跨模块越界修改（见 `AGENTS.md` 第 4 节）

---

## 10. 进度记录

### 2026-11-09 · M1 完成

| 卡 | 结果 |
|---|---|
| T1 构建骨架 | ✅ 三目标 + ASan/UBSan + `-Werror` + scripts |
| T2 状态机与事件总线 | ✅ 七态、21 条合法迁移、派发期增删安全 |
| T3 Mock CD | ✅ 12 轨假 TOC、1kHz PCM、故障注入 |
| T4 Mock 音频 | ✅ null/WAV 两种 sink、实时节流、软件音量 |
| T5 配置 | ✅ player.conf / keys.conf，坏输入只告警 |
| T6 界面与按键 | ✅ 自绘 5x7 字体、无头渲染、22 个键位动作 |
| T7 集成 | ✅ `--mock --auto-disc` 全流程 |

验证命令与结果：

```bash
$ ./scripts/test.sh
100% tests passed, 0 tests failed out of 9

$ SDL_VIDEODRIVER=dummy ./build/retrodisc --mock --auto-disc
[toc] 12 tracks, 46:25 total, disc MOCK-DISC-0001
[state] PLAYING
[play] track 01  00:02 / 03:45  vol 80%
[state] STOPPED / EJECTING / NO_DISC
[done] played 2.5 s of audio, final state NO_DISC
```

界面截图：`docs/images/m1-ui.png`（800x480，dummy 视频驱动下抓帧）。

已知遗留（不阻塞 M1）：

1. `--seconds N` 会因为按 0.5 s 分块而最多多播 0.5 s；
2. 主界面中段留白较多，等 M2 主题系统再排版；
3. 进度条背景色偏亮，M2 调整配色。

### 2026-11-09 · M2 完成

| 卡 | 结果 |
|---|---|
| T8 主题框架 | ✅ 5 套主题、运行时轮转、持久化到 state.conf |
| T9 主题视觉 | ✅ 四套配色 + off，截图 docs/images/m2-themes.png |
| T10 Display Off | ✅ F8 息屏，任意键唤醒且不误触 |
| T11 亮度双通道 | ✅ UI 5 档 + sysfs 背光，缺失设备自动降级 |
| T12 HELP | ✅ 10 个条目、滚动、与 SERVICE 互斥 |
| T13 SERVICE MODE | ✅ 10 项诊断，缺设备显示 N/A |
| T14 播放模式 | ✅ 顺序 / 整碟循环 / 单曲循环 / 随机 |
| T15 断点续播 | ✅ 按 discId 匹配，位置扇区对齐 |
| T16 频谱 | ✅ 16 段 Goertzel，无 FFT 依赖 |

验证：`ctest 16/16`，clean rebuild 通过。

已知限制：

1. 点阵字体仅覆盖 ASCII，界面与帮助文本为英文；中文需要扩字库或引入 FreeType（后者违反当前依赖约束，需人工决策）。
2. 频谱为 16 段 Goertzel，不是 FFT，频率分辨率较粗。
3. 背光在台式机上不可用，只验证了降级路径；真机验证留给 M5。

### 2026-11-09 · M3 进行中（T18–T22 完成，T17 待验证）

| 卡 | 结果 |
|---|---|
| T18 systemd 服务 | ✅ 专用 tty1、Restart=on-failure + RestartSec=3、加固项齐全 |
| T19 图形会话 | ✅ 有 X/Wayland 用之，否则 SDL kmsdrm；异常退出码 78 防重启风暴 |
| T20 权限 | ✅ check_permissions.sh 检查 /dev/sr0 与 /dev/snd，给出修复命令 |
| T21 部署 | ✅ deploy.sh 装二进制/配置/单元，配置不覆盖，保留 previous |
| T22 回滚 | ✅ rollback.sh 交换 current/previous，无备份时明确失败 |
| T17 Debian VM | ✅ 已用 qemu + Debian 13 cloud image 验证，见 docs/vm-test.md |

验证方式说明：T18–T22 全部通过**不依赖虚拟机**的测试（`ctest` 19/19），
包括 `systemd-analyze verify`、脚本退出码、参数透传、临时前缀下的真实部署与回滚。

T17 未完成的原因：本机没有 qemu/virt-install/VirtualBox。可行方案是
qemu + Debian cloud image + cloud-init（`/dev/kvm` 可用），但需要在 VM 内
重新编译（Ubuntu 26.04 的 glibc 比 Debian 13 新，二进制不可直接复用）。

### 2026-11-09 · M3 完成（T17 已验证）

qemu + Debian 13 (trixie) cloud image 实测结果：

| 验证项 | 结果 |
|---|---|
| VM 内 ctest | 19/19（源码在 Debian 上重新编译） |
| 开机自启 | 重启后 `enabled` + `active`，无需人工干预 |
| 崩溃自愈 | `kill -9` 后 6 秒内换 PID 重新播放 |
| 日志 | `[toc]`/`[state]`/`[play]` 全部进 journal |

过程中发现并修复两个只有真机才暴露的问题（tty DeviceAllow、stdout 块缓冲），
详见 `docs/vm-test.md`。

### 2026-11-09 · M4 提前完成 T25（ALSA 后端）

M4 的 T23/T24 必须等音频 CD，但 T25 与光驱无关，已提前完成：

- `ALSAAudioBackend` 实现 `IAudioBackend`，直接对接 alsa-lib
- 44.1 kHz / 16 bit / 立体声；`snd_pcm_set_params` 允许 ALSA 重采样（D2：不追 Bit-Perfect）
- 欠载 (`-EPIPE`) 自动 prepare 重试；系统挂起 (`-ESTRPIPE`) 先 resume 再降级 prepare
- 音量是软件增益，与 Mock 语义一致
- 验证：`ctest` 22/22；ALSA `null` 插件跑全部用例；台式机 `default` 设备实测写入 4410 帧成功

RealCDBackend（T24）仍然**卡在 T23 之前不动**——这是计划里写死的闸门。

### 2026-11-09 · T29 真机背光验证（笔记本 ASUS FX505GT）

| 项 | 结果 |
|---|---|
| 设备 | `/sys/class/backlight/intel_backlight`，max 120000 |
| 权限 | 原为 `root:root 0644`（无法写）→ udev 规则改为 `root:video 0664` |
| 五档 | 100/75/50/25/0 全部按预期写入并回读一致 |
| 恢复 | 按原始 raw 值恢复（37499 → 37499），不是恢复到最近档位 |
| 0% 唤醒 | `BrightnessController::wakeFromBlank()`，任意键回到 25% 且不触发原动作 |

发现并修复的问题：最初恢复逻辑只在 5 个档位里找匹配，原亮度 31% 匹配不到就退回 100%，
把用户屏幕留在了最亮。改为保存/恢复 raw 值。

### 2026-11-10 · T30 电源键（代码完成，待物理按键验证）

- `PowerKeyState`：短按/长按判定，纯状态机，可用合成时间戳测试
  - 短按（< 1500 ms）在松开时上报 ShortPress
  - 长按在按住期间越过阈值即上报 LongPress 一次，松手不再重复
  - 内核自动重复不会重置计时
- `PowerKey`：evdev 直接读 /dev/input，`findPowerKeyDevice()` 按 KEY_POWER 能力自动发现
- `system/logind-retrodisc.conf`：`HandlePowerKey=ignore`，否则 logind 会先关机
  - 长期按住仍由 EC/固件强制断电，这一点不受影响
- 笔记本实测：自动发现 `/dev/input/event7 (Power Button)`；drop-in 已装，
  `systemd-analyze cat-config` 显示 `HandlePowerKey=ignore`

**已实测（2026-11-09，重启后）**：

```
[power] /dev/input/event7 (Power Button)
[power] short press -> display off  backlight 0%
[power] short press -> display on  backlight 31%
[power] short press -> display off  backlight 0%
[power] short press -> display on  backlight 31%
```

连按四次全部正确交替；结束后 `sysfs brightness = 37499`（= 原值 31%），
说明息屏会真正关掉背光、唤醒会精确恢复，而不是只把画面涂黑。

`busctl get-property ... HandlePowerKey` 返回 `"ignore"`，确认 logind 已放权。

### 2026-11-10 · T31 CPU 占用初测

笔记本（i7-9750H，`powersave` 调速器）上 Mock 播放 10 秒采样：

| 项 | 结果 |
|---|---|
| CPU 占用 | **1.3%**（目标 < 15%） |
| 运行核数 | 单线程 |
| 说明 | Mock 播放不含真实 CD 抓轨开销，真 CD 上线后需复测 |

