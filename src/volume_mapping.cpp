#include "volume_mapping.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

float volumeFromAngle(double angle, double closedAngle, double maximumAngle)
{
    const double range = maximumAngle - closedAngle;
    if (!std::isfinite(angle) || !std::isfinite(closedAngle) ||
        !std::isfinite(maximumAngle) || !std::isfinite(range) || range <= 0.0)
    {
        throw std::invalid_argument("角度必须是有限数值，且最大角度必须大于闭合角度");
    }

    // 先限制角度，再做减法，避免异常大的输入造成溢出。
    const double boundedAngle = std::clamp(angle, closedAngle, maximumAngle);
    return static_cast<float>((boundedAngle - closedAngle) / range);
}
