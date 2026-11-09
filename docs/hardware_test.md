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
