#pragma once

// 闭合位置按 0° 计算；133° 是这台 MacBook 实测的最大开合角度。
inline constexpr double kClosedLidAngle = 0.0;
inline constexpr double kMaximumLidAngle = 133.0;

// 将角度线性映射到 [0, 1]；超出控制范围时取边界值。
// 非有限角度、非有限边界或上限不大于下限时抛出 std::invalid_argument。
float volumeFromAngle(double angle,
                      double closedAngle = kClosedLidAngle,
                      double maximumAngle = kMaximumLidAngle);
