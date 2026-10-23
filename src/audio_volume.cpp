#include "audio_volume.h"
#include <cmath>
#include <stdexcept>
#include <string>

AudioDeviceID getDefaultOutputDevice()
{
    AudioDeviceID device = kAudioObjectUnknown;
    UInt32 size = sizeof(device);
    AudioObjectPropertyAddress address{};
    address.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    address.mScope = kAudioObjectPropertyScopeGlobal;
    address.mElement = kAudioObjectPropertyElementMain;

    OSStatus status = AudioObjectGetPropertyData(
        kAudioObjectSystemObject,
        &address,
        0,
        nullptr,
        &size,
        &device);

    if (status != noErr)
    {
        throw std::runtime_error("读取默认输出设备失败，状态码：" + std::to_string(status));
    }

    if (device == kAudioObjectUnknown)
    {
        throw std::runtime_error("没有默认输出设备");
    }

    return device;
}

void setVolume(AudioDeviceID device, Float32 volume)
{
    if (!std::isfinite(volume) || volume < 0.0f || volume > 1.0f)
    {
        throw std::invalid_argument("音量必须是 [0.0, 1.0] 范围内的有限数值");
    }

    AudioObjectPropertyAddress address{};
    address.mSelector = kAudioDevicePropertyVolumeScalar;
    address.mScope = kAudioObjectPropertyScopeOutput;
    address.mElement = kAudioObjectPropertyElementMain;

    if (!AudioObjectHasProperty(device, &address))
    {
        throw std::runtime_error("该设备不提供主输出音量属性");
    }

    Boolean settable = false;

    OSStatus status = AudioObjectIsPropertySettable(
        device,
        &address,
        &settable);

    if (status != noErr)
    {
        throw std::runtime_error("检查音量是否可修改失败，状态码：" + std::to_string(status));
    }

    if (!settable)
    {
        throw std::runtime_error("该设备的音量属性不可修改");
    }

    status = AudioObjectSetPropertyData(
        device,
        &address,
        0,
        nullptr,
        sizeof(volume),
        &volume);

    if (status != noErr)
    {
        throw std::runtime_error("设置音量失败，状态码：" + std::to_string(status));
    }
}

Float32 getVolume(AudioDeviceID device)
{
    AudioObjectPropertyAddress address{};
    address.mSelector = kAudioDevicePropertyVolumeScalar;
    address.mScope = kAudioObjectPropertyScopeOutput;
    address.mElement = kAudioObjectPropertyElementMain;

    if (!AudioObjectHasProperty(device, &address))
    {
        throw std::runtime_error("该设备不提供主输出音量属性");
    }

    Float32 volume = 0.0f;
    UInt32 size = sizeof(volume);

    OSStatus status = AudioObjectGetPropertyData(
        device,
        &address,
        0,
        nullptr,
        &size,
        &volume);

    if (status != noErr)
    {
        throw std::runtime_error("读取音量失败，状态码：" + std::to_string(status));
    }

    return volume;
}
