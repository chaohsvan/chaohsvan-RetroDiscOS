# 架构决策记录（ADR）

记录已确认的关键决策。**推翻任何一条都要先改本文件**，不要让 agent 在代码里悄悄改掉。

---

## D1 · CD 音频进入系统的路径

**决策：数字抓轨 → ALSA**（2026-11-09 确认）

- 用 `libcdio` 读 TOC，用 `libcdio-paranoia` 把音频扇区读到内存，再写入 ALSA。
- 不使用光驱内部的模拟解码输出。

**理由**：能统一走 DAC；能拿到 PCM 做频谱显示（F9）；能做精确 seek（F11/F12）和断点续播（PLAY-008）。模拟输出路径这些全部做不到。

**代价**：读取兼容性风险最高，必须在真实光驱上验证。见 `hardware_test.md`。

**对代码的约束**：
- `ICDBackend` 必须暴露"读第 N 帧 PCM"的能力，而不只是 `playTrack()`。
- 播放缓冲区归 `Audio Module` 管，`Player Core` 只发命令。

---

## D2 · 不要求 Bit-Perfect

**决策：先不做 Bit-Perfect，音量走 ALSA `softvol`**（2026-11-09 确认）

**理由**：44.1 kHz 可能被重采样，音质非最优，但实现简单、兼容任何声卡。Bit-Perfect 列为 P1，等 M4 之后再评估。

**对代码的约束**：
- `IAudioBackend::setVolume()` 明确是**软件音量**语义，注释里写清楚。
- 不要为了"可能 Bit-Perfect"提前设计 hw mixer 抽象，避免过度设计。
- 文档和 UI 里不得出现 "Bit-Perfect" 字样，避免误导。

---

## D3 · 纯键盘输入

**决策：只做 evdev 键盘，不做鼠标**（2026-11-09 确认）

**理由**：产品定位是复古 CD 机，鼠标在专用设备上无意义；省掉 UI 的命中检测和一整套指针逻辑。

**对代码的约束**：
- `Input Module` 只产出语义事件（PLAY_PAUSE / NEXT / PREV / VOL_UP …），UI 不做坐标判断。
- `keys.conf` 必须是唯一键位来源，代码里不硬编码键码。

---

## D4 · 当前硬件

**状态：有 USB 光驱，无 DAC，暂时无音频 CD 在手**（2026-11-09 确认）

已探测到的设备：

| 项 | 实测值 |
|---|---|
| 光驱 | `HL-DT-ST DVDRAM GP78N`，挂在 `/dev/sr0` |
| 介质 | 空（`blockdev` 报 "找不到介质"） |
| 音频设备 | 本机无 `/dev/snd`（SSH/容器环境），真机播放待定 |

**影响**：
- M1/M2 用 Mock 后端，不受影响。
- Step 4 的真机 spike 需要：一张正版音频 CD + 安装 `cdparanoia` CLI。
- 没有 DAC 时，先用板载声卡代替，`IAudioBackend` 的设备名走配置，不写死。
