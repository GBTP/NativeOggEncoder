# NativeOggEncoder

原生 C 库，用于将 PCM 音频编码为 OGG Vorbis 格式，内置采样率转换。设计为 Unity 跨平台插件。

## 特性

- 基于 libvorbis 的 OGG Vorbis VBR 编码
- 基于 Speex resampler 的内置重采样
- 支持 planar 和 interleaved 浮点 PCM 输入
- 线程安全的错误报告
- 支持单声道和立体声

## 支持平台

| 平台 | 架构 | 产物 |
|------|------|------|
| Windows | x64 | native_ogg_encoder.dll |
| Linux | x64 | libnative_ogg_encoder.so |
| macOS | x64 | libnative_ogg_encoder.dylib |
| macOS | arm64 | libnative_ogg_encoder.dylib |
| Android | arm64-v8a | libnative_ogg_encoder.so |
| Android | armeabi-v7a | libnative_ogg_encoder.so |
| Android | x86_64 | libnative_ogg_encoder.so |
| Android | x86 | libnative_ogg_encoder.so |
| iOS | arm64 | libnative_ogg_encoder.a（静态库） |

## Unity 集成

下载最新 Release，将 `NativeOggEncoder/` 文件夹复制到项目的 `Assets/Plugins/` 下即可。

```csharp
// 基本用法（不重采样）
byte[] ogg = NativeOggEncoder.ConvertToBytes(samples, 44100, 2, 0.6f);

// 带重采样（48000 → 44100）
byte[] ogg = NativeOggEncoder.ConvertToBytes(samples, 48000, 44100, 2, 0.6f);
```

参数说明：
- `samples` — `float[][]` planar 音频数据（每个通道一个数组）
- `inputSampleRate` — 源采样率
- `outputSampleRate` — 目标采样率（不同时自动重采样）
- `channels` — 声道数（1 或 2）
- `quality` — Vorbis VBR 质量（0.0 ~ 1.0）

## 从源码构建

依赖：CMake 3.16+，C11 编译器。

```bash
git clone --recursive https://github.com/GBTP/NativeOggEncoder.git
cd NativeOggEncoder
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

所有平台通过 GitHub Actions 自动构建。推送 `v*` 标签时会自动创建 Release 并上传完整的 Unity 插件包。

## 许可证

MIT License

本项目使用的第三方库：
- [libogg](https://github.com/xiph/ogg) — BSD
- [libvorbis](https://github.com/xiph/vorbis) — BSD
- [Speex resampler](https://speex.org/) — BSD
