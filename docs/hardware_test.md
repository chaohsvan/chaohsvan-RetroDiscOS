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

## DAC / 音频输出（待做）

本机 shell 无 `/dev/snd`，需要真实声卡环境。

## 背光 / ACPI 电源键（待做）

需要目标笔记本。

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

### 待填：实测结果

_（插盘后运行上面的命令，把报告贴在这里）_
