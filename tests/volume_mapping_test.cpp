#include "volume_mapping.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void expectVolume(double angle, float expected)
{
    if (std::abs(volumeFromAngle(angle) - expected) > 0.000001f)
        throw std::runtime_error("角度映射结果不符合预期");
}

template <typename Function>
void expectInvalidArgument(Function operation)
{
    try
    {
        operation();
    }
    catch (const std::invalid_argument &)
    {
        return;
    }
    throw std::runtime_error("非法输入未被拒绝");
}

int main()
{
    try
    {
        expectVolume(0.0, 0.0f);
        expectVolume(66.5, 0.5f);
        expectVolume(133.0, 1.0f);
        expectVolume(-1.0, 0.0f);
        expectVolume(134.0, 1.0f);
        expectVolume(std::numeric_limits<double>::max(), 1.0f);

        float previous = 0.0f;
        for (int angle = 0; angle <= 133; ++angle)
        {
            const float volume = volumeFromAngle(angle);
            if (volume < previous || volume < 0.0f || volume > 1.0f)
                throw std::runtime_error("音量应随角度单调增加且处于有效范围");
            previous = volume;
        }

        if (std::abs(volumeFromAngle(70.0, 10.0, 130.0) - 0.5f) > 0.000001f)
            throw std::runtime_error("自定义边界映射错误");

        expectInvalidArgument([] { volumeFromAngle(30.0, 0.0, 0.0); });
        expectInvalidArgument([] { volumeFromAngle(30.0, 100.0, 0.0); });
        expectInvalidArgument([] { volumeFromAngle(std::numeric_limits<double>::quiet_NaN()); });
        expectInvalidArgument([] { volumeFromAngle(std::numeric_limits<double>::infinity()); });
        expectInvalidArgument([] { volumeFromAngle(30.0, std::numeric_limits<double>::quiet_NaN(), 133.0); });
        expectInvalidArgument([] { volumeFromAngle(30.0, 0.0, std::numeric_limits<double>::infinity()); });
        std::cout << "角度映射测试通过\n";
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
