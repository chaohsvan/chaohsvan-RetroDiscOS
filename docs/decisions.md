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

## D4 · 当前硬件与目标机

**状态：目标机已就绪（Debian 13），USB 光驱已接上，仍无音频 CD**（2026-11-10 更新）

### 目标机（最终设备）

| 项 | 实测值 |
|---|---|
| 机型 | `ASUSTeK COMPUTER INC. TUF Gaming FX505GT_FX95GT` |
| CPU | Intel Core i7-9750H（6 核 12 线程，`intel_pstate`，`powersave`） |
| 内存 / 磁盘 | 15.5 GiB / 根分区 18 GiB |
| 系统 | Debian GNU/Linux 13 (trixie)，内核 6.12.116 |
| 显示 | `intel_backlight`（max 120000），`/dev/dri/card0..1`，kmsdrm 可用 |
| 音频 | `sof-hda-dsp`（Sound Open Firmware）+ NVidia HDMI |
| 风扇 | hwmon5 双风扇（实测 2700/2800 RPM），**只监测不控制** |
| 电源 | `AC0` 在线 + `BAT0` |
| 电源键 | `/dev/input/event7`（`Power Button`，KEY_POWER） |

### 光驱

| 项 | 实测值 |
|---|---|
| 型号 | `HL-DT-ST DVDRAM GP78N`，固件 `1.01`，USB 外置，`/dev/sr0` |
| 介质 | 空（无盘时 `cdparanoia` 报 "Unable to open disc"） |

> 注意：空的 slim 光驱会报告一个假容量（2097151 扇区），**不能用容量判断是否有盘**，
> 必须看 `cdparanoia` 的错误类型。

**光驱能力**（`cd-drive` 实测）：

| 能力 | 结果 | 意义 |
|---|---|---|
| C2 Error pointers | ✅ 支持 | 抓轨能拿到差错位置，可实现校验重读，对 D1 有利 |
| CD-Text | ✅ 支持 | CD-003（CD-Text 曲目名）有硬件基础 |
| 加载方式 | Tray（托盘） | 非吸入式 |
| 托盘收回 | ❌ 固件不支持 | 见下 |

**托盘约束（重要，属于产品行为而非 bug）**：

- 软件可以**弹出**托盘（`eject` / `CDROMEJECT` 可用）。
- 软件**无法收回**托盘：`CDROMCLOSETRAY` 返回成功但驱动器无动作，SCSI
  `START STOP UNIT`（LoEj=1, Start=1）返回 `ILLEGAL REQUEST / INVALID FIELD IN CDB (0x24)`。
- **产品决策：接受这个限制。** 弹出后由用户手动推回托盘，这是真实 CD 机的使用方式。

**对代码的约束**：
- `ICDBackend::eject()` 只负责弹出，不负责收回；接口里不提供 `load()`。
- UI 弹出后**不得轮询等待自动关仓**，应进入「等待用户放入光盘」状态，
  检测到介质后自动继续（对应 CD-001 / CD-005）。
- CD-007「弹出前停止播放并释放设备」照常实现，但释放动作止于弹出。
- 没有 DAC 时先用板载声卡代替，`IAudioBackend` 的设备名走配置，不写死。

**Step 4 待办**：需要一张正版音频 CD，然后跑 `scripts/cdda_spike.sh --rip`，
把报告贴进 `docs/hardware_test.md`。

**已经落地、依赖硬件的决策**（详见 `docs/hardware_test.md`）：

- 背光：udev 规则把 `/sys/class/backlight/*/brightness` 改成 `video` 组可写，
  真机五档回读全中，按 raw 值精确恢复。
- 电源键：logind `HandlePowerKey=ignore` + 播放器读 evdev；短按息屏（连背光一起关），
  长按仍由固件断电。
- 显示：真机用 SDL `kmsdrm` 直接跑在 tty2 上，已验证可渲染、可收键。

## D5 · CD-Text 编码与非 ASCII 标题

**决策：非可打印 ASCII 的 CD-Text 一律丢弃，回退成 `TRACK NN`**（2026-11-10）

实测那张自刻 CD 的 CD-Text 是 **GBK** 编码（中文标题），libcdio 按 Latin-1 返回，
结果是 `K¸èÖ®Íõ` 这种乱码。界面用的是自绘 5×7 点阵字体，**只覆盖 ASCII**，
即使正确转码也画不出来。

因此 `sanitizeCdText()` 在 backend 层就丢掉非 ASCII 字节，
`formatTrackTitle()` 自然回退到 `TRACK NN`。英文标题（如 `Waiting For You`）正常保留。

**等字体方案定下来后要回来改这一条**：如果改用 FreeType，就应当把 GBK/UTF-16
转成 UTF-8 再显示，而不是丢弃。
