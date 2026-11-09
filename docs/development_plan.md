# RetroDisc OS 开发计划

> 版本：v1.0 · 2026-11-09
> 依据：`README.md`（PRD V0.1 + TDD V0.1）、`docs/decisions.md`（D1–D4）
> 状态标记：✅ 已完成 · 🔄 进行中 · ⬜ 未开始

---

## 0. 总体路线

| 里程碑 | 内容 | 依赖 | 状态 |
|---|---|---|---|
| M0 | 需求确认、架构设计、工程规则 | — | ✅ |
| M1 | 播放器核心 + Mock 后端（可跑通全流程） | M0 | ⬜ |
| M2 | 显示界面与交互（主题、亮度、HELP、SERVICE） | M1 | ⬜ |
| M3 | Debian 系统集成（systemd、自启动、权限） | M1 | ⬜ |
| M4 | 真机光驱与 DAC 适配 | M1、真机 | ⬜ |
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
| T25 | `ALSAAudioBackend` | 有声卡/DAC |
| T26 | 设备热插拔与断连处理 | 拔 DAC 不崩溃，显示异常状态 |
| T27 | 精确 seek（F11/F12） | 定位误差 < 1 秒 |
| T28 | 光盘更换检测 | 换盘后 TOC 更新，续播不误触发 |

**如果 T23 失败**（`cdparanoia` 读不出或大量爆音）：立刻停下来讨论，不要硬写 `RealCDBackend`。备选是换光驱，或退回模拟输出方案（需重开 D1）。

## 5. M5：电源、背光、功耗

| 卡 | 内容 | 验收 |
|---|---|---|
| T29 | 背光 sysfs 控制 | 5 档实测有效，0% 时键盘可唤醒 |
| T30 | ACPI 电源键 | 短按息屏/唤醒，长按仍由固件处理 |
| T31 | cpufreq / cpuidle | 播放时 CPU 占用 < 15% |
| T32 | journald 限额 + tmpfs | 24 小时日志增量 < 10 MB |
| T33 | USB autosuspend 白名单 | 光驱/DAC 使用期间不掉线 |
| T34 | 功耗实测 | 记录待机/播放/满亮度三档的整机瓦数 |

## 6. M6：稳定性与验收

| 卡 | 内容 | 验收 |
|---|---|---|
| T35 | 24 小时连续播放 | 无中断、无内存增长（ASan + RSS 采样） |
| T36 | 异常注入 | 拔盘/拔 DAC/杀进程/断电恢复均不损坏数据 |
| T37 | 验收表核对 | PRD 第四部分 AC-001 ~ AC-015 逐条打勾或写明原因 |
| T38 | 用户手册 + 硬件调试手册 | 对应 DEL-009 / DEL-010 |

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
