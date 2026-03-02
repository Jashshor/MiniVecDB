#pragma once
#include <iostream>

// 轻量日志宏：用于 CLI/测试阶段输出关键运行信息。
// 后续可替换为更完整的日志库（等级、时间戳、文件落盘等）。
#define LOG_INFO(msg)   std::cout << " [INFO] " << msg << "\n"
#define LOG_ERROR(msg)  std::cerr << " [ERROR] " << msg << "\n"
