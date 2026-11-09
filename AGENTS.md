# AGENTS.md — RetroDisc OS 工程规则

本文件是任何 AI 编码 agent（Claude Code / Codex CLI / OpenCode / DSH 等）
进入本仓库后的**第一份必读文件**。规则优先级高于任何临时指令；
与用户当次指令冲突时，先停下来说明冲突，不要自行取舍。

---

## 1. 项目一句话

Debian 上的专用 CD 播放器：开机全屏进入播放界面，读实体 CD-DA，
经 USB DAC 输出，复古显示风格，纯键盘控制，离线可用。

## 2. 技术栈（不要替换）

| 层 | 技术 |
|---|---|
| 语言 | C++17 |
| 构建 | CMake ≥ 3.16 + CTest |
| 图形 | SDL2（全屏） |
| 音频 | ALSA |
| CD | libcdio + libcdio-paranoia |
| 输入 | evdev |
| 服务 | systemd |

## 3. 构建与测试（每次改动后必须执行）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
```

- **没有通过 `ctest` 的改动不算完成**，不要用"应该可以""逻辑上没问题"代替实测输出。
- Debug 构建默认开启 `-fsanitize=address,undefined`（见 `CMakeLists.txt`）。
- 提交前跑 `clang-format -i`（配置在 `.clang-format`）。

## 4. 目录边界（硬规则）

```
src/core/     播放状态机、事件总线、曲目管理
src/cd/       ICDBackend / RealCDBackend / MockCDBackend
src/audio/    IAudioBackend / ALSAAudioBackend / MockAudioBackend
src/input/    键盘事件、按键映射、长按识别
src/display/  SDL2 渲染、主题、亮度
src/system/   电源、硬件监控、日志
src/ui/       主界面、设置、帮助、诊断
config/      player.conf / keys.conf / display.conf
tests/       CTest 用例
scripts/     build.sh / test.sh / deploy.sh
system/      retrodisc.service
docs/        architecture.md / hardware_test.md / decisions.md
```

- 一次任务**只改一个模块目录**，跨模块改动必须先在回复里说明原因。
- `src/core/` 不允许出现任何 `SDL_*`、`snd_*`、`cdio_*`、`open("/dev/...")` 调用。
  硬件访问只能通过接口注入。
- 禁止把业务逻辑写进 `src/ui/`；UI 只负责把状态渲染出来 + 把按键翻译成事件。

## 5. 架构约束

1. **两种后端必须共存**：`Mock*` 与 `Real*` 实现同一接口，`main` 用 `--mock` 切换。
   Mock 模式下程序必须能在**无光驱、无声卡**的机器上完整跑通全流程。
2. **单一事实来源**：播放状态只由 `PlayerStateMachine` 维护，UI 和硬件模块只读不写。
3. **状态迁移必须集中**：非法迁移返回 false 并记日志，不要 `assert` 崩溃。
4. **接口先行**：新增硬件能力 → 先在接口里加纯虚函数 → 补 Mock → 再写 Real。
5. **配置与程序分离**：所有可调参数（键位、主题、亮度档位、设备名）走 `config/*.conf`，
   代码里只保留默认值。

## 6. 禁止事项（违反即视为任务失败）

- ❌ 自动执行 `fdisk` / `parted` / `mkfs` / `grub-install` / EFI 相关命令。
- ❌ 修改内核参数、`/etc/default/grub`、风扇/EC/热保护相关设置。
- ❌ `rm -rf` 任何源码目录外的路径；不要 `git push --force`。
- ❌ 一次性生成整个工程（超过 1 个任务卡的改动）。
- ❌ 为了"让测试通过"而放宽断言、注释掉测试、加 `try/catch` 吞异常。
- ❌ 引入新的第三方依赖（除 SDL2 / ALSA / libcdio / evdev / systemd 外），
  确需引入必须先说明理由和体积影响。

## 7. 任务卡工作流

每个任务按此顺序，**不要跳步**：

1. 读本文件 + 相关 `docs/`；
2. 用一句话复述任务和验收标准，不确定就先问；
3. 写/改代码（只碰任务卡指定目录）；
4. 构建 + 跑相关 ctest；
5. 汇报：改了什么 / 怎么验证的 / 有什么没解决。

提交信息格式：

```
<模块>: <一句话做了什么>

- 验证: cmake --build + ctest 结果
- 未解决: ...
```

## 8. 历史决策（不要反复推翻）

见 `docs/decisions.md`。目前关键结论：

- **D1 播放路径**：走数字抓轨（libcdio-paranoia → ALSA），不用光驱模拟输出。
- **D2 Bit-Perfect**：非硬要求；音量先用 ALSA `softvol`，Bit-Perfect 列为 P1。
- **D3 输入**：纯键盘（evdev），不做鼠标 hit-test。

需要推翻以上任一结论时，先在回复里提出，由人确认后再改，不要直接改代码。

## 9. 硬件相关

- 风扇**只读监测**，绝不控制；不得关闭 BIOS/EC 热保护。
- 背光、ACPI 电源键、光驱弹出均按"能力探测 + 不可用则降级"实现，
  不允许 `exit(1)` 直接退出。
- 涉及 `/dev/sr*`、`/dev/snd/*`、`/sys/class/backlight/` 的代码必须能在
  设备不存在时优雅降级，并保证 Mock 模式不受影响。

## 10. 参考文档

- `README.md` — PRD + TDD（需求来源，唯一权威需求文档）
- `docs/architecture.md` — 四层架构与接口签名
- `docs/decisions.md` — 架构决策记录
- `docs/hardware_test.md` — 真机验证记录（光驱/DAC/背光/ACPI）
