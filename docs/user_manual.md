# RetroDisc OS 用户手册

> 对应版本：0.1.0 · 交付物 DEL-009
> 项目当前状态：M0–M3 完成，M4 的真实 CD 读取尚未接入（见文末「已知限制」）。

---

## 1. 这是什么

一台专用 CD 播放机：开机直接进全屏播放界面，读实体 CD，纯键盘操作，离线可用。
它不是桌面系统，装好之后没有桌面、没有浏览器，只有播放器。

## 2. 安装

### 2.1 准备

| 项 | 要求 |
|---|---|
| 系统 | Debian 13 (trixie) 或同代 |
| 光驱 | USB 外置 CD/DVD 光驱 |
| 音频 | 板载声卡或 USB DAC |
| 权限 | 服务用户需要在 `audio` `video` `cdrom` `input` `tty` 组里 |

### 2.2 编译与安装

```bash
sudo apt install -y build-essential cmake pkg-config \
  libsdl2-dev libasound2-dev libcdio-dev libcdio-paranoia-dev \
  libudev-dev libevdev-dev libsystemd-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure

sudo scripts/deploy.sh --prefix /opt/retrodisc --build build \
  --unit-dir /etc/systemd/system \
  --tmpfiles-dir /etc/tmpfiles.d \
  --etc-dir /etc
```

### 2.3 创建服务用户并启用

```bash
sudo useradd -r -m -d /home/retrodisc -s /bin/bash retrodisc
sudo usermod -aG audio,video,cdrom,input retrodisc

sudo systemd-tmpfiles --create retrodisc.conf
sudo systemctl daemon-reload
sudo systemctl enable --now retrodisc
```

### 2.4 升级与回滚

```bash
sudo scripts/deploy.sh --prefix /opt/retrodisc --build build   # 升级
sudo scripts/rollback.sh --prefix /opt/retrodisc               # 回滚到上一版
```

配置文件**永远不会被覆盖**，升级不会丢你的键位和音量设置。

## 3. 按键

所有键位都在 `/opt/retrodisc/etc/keys.conf` 里，改完重启服务即可生效，不需要重新编译。

| 按键 | 功能 | 按键 | 功能 |
|---|---|---|---|
| `SPACE` | 播放 / 暂停 | `F1` | 播放模式循环 |
| `LEFT` | 上一曲 | `F2` | 随机播放 |
| `RIGHT` | 下一曲 | `F3` | 循环模式 |
| `UP` | 音量 + | `F5` | 切换主题 |
| `DOWN` | 音量 − | `F6` / `F7` | 亮度减 / 增 |
| `F10` | 静音 | `F8` | 息屏 / 亮屏 |
| `F11` / `F12` | 快退 / 快进 | `F9` | 频谱显示 |
| `E` | 弹出托盘 | `H` | 帮助 |
| `X` | 停止 | `S` | 诊断模式 |
| `Q` | 退出播放器 | | |

> 托盘弹出后**必须手动推回**。这台光驱的固件不支持软件关仓（详见 `docs/decisions.md` D4）。

## 4. 主题

`F5` 循环切换，选择会保存，下次开机沿用：

| 主题 | 风格 |
|---|---|
| `digital-hifi` | 绿色数字 Hi-Fi（默认） |
| `classic-vfd` | 青绿荧光屏 |
| `amber` | 琥珀色 |
| `vintage-analog` | 米白暖色 |
| `off` | 纯黑，只剩声音 |

## 5. 亮度

- `F6` / `F7` 在 **100% / 75% / 50% / 25% / 0%** 之间调节。
- 同时作用于**界面亮度**和**屏幕背光**两个独立通道。
- 到 0% 时屏幕基本全黑，**按任意键回到 25%**，且不会触发那个键原本的功能。
- `F8` 息屏会同时关闭背光；再按任意键亮屏并恢复原来的背光值。

背光需要 udev 规则授权（`deploy.sh --etc-dir` 已包含）。没有背光设备时（台式机、虚拟机）
只调界面亮度，不会报错。

## 5.5 界面上的动效

| 元素 | 行为 |
|---|---|
| 播放图标 | 状态行左侧：播放 ▶ / 暂停 ⏸ / 停止 ■ |
| 冒号 | 时钟里的冒号每 0.5 秒闪一次 |
| 碟片 | 右上角圆盘，播放时约 1 秒转一圈，暂停立即停住 |
| 鼠标指针 | 始终隐藏（CD 机不需要指针） |

刷新率约 18 fps（每 53 ms 一个音频块）。

## 6. 播放模式

- `F1` 依次循环：顺序 → 整碟循环 → 单曲循环 → 随机
- `F2` 直接切到随机
- `F3` 在整碟循环 / 单曲循环 / 顺序之间切换

随机模式保证不会连续两次播放同一首。

## 7. 断点续播

停止播放时自动记录「哪张碟、第几轨、第几秒」，下次放入**同一张碟**会自动回到那个位置。

- 换一张碟不会误恢复（按光盘 TOC 生成的 ID 匹配）。
- 位置按扇区对齐；记录点超出轨道长度时从该轨开头播放。
- 状态保存在 `~/.local/state/retrodisc/state.conf`。

## 8. 帮助与诊断

- `H`：离线帮助，10 个条目，`UP`/`DOWN` 滚动，`H` 或 `ESC` 关闭。
- `S`：诊断模式，只读展示 10 项硬件状态：

| 项目 | 内容 |
|---|---|
| CD-ROM | 设备名与型号 |
| AUDIO | 声卡是否存在 |
| INPUT | 输入设备数量 |
| DISPLAY | 已连接的显示输出 |
| BACKLIGHT | 背光设备数量 |
| ACPI | 电源设备 |
| CPU | 型号与核心数 |
| FAN | 风扇转速（只读，绝不控制） |
| STORAGE | 剩余 / 总空间 |
| SYSTEM | 播放器版本与内核版本 |

## 9. 配置文件

位置：`/opt/retrodisc/etc/`

**player.conf**

| 键 | 说明 | 默认 |
|---|---|---|
| `audio_device` | ALSA 设备名，`null` 表示丢弃 | `null` |
| `volume` | 初始音量 0.0–1.0（软件音量） | `0.8` |
| `theme` | 初始主题 | `digital-hifi` |
| `resume_file` | 断点续播文件路径 | — |

**keys.conf**：`动作 = 按键名`，动作名固定，见第 3 节。

配置写错不会导致启动失败：非法行会被忽略并在日志里告警。

## 10. 故障排查

| 现象 | 检查 |
|---|---|
| 开机没进播放器 | `systemctl status retrodisc`；`journalctl -u retrodisc -n 50` |
| 界面黑屏但能按键 | 亮度是不是 0%（按 `F7`）；`F8` 是否处于息屏 |
| 没有声音 | `aplay -l` 有没有设备；`player.conf` 的 `audio_device` 对不对 |
| 提示无权限读光驱 | `scripts/check_permissions.sh` |
| 服务反复重启 | 看退出码：`78` 表示二进制或配置缺失；其它看 journal |
| 屏幕亮度调不动 | `scripts/check_power.sh`；背光 udev 规则是否安装了 |

日志一律走 journald，且被限制在 64 MB 以内（`/etc/systemd/journald.conf.d/retrodisc.conf`）。

## 11. 已知限制

1. **真实 CD 播放尚未接入**：`RealCDBackend`（M4）要等 CD-DA 抓轨可行性验证通过。
   当前只能以 `--mock` 运行，用模拟光盘跑通全部界面与逻辑。
2. **界面文字仅支持 ASCII**：点阵字体没有汉字，帮助与诊断页是英文。
3. **软件音量**：不保证 Bit-Perfect（见 `docs/decisions.md` D2）。
4. **背光**：需要 udev 规则 + `video` 组；未安装时自动降级为只调界面亮度。
5. **功耗与长稳**：尚未在真机上做 24 小时连续播放验证。
