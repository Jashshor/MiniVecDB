#pragma once
#include <chrono>

namespace minivecdb {
    namespace utils {
        // 轻量计时器：创建对象后立即开始计时。
        // 常见用法：在一段代码前后读取耗时。
        class Timer {
        public:
            // 构造时调用 reset，记录起始时间点。
            Timer() { reset(); }

            // 重新开始计时，常用于复用同一个计时器对象。
            void reset() { start_ = std::chrono::high_resolution_clock::now(); }

            // 返回从 start_ 到当前时刻的毫秒数。
            // const 表示该函数不会修改成员变量。
            double elapsed_ms() const {
                auto now = std::chrono::high_resolution_clock::now();
                // duration<double, std::milli> 表示“毫秒单位的浮点时长”。
                return std::chrono::duration<double, std::milli>(now - start_).count();
            }

        private:
            // time_point 表示时间轴上的一个时刻，这里保存起始时刻。
            std::chrono::time_point<std::chrono::high_resolution_clock> start_;
        };
    }
}
