# 真机验证记录

所有结论必须来自实测输出，不接受"应该可以"。

---

## 设备信息

| 项 | 值 | 采集方式 |
|---|---|---|
| 光驱型号 | `HL-DT-ST DVDRAM GP78N` | `cat /sys/block/sr0/device/model` |
| 固件版本 | `1.01` | `cd-drive -d /dev/sr0` |
| 接口 | USB 外置 | `lsusb` |
| 托盘类型 | Tray | `cd-drive` Removable Medium Feature |
| C2 错误指针 | 支持 | `cd-drive` CD Read Feature |
| CD-Text | 支持 | `cd-drive` CD Read Feature |

## 托盘行为

| 动作 | 结果 | 备注 |
|---|---|---|
| 弹出 | ✅ 可用 | `eject /dev/sr0` |
| 收回 | ❌ 不可用 | 固件拒绝 `START STOP UNIT`(LoEj=1,Start=1)，返回 `ILLEGAL REQUEST / INVALID FIELD IN CDB (0x24)` |

结论：**弹出后必须手动推回**。已作为产品行为记录在 `docs/decisions.md` D4。

## CD-DA 读取（待做）

需要一张正版音频 CD。步骤：

```bash
cdparanoia -vsQ                 # 读 TOC，判断是否支持精确读取
cdparanoia -B -d /dev/sr0 1     # 抓第一轨，人工试听有无爆音
```

通过标准：

- `cdparanoia -vsQ` 能列出全部音轨且无 read error；
- 抓轨过程无 "scsi command aborted" / "read error"；
- 抓出的 WAV 在 Audacity 里看不到规律性静音段或重复段。

结果（待填）：_未测试_

## DAC / 音频输出（部分验证）

| 项 | 结果 |
|---|---|
| ALSA 后端 | `ALSAAudioBackend` 已完成；台式机 `default` 设备实测写入 4410 帧成功 |
| 目标机声卡 | `sof-hda-dsp`（Sound Open Firmware），存在 `pcmC1D0p` 等播放节点 |
| 目标机实测 | **未做**——需要真实播放路径（等 `RealCDBackend`） |
| USB DAC | 尚未接触过，无型号信息 |

## 背光（已实测，2026-11-09）

目标机：`/sys/class/backlight/intel_backlight`，`max_brightness = 120000`。

| 项 | 结果 |
|---|---|
| 默认权限 | `root:root 0644` —— 服务用户（`video` 组）**写不了** |
| 解决 | `system/85-retrodisc-backlight.rules` 改成 `root:video` 且组可写 |
| 五档写入 | 100 / 75 / 50 / 25 / 0 全部按预期写入并回读一致 |
| 精确恢复 | 按 raw 值恢复（37499 → 37499），不是恢复到最近档位 |
| 0% 唤醒 | `wakeFromBlank()`：任意键回到 25%，且不触发该键原动作 |

## ACPI 电源键（已实测，2026-11-09）

| 项 | 结果 |
|---|---|
| 设备 | `/dev/input/event7`，名 `Power Button`，具备 `KEY_POWER` |
| logind | `HandlePowerKey=ignore`（`systemd-analyze cat-config` 与 `busctl` 双向确认） |
| 短按 | 连按 4 次，`display off` / `on` 交替，背光 `0% ↔ 31%` |
| 息屏语义 | 同时写背光为 0 并记住 raw 值，唤醒精确恢复 |
| 长按 | 未实测（会直接断电）；由 EC/固件处理，不受 OS 配置影响 |

## 图形输出（已实测，2026-11-10）

| 项 | 结果 |
|---|---|
| 后端 | SDL `kmsdrm`，直接运行在 tty2（`openvt -c 2 -s`） |
| 渲染 | 正常，无 SDL 错误；`当前 VT: tty2` |
| 输入 | 键盘事件在 kmsdrm 下可用（SPACE/F5/F6/F7/F8/Q） |
| 鼠标 | 已 `SDL_ShowCursor(SDL_DISABLE)`，并在鼠标移动时再次隐藏 |

---

## 刻盘要求（T23 前置条件）

自刻盘完全可以用来做可行性验证，但**必须是 Audio CD**，否则验证的是数据盘而不是 CD-DA：

| 要求 | 说明 |
|---|---|
| 光盘格式 | **Audio CD (CD-DA)**。把 WAV 文件当数据刻进 ISO 不算，光驱会把它当数据盘 |
| 盘片类型 | 优先 **CD-R**；CD-RW 反射率低，部分笔记本光驱读不稳 |
| 源文件 | 16 bit / 44.1 kHz / 立体声 WAV；MP3 转 CD 也行，但音质不再是验证重点 |
| 时长 | 每轨 ≥ 4 秒（太短部分刻录软件会拒绝） |
| CD-Text | 建议勾选，正好验证 CD-003；不勾也能验证读取本身 |
| 标签 | **不要贴标签纸**（配重不均会导致读取失败），用记号笔写 |

刻完记录：刻录软件、倍速、源格式。这些写进下面的报告里。

## 验证步骤

插盘后推回托盘（这台光驱软件关不上仓，见 D4），然后：

```bash
# 第一步：只读 TOC，10 秒出结论
./scripts/cdda_spike.sh

# 第二步：抓第一轨并分析 PCM（约 50 MB 临时文件）
./scripts/cdda_spike.sh --rip
```

脚本会输出一份 markdown 报告，包含：设备型号、盘片容量、TOC、音轨数、
WAV 的格式/时长/峰值/最长静音段。把报告原样贴到本节下面即可。

判定标准：

- `tracks detected` > 0；
- `cdparanoia` 退出码 0；
- `peak` 不为 0（0 表示抓到的是数字静音，说明读取失败）；
- `longest near-silent run` 小于 0.05 秒（更大的静音段通常是读丢帧）。

### 实测结果（2026-11-10，自刻 Audio CD，T23 通过）

| 项 | 结果 |
|---|---|
| 光盘类型 | Audio CD（CD-DA），`cdparanoia` 能直接读 TOC |
| 音轨 | 4 轨，总时长 21:10.74 |
| TOC 读取 | 成功，`cdparanoia -vsQ` 退出码 0 |
| 抓轨 | `cdparanoia -w` 抓第 1 轨，退出码 0，无 read error / 无 "Repairing" |
| 格式 | 44100 Hz / 2 ch / 16 bit |
| 轨长 | 224.36 s（3:44.26，与 TOC 一致） |
| 峰值 | 32069（97.9% 满刻度，不是数字静音） |

**结论：T23 PASS。** 这块 `DVDRAM GP78N` 能可靠读取 CD-DA 数字音频，
`RealCDBackend`（T24）可以基于 libcdio-paranoia 实现。

#### 一次假阳性及其修正

第一次跑脚本时报「6.822 s 静音段，疑似读丢」。追查后确认是**误报**：

- 该静音段位于音轨**末尾 3.41 s**（音源自带的尾部静音），另有开头 0.33 s；
- 全长 1.5% 的采样是精确 0，属于正常；
- `cdparanoia` 退出码 0、没有任何重读或修复日志。

脚本原来的判据是「存在超过 50 ms 的静音 → 警告」，对音乐根本不成立。
已改为：**只检查音轨中段**的静音，并且要求该段几乎全是精确 0（读坏扇区被填零的特征）
才报警；头尾静音一律忽略。修正后重跑，输出为
`no suspicious silent gaps`。

## 目标机硬件普查（2026-11-09，`scripts/hw_survey.sh`）

| 项 | 值 |
|---|---|
| 机型 | ASUSTeK TUF Gaming FX505GT_FX95GT |
| CPU | Intel Core i7-9750H，12 线程，`intel_pstate` / `powersave` |
| 内存 | 15.5 GiB |
| 根分区 | 18 GiB（已用 9%） |
| 背光 | `intel_backlight`，max 120000，初始 37499 |
| DRM | `card0` `card1` `renderD128` `renderD129` |
| 电源 | `AC0` 在线，`BAT0` 91% |
| 温度 | 8 个 thermal zone（`x86_pkg_temp` 47 °C 等） |
| 风扇 | `hwmon5/fan1_input` 2700 RPM、`fan2_input` 2800 RPM |
| 音频 | `sof-hda-dsp` + NVidia HDMI |
| 键盘 | `AT Translated Set 2 keyboard`（event0） |
| 电源键 | `Power Button`（event7） |

## CPU 占用（2026-11-10，Mock 播放）

| 项 | 结果 |
|---|---|
| 采样方式 | 读 `/proc/<pid>/stat` 的 utime+stime，10 秒窗口 |
| CPU 占用 | **1.3%**（目标 < 15%） |
| 调速器 | `powersave`（`intel_pstate`） |
| 说明 | Mock 播放不含真实抓轨与解码开销，`RealCDBackend` 上线后必须复测 |
