# 架构

四层结构，依赖方向只有一个：**UI 依赖 Core，Core 依赖接口，接口由后端实现**。
反过来一律不允许——这是 `AGENTS.md` 里那条「`src/core/` 不得出现 `SDL_*`/`snd_*`/`cdio_*`」的来源。

```
        ┌──────────────────────────────────────────┐
        │  UI 层                                   │
        │  main_screen / help_screen / service_    │
        │  screen / theme / text_renderer          │
        └───────────────▲──────────────────────────┘
                        │ 只读 UiModel 快照 + 主题
        ┌───────────────┴──────────────────────────┐
        │  Core 层                                 │
        │  PlayerCore · PlayerStateMachine ·       │
        │  EventBus · Config · Resume · Spectrum   │
        └───────────────▲──────────────────────────┘
                        │ 只调接口
        ┌───────────────┴──────────────────────────┐
        │  后端层（Mock 与 Real 同接口，可互换）    │
        │  ICDBackend · IAudioBackend ·            │
        │  IBacklightDevice · EvdevKeyboard        │
        └───────────────▲──────────────────────────┘
                        │
        ┌───────────────┴──────────────────────────┐
        │  系统层                                  │
        │  libcdio/paranoia · ALSA · evdev · sysfs │
        │  systemd · logind · udev                 │
        └──────────────────────────────────────────┘
```

## 接口

```cpp
class ICDBackend {                       // src/cd/cd_interface.h
    bool open();  void close();
    bool detectDisc();
    DiscInfo readTOC();                  // 轨数、各轨 LBA、总时长、CD-Text
    int readFrames(int track, int startLba, int count, std::vector<int16_t>& out);
    bool eject();                        // 只有弹出，没有 load()（D4）
};

class IAudioBackend {                    // src/audio/audio_interface.h
    bool open(const std::string& device);  void close();
    bool start();  bool pause();  bool stop();
    bool setVolume(float);               // 软件音量（D2）
    int write(const int16_t* frames, int frameCount);
    AudioState state();  long long framesWritten();
};

class IBacklightDevice {                 // src/display/backlight.h
    bool available();  int percent();  bool setPercent(int);
    int raw();  bool setRaw(int);        // 精确恢复用
};
```

## 数据流

```
CD 扇区 ──readFrames──▶ PlayerCore::tick()
                              │
                              ├──▶ IAudioBackend::write()   （出声）
                              └──▶ SpectrumAnalyzer::push() （频谱 / VU 表）

PlayerCore 状态 ──▶ UiModel（纯数据快照）──▶ renderMainScreen()
```

- **播放状态只由 `PlayerStateMachine` 维护**，UI 和硬件模块只读。
- **UI 不持有状态**：每次渲染拿一个 `UiModel`，画完就丢。
- **动画不在这里**：碟片角度、冒号闪烁由主循环的单调时钟算好，作为字段塞进 `UiModel`，
  渲染函数保持纯函数（同样的输入 → 同样的像素），这也是它能被帧缓冲对比测试的原因。

## 线程模型

**单线程**，不用锁：

- 音频写入本身就是阻塞的（`snd_pcm_writei` 或 Mock 的节流），天然形成节拍。
- 主循环每处理一个音频块（4 扇区 ≈ 53 ms）就渲染一帧，约 18 fps。
- 代价是动画帧率受音频块大小限制；收益是没有数据竞争、没有锁、没有线程生命周期管理。

## 状态存在哪

| 数据 | 位置 | 生命周期 |
|---|---|---|
| 播放状态 | `PlayerStateMachine`（内存） | 进程内 |
| 播放位置 / 当前主题 | `~/.local/state/retrodisc/state.conf` | 跨重启 |
| 音量 / 键位 / 设备名 | `/opt/retrodisc/etc/*.conf` | 用户可改，部署不覆盖 |
| 日志 | journald（上限 64 MB） | 1 周 |

## 两种运行模式

| 模式 | 后端 | 用途 |
|---|---|---|
| `--mock` | `MockCDBackend` + `MockAudioBackend` | 无硬件也能跑通全流程；CI 与开发 |
| 真机 | `RealCDBackend`（**待写**）+ `ALSAAudioBackend` | 目标机 |

`--render` 决定是否开窗口；在目标机上配合 `SDL_VIDEODRIVER=kmsdrm` 直接接管 VT。

## 刻意不做的事

- 不做专辑封面、媒体库、数据库索引（PRD 3.2）。
- 不做鼠标、触屏、网络（D3）。
- 不做风扇控制、内核参数调整（`AGENTS.md` 第 6 节）。
- 不引入除 SDL2 / ALSA / libcdio / libevdev / systemd 之外的依赖。
