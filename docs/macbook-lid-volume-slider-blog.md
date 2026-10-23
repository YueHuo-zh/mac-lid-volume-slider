# 把 MacBook 的屏幕变成音量滑块：一次 C++ 与 Core Audio 实践

MacBook 的屏幕开合角度能不能用来控制系统音量？

这个项目把想法做成了一个命令行程序：屏幕打开得越大，音量越高。程序通过 HID 设备读取角度，再使用 Core Audio 修改默认输出设备的音量。在我的机器上，实测最大开合角度是 133°，因此把 0°～133° 映射到 0%～100%。

最终验证时，传感器读到 99°，程序计算出约 74.4% 的目标音量。提交设置后重新查询，系统返回约 74.44%，完成了从传感器输入到音量控制的实际验证。

项目使用 C++17 和 CMake。下面主要讨论系统接口、模块边界和控制逻辑，代码片段省略了部分外围处理。

## 先把“系统音量”定位到设备

在这个项目里，“系统音量”具体指默认输出设备的主输出音量。内置扬声器、耳机和外部音频设备可能提供不同的控制能力，不能假定所有设备都有一个可写的主音量属性。

Core Audio 的这部分接口采用对象与属性模型。读取音量需要两次查询：

```text
系统音频对象
    └─ 查询默认输出设备 → AudioDeviceID
                              └─ 查询主输出音量 → Float32
```

两次查询使用相同的 `AudioObjectGetPropertyData`，区别在于对象 ID 和属性地址：

| 查询内容 | 对象 | Selector | Scope / Element |
| --- | --- | --- | --- |
| 默认输出设备 | `kAudioObjectSystemObject` | `kAudioHardwarePropertyDefaultOutputDevice` | Global / Main |
| 主输出音量 | 查询到的设备 ID | `kAudioDevicePropertyVolumeScalar` | Output / Main |

音量属性对应一个 `[0, 1]` 范围的 `Float32`。读取时，函数返回 `OSStatus`，实际数据通过缓冲区参数写回；设置时则使用 `AudioObjectSetPropertyData`，让系统读取传入的数值。

```cpp
// address 已配置为指定设备的主输出音量属性。
Float32 volume = 0.0f;
UInt32 size = sizeof(volume);

OSStatus status = AudioObjectGetPropertyData(
    device, &address, 0, nullptr, &size, &volume);

// 检查读取状态后，按业务规则计算 targetVolume。
status = AudioObjectSetPropertyData(
    device, &address, 0, nullptr,
    sizeof(targetVolume), &targetVolume);
```

Get 接收大小的地址，因为它需要写回实际读取长度；Set 只接收要写入的数据大小。这类接口最重要的是让属性、缓冲区类型和长度保持一致。[Apple：AudioObjectGetPropertyData](https://developer.apple.com/documentation/coreaudio/audioobjectgetpropertydata%28_%3A_%3A_%3A_%3A_%3A_%3A%29?language=objc)

我把这些操作收敛成了三个函数：

```cpp
AudioDeviceID getDefaultOutputDevice();
Float32 getVolume(AudioDeviceID device);
void setVolume(AudioDeviceID device, Float32 volume);
```

模块内部检查属性是否存在、是否可写，以及每次系统调用的返回状态。非法音量抛出 `std::invalid_argument`，系统调用失败抛出 `std::runtime_error`，由入口统一处理。

音量输入除了检查范围，还要用 `std::isfinite` 拒绝 NaN 和无穷值。仅判断小于零或大于一，会漏掉 NaN。

另一个细节是：Set 返回成功不等于所有设备都已经同步完成更新。Apple 文档明确提到部分属性会异步变化，因此程序输出的是“已提交音量”，而不是声称每次都回读确认了当前值。[Apple：AudioObjectSetPropertyData](https://developer.apple.com/documentation/coreaudio/audioobjectsetpropertydata%28_%3A_%3A_%3A_%3A_%3A_%3A%29?language=objc)

## 角度来自 HID 报告

角度读取使用了开源库 [ufoym/mac-angle](https://github.com/ufoym/mac-angle)。它封装了 IOKit HID 的设备发现、打开、报告读取和资源释放，对外提供 `LidAngleSensor::readAngle()`。

在本机上，它匹配的设备条件是：

| 条件 | 值 |
| --- | --- |
| Vendor ID | `0x05AC` |
| Product ID | `0x8104` |
| Usage Page | `0x0020` |
| Usage | `0x008A` |

源码通过 `IOHIDDeviceGetReport` 读取 ID 为 1 的 Feature Report，然后从报告的两个字节中组合出角度值。IOKit 提供了通用读取接口，具体设备的匹配条件和报告解释则来自库对传感器协议的实现，不能据此保证所有 MacBook 都可用。[库的实现](https://github.com/ufoym/mac-angle/blob/main/angle.cpp)

这里也值得核对文档与源码：库的说明提到了 0.01° 分辨率，但当前实现把组合得到的整数直接转换为 `double` 返回。因此，输出 `99.00°` 并不能证明传感器提供了百分之一度的精度。

CMake 中直接用 `add_subdirectory` 纳入第三方项目，让主程序链接 `lid_angle`。这个库已经声明了所需头文件路径，以及 IOKit、Core Foundation 依赖，无需在主项目中再次展开其实现细节。

## 最大数值范围不等于机械最大角度

控制上限不能直接取库里描述的 `360°`。数据能够表示的范围与屏幕铰链实际能打开的范围是两回事。

本项目把闭合端点按 0° 计算，上限采用这台机器的手动实测值 133°。它们目前是编译期配置，没有自动校准或跨机型推断。

映射本身独立成纯函数，接收当前角度和两个端点，返回 `[0, 1]` 的音量比例：

```cpp
const double boundedAngle = std::clamp(angle, closedAngle, maximumAngle);
return static_cast<float>(
    (boundedAngle - closedAngle) / (maximumAngle - closedAngle));
```

实际实现会先检查参数是否有限、上下限是否有效，再进行计算。先限制角度再做减法，也能避免极大输入参与差值计算。

这条规则意味着 66.5° 对应 50%，133° 对应 100%；略高于上限的有效读数仍映射到 100%。主循环另外拒绝非有限值和超出当前传感器检查范围的读数，避免直接把明显异常的输入转换成满音量。

这里的线性关系作用于音量标量，并不意味着人耳感受到的响度也按相同比例变化。后续若要改善手感，可以调整映射曲线，而不必改变硬件访问模块。

## 控制循环需要记住什么

当前实现采用同步轮询，每次读取后等待 100 毫秒。这个频率足够先验证交互，但加上设备访问耗时，实际采样频率会略低于每秒 10 次，也不提供实时性保证。

循环中记录两个状态：

```cpp
std::optional<float> lastTargetVolume;
AudioDeviceID lastDevice = kAudioObjectUnknown;
```

只有以下情况才提交设置：第一次读取、目标音量变化，或者默认输出设备切换。

```cpp
if (!lastTargetVolume ||
    targetVolume != *lastTargetVolume ||
    device != lastDevice)
{
    setVolume(device, targetVolume);
    lastTargetVolume = targetVolume;
    lastDevice = device;
}
```

`lastTargetVolume` 在设置成功后才更新。如果系统调用失败，不能把这次目标记成已经处理完成。

默认输出设备在每轮正式控制中重新查询。否则，用户连接耳机后，如果屏幕角度没变，程序可能继续持有旧设备 ID，或者无法把当前目标应用到新设备。

这个版本没有持续强制系统音量等于目标值。用户手动调整音量后，只要目标值和输出设备都没变化，程序就不会重新写入；下次目标变化时才恢复角度控制。这是当前的交互选择，也避免了每 100 毫秒重复提交相同设置。

传感器对象放在循环外创建。Ctrl+C 和 SIGTERM 的处理函数只设置 `volatile std::sig_atomic_t` 标记，由主循环正常退出，让局部对象析构并释放设备资源。

## 把硬件副作用留在测试边界外

这个项目有三项自动化测试。

映射测试覆盖端点、中点、越界限制、单调性，以及 NaN、无穷值和非法上下限。它不依赖 macOS 设备状态。

控制循环测试则编译真实的 `main.cpp`，但在链接时提供模拟的传感器和音量接口。输入序列包含重复角度和设备切换，检查程序是否跳过重复设置、是否向新设备提交相同目标，以及能否通过信号退出。

预览模式测试使用相同入口，确认 `--dry-run` 路径不会调用音频设备查询和设置接口。三项测试都不会修改真实系统音量。

自动化测试之外，再分两步验证硬件：

```bash
# 验证真实传感器和映射，不改变音量。
./build/MacLidVolumeSlider --dry-run --once

# 按当前角度实际设置一次音量。
./build/MacLidVolumeSlider --once
```

前面提到的 99° → 74.44% 就来自一次真实写入后的独立回读。模拟测试验证程序决策，硬件测试验证当前机器上这条调用链是否成立，两者覆盖的范围不同。

## 能运行之后，仍然有哪些边界

最容易忽略的是合盖睡眠。0° 虽然映射到 0%，但系统可能在程序采样到完全闭合之前就进入睡眠。因此，这个实现不能保证合盖瞬间音量一定归零，也没有尝试阻止睡眠。

另外，它目前只处理设备的主输出音量属性。遇到不支持该属性的设备会报错退出，没有为分声道控制或其他设备类型增加回退逻辑。程序不会解除静音；传感器读取或音频操作失败后，也没有自动重连机制。

退出时保留最后的系统音量，没有恢复启动前的值。当前形态是前台命令行程序，尚未提供菜单栏、开机自启或后台服务。

这些边界在功能很小的时候就值得写清楚：它们直接决定了用户换设备、合盖、手动调音量时，程序应该表现成什么样。

## 项目如何运行

本项目要求 macOS、可用的角度传感器、支持 C++17 的工具链，以及 CMake 3.31 或更新版本。第三方库源码位于 `third_party/mac-angle`。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure

# 连续预览。
./build/MacLidVolumeSlider --dry-run

# 实际控制音量，Ctrl+C 退出。
./build/MacLidVolumeSlider
```

最终代码分成三层：`audio_volume` 负责 Core Audio，`volume_mapping` 负责数值转换，`main` 负责把传感器、设备和控制策略连接起来。保留这样的边界后，无论接下来增加校准、曲线映射还是菜单栏界面，都能从相对独立的位置继续扩展。

对这个项目来说，真正有意思的部分，是把一个能读到的硬件数值，变成有明确边界、能测试、也能解释其行为的系统控制工具。
