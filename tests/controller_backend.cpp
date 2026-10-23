// 为真实 main.cpp 提供模拟传感器和音量接口，验证循环而不改变系统音量。
#include "angle.h"
#include "audio_volume.h"
#include <array>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
struct Frame
{
    double angle;
    AudioDeviceID device;
};

const std::array<Frame, 5> frames{{
    {0.0, 1}, {66.5, 1}, {66.5, 1}, {66.5, 2}, {133.0, 2}
}};
const std::array<float, 4> expectedVolumes{{0.0f, 0.5f, 0.5f, 1.0f}};
const std::array<AudioDeviceID, 4> expectedDevices{{1, 1, 2, 2}};
std::size_t reads = 0;
std::size_t writes = 0;
std::size_t deviceQueries = 0;

bool previewTest()
{
    return std::getenv("SLIDER_TEST_PREVIEW") != nullptr;
}
}

namespace MacBookLidAngle
{
class LidAngleSensor::Impl {};

LidAngleSensor::LidAngleSensor() : pImpl(std::make_unique<Impl>()) {}
LidAngleSensor::~LidAngleSensor() = default;

double LidAngleSensor::readAngle()
{
    if (reads == frames.size())
    {
        const std::size_t expectedWrites = previewTest() ? 0 : expectedVolumes.size();
        const std::size_t expectedQueries = previewTest() ? 0 : frames.size();
        if (writes != expectedWrites || deviceQueries != expectedQueries)
            throw std::runtime_error("循环中的设置次数或设备查询次数不正确");

        std::cout << "控制循环验证通过\n";
        std::raise(SIGINT);
        return frames.back().angle;
    }
    return frames[reads++].angle;
}
}

AudioDeviceID getDefaultOutputDevice()
{
    if (previewTest() || reads == 0 || reads > frames.size())
        throw std::runtime_error("不应在此时查询音频设备");
    ++deviceQueries;
    return frames[reads - 1].device;
}

void setVolume(AudioDeviceID device, Float32 volume)
{
    if (previewTest() || writes >= expectedVolumes.size() ||
        device != expectedDevices[writes] ||
        std::abs(volume - expectedVolumes[writes]) > 0.000001f)
    {
        throw std::runtime_error("音量设置的值、设备或时机不正确");
    }
    ++writes;
}
