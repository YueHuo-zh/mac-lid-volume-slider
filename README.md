# MacLidVolumeSlider

把 MacBook 屏幕的开合角度变成系统音量控制器：屏幕打开得越大，目标音量越高。

这是一个边开发边学习 **C++17、CMake 和 macOS 系统接口**的命令行项目。
程序通过 [mac-angle](https://github.com/ufoym/mac-angle) 读取角度，
通过 Core Audio 修改当前默认输出设备的音量。

## 当前功能

- 按约 100 毫秒的间隔持续读取屏幕开合角度。
- 将 **0°～133°** 线性映射到 **0%～100%** 音量。
- 启动时应用一次目标音量，之后仅在目标值或默认输出设备变化时提交设置。
- 跟随系统默认输出设备切换，例如从内置扬声器切换到耳机。
- 支持只预览、单次执行，以及 Ctrl+C 正常退出。
- 提供纯计算测试和使用模拟硬件的控制循环测试。

**默认运行会实际改变系统音量，退出后保留最后的音量，不恢复启动前的值。**
想先观察角度和映射结果，可以使用 `--dry-run`。

## 环境与依赖

| 依赖 | 要求或用途 |
| --- | --- |
| macOS / MacBook | 需要能被 `mac-angle` 访问的开合角度传感器 |
| Apple Clang 与 macOS SDK | 安装 Xcode 或 Xcode Command Line Tools，支持 C++17 |
| CMake | 3.31 或更新版本 |
| Git | 下载第三方库源码时使用 |
| Core Audio | 系统提供，负责设备查询和音量读写 |
| IOKit、Core Foundation | 系统提供，由 `mac-angle` 使用 |
| `third_party/mac-angle` | 角度传感器的 C++ 封装 |

可以先检查开发工具：

```bash
xcrun --find clang++
cmake --version
```

本项目已在一台型号标识为 `Mac15,6` 的 MacBook 上完成角度读取和实际音量写入验证。
这不代表所有 MacBook 都兼容；传感器访问情况和输出设备是否支持音量调节仍需在本机验证。

## 构建与快速开始

以下命令均在**项目根目录**执行。

### 1. 准备第三方库

仅在 `third_party/mac-angle` 尚不存在时执行：

```bash
mkdir -p third_party
git clone https://github.com/ufoym/mac-angle.git third_party/mac-angle
```

主项目使用 `add_subdirectory` 将这个库纳入构建，随后链接它提供的 `lid_angle` 目标。
无需再单独执行第三方 README 中的安装命令，也不需要把库安装到系统目录。
`BUILD_EXAMPLE` 关闭的只是第三方示例程序，库本身仍会按依赖关系编译。

### 2. 配置和编译

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

- `-S .` 指定源码根目录。
- `-B build` 指定构建目录，缺少时会自动创建。
- `Debug` 用于当前的调试开发。
- 第一条命令生成构建规则，第二条命令执行编译和链接。

默认也构建测试程序。只想编译主程序时，可以使用：

```bash
cmake --build build --target MacLidVolumeSlider
```

### 3. 先读取一次，确认传感器可用

```bash
./build/MacLidVolumeSlider --dry-run --once
```

输出示例（第三方库还会打印初始化信息）：

```text
预览模式：0° → 0%，133° → 100%。按 Ctrl+C 停止。
角度：114.0°，目标音量：85.7%
```

### 4. 持续控制音量

```bash
./build/MacLidVolumeSlider
```

缓慢改变屏幕开合角度，系统音量会跟随变化。按 **Ctrl+C** 停止。
如果只想连续观察映射而不修改音量，运行：

```bash
./build/MacLidVolumeSlider --dry-run
```

## 命令行参数

| 参数 | 行为 | 是否修改系统音量 |
| --- | --- | --- |
| 不带参数 | 持续读取并控制音量 | 是 |
| `--dry-run` | 持续预览角度和目标音量 | 否 |
| `--once` | 读取角度并设置一次音量，然后退出 | 是 |
| `--dry-run --once` | 只预览一次，然后退出 | 否 |
| `--help` | 显示用法，不初始化传感器 | 否 |

未知参数会报错并以状态码 `1` 退出。正常结束返回 `0`。

## 映射规则与边界

这里测量的是**屏幕与机身之间的开合角度**，不是整台电脑相对地面的倾斜角度。

当前下限按闭合位置 **0°** 计算，上限采用本机手动实测的 **133°**。
这些是控制范围，不是程序自动查询到的机械极限；库中的传感器数值范围也不等于铰链的可动范围。

```text
有效角度 = clamp(当前角度, 闭合角度, 最大角度)
目标音量 = (有效角度 - 闭合角度) / (最大角度 - 闭合角度)
```

| 开合角度 | 目标音量 |
| --- | --- |
| 0° | 0% |
| 66.5° | 50% |
| 99° | 约 74.4% |
| 133° | 100% |
| 大于 133° 的有效读数 | 100% |

控制范围定义在 [src/volume_mapping.h](src/volume_mapping.h)：

- `kClosedLidAngle`：闭合端点，当前为 `0.0`。
- `kMaximumLidAngle`：最大端点，当前为 `133.0`。

换一台电脑时，应通过预览模式观察它正常开到最大位置时的稳定读数，再调整上限并重新编译。
遇到铰链阻力时不要继续施力。当前没有命令行校准功能或配置文件。

映射函数会将超出控制范围的输入限制到边界，并拒绝非有限数值及非法边界。
主循环还会拒绝小于 0°、大于 360° 或非有限的传感器读数，避免把损坏的数据当作有效输入。
`360°` 只是当前读取层的检查上限，不代表本机可以打开到该角度。

## 控制流程

```text
解析参数，创建传感器
        ↓
读取当前角度并检查有效性
        ↓
计算 [0, 1] 范围的目标音量
        ↓
正式模式：查询当前默认输出设备
        ↓
首次读取、目标音量变化或输出设备切换？
        ├── 是：提交音量设置并输出信息
        └── 否：保持现状
        ↓
等待 100 毫秒，继续读取
```

预览模式只计算和输出，不查询或设置音频设备。
设备查询、传感器读取和音量设置有额外耗时，所以实际采样频率略低于每秒 10 次。

## 项目结构

```text
mac-lid-volume-slider/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp                 参数解析、监控循环和退出处理
│   ├── audio_volume.h           音量模块的函数声明
│   ├── audio_volume.cpp         Core Audio 设备查询和音量读写
│   ├── volume_mapping.h         映射接口和控制边界
│   ├── volume_mapping.cpp       角度到音量的纯计算
│   └── lid_monitor.cpp          保留的独立角度练习
├── tests/
│   ├── volume_mapping_test.cpp  映射规则测试
│   └── controller_backend.cpp   控制循环使用的模拟硬件
├── third_party/
│   └── mac-angle/               第三方传感器库
└── build/                       CMake 自动生成的构建产物
```

`src/lid_monitor.cpp` 没有加入最终目标，它自己的 `main()` 不会与主程序冲突。
`build/` 可以删除，源码与第三方库仍需保留。

音量模块提供三个操作：

| 函数 | 作用 |
| --- | --- |
| `getDefaultOutputDevice()` | 查询调用时的默认输出设备 ID |
| `getVolume(device)` | 读取指定设备的主输出音量，返回 `[0, 1]` 的数值 |
| `setVolume(device, volume)` | 将指定设备的主输出音量设置为传入值 |

模块通过异常报告失败，由入口统一捕获并输出信息。持续控制路径使用设备查询和音量设置；
读取音量函数保留用于验证或后续功能。

## 测试与验证

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

| 测试 | 覆盖内容 |
| --- | --- |
| `volume_mapping` | 端点、中点、越界限制、单调性、自定义边界和非有限输入 |
| `controller` | 用真实入口配合模拟硬件，验证首次设置、跳过重复目标、设备切换和信号退出 |
| `controller_preview` | 验证预览模式不调用音频设备查询和设置接口 |

这三项自动化测试不读取真实角度传感器，也不修改系统音量。
它们不能替代实际硬件验证。硬件读取可以使用 `--dry-run --once`，实际写入可以使用 `--once`。
本机曾验证：99° 对应约 74.4%，设置后重新读取系统音量为约 74.44%。

若配置时使用了 `-DBUILD_TESTING=OFF`，需要重新配置为 `-DBUILD_TESTING=ON` 后再构建和运行测试。

## 日常开发与 C++ / CMake 学习点

修改源码后重新构建、运行即可：

```bash
cmake --build build --target MacLidVolumeSlider
./build/MacLidVolumeSlider --dry-run --once
```

如果删除了 `build/`，重新执行“配置和编译”中的两条命令即可恢复。
修改头文件中的控制边界后，也必须重新编译才会生效。

本项目可以按下面的顺序阅读：

1. `volume_mapping.cpp`：无硬件依赖的函数、`std::clamp`、数值检查和异常。
2. `audio_volume.h/.cpp`：声明与定义分离，调用 C 接口，以及返回状态码与输出参数的区别。
3. `main.cpp`：对象生命周期、`std::optional`、定时读取和退出处理。
4. `CMakeLists.txt`：可执行目标、库目标、链接依赖和测试目标。
5. `third_party/mac-angle/angle.cpp`：设备发现、HID 报告读取以及资源释放。

几个与 Java、JS 等语言开发体验不同的地方：

- **声明与链接分开**：包含头文件让编译器理解函数调用，函数实现仍需作为源文件或库参与链接。
- **按地址写回结果**：Core Audio 的返回值是状态码，查询到的设备 ID 或音量写入传入地址所指向的变量。
- **内存大小是接口参数**：`sizeof(variable)` 是变量占用的字节数，`sizeof(&variable)` 则是指针大小。
- **局部对象负责资源生命周期**：传感器在循环外创建，正常退出或异常展开时析构，关闭设备并释放资源。
- **用 `std::optional` 表示“尚无上次结果”**：避免读取未初始化的局部数值。
- **信号处理只设置标记**：主循环检查标记后退出，资源释放和输出留在正常执行路径。

`MacLidVolumeSlider` 目标编译三个实现文件并链接 Core Audio 与 `lid_angle`。
头文件不会自动把同名 `.cpp` 加入构建，因此新增实现文件后需要更新 CMake。

## 常见问题

| 现象 | 检查方式 |
| --- | --- |
| 找不到 `third_party/mac-angle` | 按“准备第三方库”下载源码，确认其中存在 `CMakeLists.txt` |
| 找不到 `build/MacLidVolumeSlider` | 先配置并构建，确认终端位于项目根目录 |
| 只能看到目标音量，系统音量没变 | 检查是否带了 `--dry-run`；正式模式输出“已提交音量” |
| 报告找不到或无法打开传感器 | 运行 `--dry-run --once` 查看具体错误，核对设备与第三方库兼容性 |
| 提示设备没有主输出音量属性或不可写 | 当前输出设备不支持本程序使用的主音量控制；可切换到内置扬声器验证 |
| `Undefined symbols` 链接错误 | 检查实现文件是否加入目标，以及 Core Audio、`lid_angle` 是否链接 |
| 手动调音量后又被覆盖 | 下次目标音量变化或设备切换时会重新应用角度映射；可退出程序停止控制 |
| 测试可执行文件不存在 | 只构建主目标不会构建测试；先执行不带 `--target` 的 `cmake --build build` |

## 当前限制

- 这是前台命令行程序，没有菜单栏界面、后台服务或开机自启。
- `133°` 是当前机器的实测配置，不会自动探测其他电脑的最大开合角度。
- 主循环不主动重连传感器；传感器或音量操作失败时报告错误并退出，需要手动重启。
- 只支持设备提供的主输出音量属性，没有单独处理左右声道或其他音量控制方式。
- 程序不修改系统静音开关；音量百分比也不代表听感响度按相同比例变化。
- 系统可能异步应用音量，输出的是提交的目标值，不是设置后的即时回读值。
- 不保证传感器具有小数精度；输出的小数位只是显示格式。
- 合盖可能先触发系统睡眠，程序无法在睡眠期间继续采样，因而不能保证完全闭合前恰好设置到零。
  程序不阻止睡眠；唤醒后进程仍在运行且传感器读取正常时，循环会继续。

## 依赖与参考资料

- [ufoym/mac-angle](https://github.com/ufoym/mac-angle)：传感器读取库，使用 MIT 许可证，见 [上游 LICENSE](https://github.com/ufoym/mac-angle/blob/main/LICENSE) 和 [第三方声明](THIRD_PARTY_NOTICES.md)。
- [Apple Core Audio Overview](https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/CoreAudioOverview/WhatisCoreAudio/WhatisCoreAudio.html)：理解音频系统的概念；这是归档文档，具体接口以本机 SDK 为准。
- [Apple IOHIDManager](https://developer.apple.com/documentation/iokit/iohidmanager_h)：HID 设备管理接口。
- [CMake 官方教程](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)：学习目标、源码和库依赖的组织方式。

## 许可证

本项目采用 [MIT License](LICENSE)，Copyright (c) 2026 YueHuo-zh。

第三方库保留原作者的版权与许可证，完整声明收录在 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
发布包含该库的源码或编译后程序时，请一并附带本项目的 `LICENSE` 和第三方声明文件。
