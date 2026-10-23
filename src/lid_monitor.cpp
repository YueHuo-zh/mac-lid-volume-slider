#include "angle.h"
#include <exception>
#include <iostream>
#include <chrono> // 表示时间长度
#include <thread> // 让当前线程等待

int main()
{
    std::cout << "开始监控屏幕开合角度\n";

    try
    {
        MacBookLidAngle::LidAngleSensor sensor;
        double lastAngle = -1.0;

        while (true)
        {
            double currentAngle = sensor.readAngle();

            if (currentAngle != lastAngle)
            {
                lastAngle = currentAngle;
                std::cout << "当前笔记本盖子角度为："
                          << currentAngle << "°\n";
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(100));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "读取失败：" << e.what() << '\n';
        return 1;
    }

    return 0;
}
