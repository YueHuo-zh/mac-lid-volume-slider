#pragma once
#include <CoreAudio/CoreAudio.h>

// 以下注释约定了接口行为，audio_volume.cpp 中的实现应遵循这些约定。

/**
 * @brief 获取调用时系统默认的音频输出设备。
 * @return 有效的设备 ID；切换系统输出设备后，需要重新查询。
 * @throws std::runtime_error 查询失败或没有默认输出设备。
 */
AudioDeviceID getDefaultOutputDevice();

/**
 * @brief 设置指定设备的主输出音量。
 * @param device 要操作的音频设备 ID。
 * @param volume 目标音量，范围为 [0.0, 1.0]，例如 0.5 表示 50%。
 * @throws std::invalid_argument 音量不是有限数值或超出有效范围。
 * @throws std::runtime_error 设备不支持该属性、属性不可写或设置失败。
 * @note 此操作不修改静音状态；设置成功后，实际音量更新可能异步完成。
 */
void setVolume(AudioDeviceID device, Float32 volume);

/**
 * @brief 读取指定设备的主输出音量，不包含静音状态。
 * @param device 要查询的音频设备 ID。
 * @return 查询到的音量，范围为 [0.0, 1.0]。
 * @throws std::runtime_error 设备不支持该属性或读取失败。
 */
Float32 getVolume(AudioDeviceID device);
