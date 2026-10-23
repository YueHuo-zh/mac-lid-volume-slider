#include "audio_volume.h"
#include "angle.h"
#include "volume_mapping.h"
#include <chrono>
#include <cmath>
#include <csignal>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
volatile std::sig_atomic_t stopRequested = 0;

void requestStop(int)
{
    // 信号处理函数只设置标记，资源由正常退出路径释放。
    stopRequested = 1;
}
}

int main(int argc, char *argv[])
{
    try
    {
        bool dryRun = false;
        bool once = false;
        for (int i = 1; i < argc; ++i)
        {
            const std::string argument = argv[i];
            if (argument == "--dry-run")
                dryRun = true;
            else if (argument == "--once")
                once = true;
            else if (argument == "--help")
            {
                std::cout << "用法：MacLidVolumeSlider [--dry-run] [--once]\n"
                          << "  --dry-run  只显示角度和目标音量，不修改系统音量\n"
                          << "  --once     只读取并处理一次，然后退出\n"
                          << "默认每 100 毫秒读取一次，按 Ctrl+C 停止。\n";
                return 0;
            }
            else
            {
                throw std::invalid_argument("未知参数：" + argument);
            }
        }

        std::signal(SIGINT, requestStop);
        std::signal(SIGTERM, requestStop);

        MacBookLidAngle::LidAngleSensor sensor;
        std::optional<float> lastTargetVolume;
        AudioDeviceID lastDevice = kAudioObjectUnknown;

        std::cout << (dryRun ? "预览模式" : "音量控制模式")
                  << "：" << kClosedLidAngle << "° → 0%，"
                  << kMaximumLidAngle << "° → 100%。按 Ctrl+C 停止。\n";

        while (!stopRequested)
        {
            const double angle = sensor.readAngle();
            // 这是传感器报告的有效范围，不是屏幕的机械开合范围。
            if (!std::isfinite(angle) || angle < 0.0 || angle > 360.0)
                throw std::runtime_error("传感器返回了无效角度");

            const float targetVolume = volumeFromAngle(angle);
            // 每轮查询默认设备，即使角度未变，也能跟随输出设备切换。
            const AudioDeviceID device = dryRun ? kAudioObjectUnknown : getDefaultOutputDevice();
            if (stopRequested)
                break;

            if (!lastTargetVolume || targetVolume != *lastTargetVolume || device != lastDevice)
            {
                if (!dryRun)
                    setVolume(device, targetVolume);

                std::cout << std::fixed << std::setprecision(1)
                          << "角度：" << angle << "°，"
                          << (dryRun ? "目标音量：" : "已提交音量：")
                          << targetVolume * 100.0f << "%";
                if (!dryRun)
                    std::cout << "，设备 ID：" << device;
                std::cout << std::endl;

                // 只在成功处理后记录，避免把失败当成已完成。
                lastTargetVolume = targetVolume;
                lastDevice = device;
            }

            if (once)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception &error)
    {
        std::cerr << "程序停止：" << error.what() << '\n';
        return 1;
    }

    return 0;
}
