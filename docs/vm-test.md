# Debian 虚拟机验证（T17）

记录 2026-11-09 实际执行过的验证流程与结果。目的是让"开机自启动"这件事**可复现**，
而不是只留一句"验证过"。

## 环境

| 项 | 值 |
|---|---|
| 宿主 | Ubuntu 26.04, qemu 10.2.1, `/dev/kvm` 可用 |
| 镜像 | `debian-13-genericcloud-amd64.qcow2`（Debian 13 trixie） |
| 内核 | 6.12.115+deb13.1-cloud-amd64 |
| 规格 | 2 vCPU / 2 GB RAM / 镜像扩容 +6 GB |
| 访问 | qemu user 网络 + 端口转发 `127.0.0.1:2222 -> :22`，cloud-init 注入 SSH 公钥 |

## 步骤

```bash
# 1. 准备 cloud-init seed
genisoimage -output seed.iso -volid cidata -joliet -rock user-data meta-data

# 2. 启动（无显示，串口日志落盘）
qemu-system-x86_64 -enable-kvm -cpu host -m 2048 -smp 2 \
  -drive file=debian-13-cloud.qcow2,if=virtio,format=qcow2 \
  -drive file=seed.iso,if=virtio,format=raw,media=cdrom \
  -netdev user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22 \
  -device virtio-net-pci,netdev=net0 \
  -display none -serial file:console.log -monitor none -daemonize

# 3. 源码进 VM（不拷二进制：Ubuntu 26.04 的 glibc 比 Debian 13 新）
tar czf - --exclude=./build --exclude=./.git . | ssh -p 2222 retro@127.0.0.1 "tar xzf - -C ~/retrodisc"

# 4. VM 内安装依赖并编译（SDL2/ALSA/libcdio/libevdev/py... 全走 apt）
sudo apt-get install -y build-essential cmake pkg-config \
  libsdl2-dev libasound2-dev libcdio-dev libcdio-paranoia-dev libudev-dev libevdev-dev
cmake -S ~/retrodisc -B ~/retrodisc/build -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j2
ctest --test-dir build --output-on-failure

# 5. 安装为服务
sudo useradd -r -m -d /home/retrodisc retrodisc
sudo usermod -aG audio,video,cdrom,input retrodisc
sudo ~/retrodisc/scripts/deploy.sh --prefix /opt/retrodisc --build build \
  --unit-dir /etc/systemd/system --tmpfiles-dir /etc/tmpfiles.d
sudo systemd-tmpfiles --create retrodisc.conf
sudo systemctl daemon-reload && sudo systemctl enable retrodisc
```

虚拟机没有光驱也没有显示，所以验证时用 drop-in 注入 `RETRODISC_EXTRA=--mock --auto-disc --seconds 3600`，
让播放器跑 Mock 全流程。**单元文件本身不改**，真机删掉这个 drop-in 即可。

## 结果

| 验证项 | 结果 |
|---|---|
| VM 内 `ctest` | 19/19 通过（Debian 13 上从源码重建） |
| `systemctl is-enabled` | `enabled` |
| 重启后 `is-active` | `active`，MainPID 678 自动拉起 |
| 重启后日志 | `[toc] 12 tracks` / `[state] PLAYING` / `[play] track 01 00:16` |
| 崩溃自愈 | `kill -9` 后 6 秒内从 PID 1437 变为 1474，重新播放 |
| `/dev/tty1` | 归属 `retrodisc:tty` |

## 验证中发现并修掉的两个真问题

1. **`StandardInput=tty` 报 `status=208/STDIN: Operation not permitted`**
   根因不是权限位，而是 `DevicePolicy=auto`：一旦写了 `DeviceAllow=`，
   策略就变成 closed，`/dev/tty1` 不在白名单里。修法是显式加
   `DeviceAllow=/dev/tty1 rw`。另外 `/dev/tty1` 默认 0600，需要 tmpfiles
   规则 `z /dev/tty1 0620 root tty -` 才能让服务用户打开。

2. **播放器日志进不了 journal**
   `StandardInput=tty` 时 stdout 也指向 VT，且 stdout 是管道时按块缓冲，
   启动日志全部丢失。修法：单元里显式 `StandardOutput=journal`，
   程序里 `setvbuf(stdout, ..., _IOLBF, 0)`。

这两个问题**只有真机/虚拟机跑一遍才会暴露**，单测和 `systemd-analyze verify` 都发现不了。

## 未覆盖

- 真实显示输出（kmsdrm / X）：VM 只有串口控制台，需在目标笔记本验证。
- 光驱、DAC、背光、ACPI 电源键：VM 无对应硬件。
