# MiniVecDB

一个基于 C++17 和 Bazel 构建的单机向量检索引擎（Vector Search Engine）核心实现。本项目以“可演示、可测、可扩展”为工程标准，涵盖了底层连续内存的向量存储、距离计算、HNSW 索引闭环以及数据持久化。

## 环境要求 (Prerequisites)

本项目在开发和测试时对齐了现代 C++ 后端工程的基础标准：
- **操作系统**: Ubuntu 24.04 LTS
- **语言标准**: C++17 
- **编译器**: GCC 13 / 14 (Ubuntu 24.04 默认自带)
- **构建系统**: Bazel 7.x (启用 Bzlmod)
- **代码规范**: `clang-format` (定制化 Google C++ 风格)

## 环境准备 (Setup)

### 1. 安装 Bazel 构建工具 (通过 Bazelisk)
为避免全局依赖污染并锁定项目编译版本（本项目配置在 `.bazelversion` 中为 `7.4.0`），请勿直接 `apt install bazel`，而是使用官方的 Bazelisk：

```bash
# 下载 Bazelisk 并放入系统 PATH
sudo wget -O /usr/local/bin/bazel [https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64](https://github.com/bazelbuild/bazelisk/releases/latest/download/bazelisk-linux-amd64)
sudo chmod +x /usr/local/bin/bazel

# 验证安装（首次执行会自动在后台下载 Bazel 7.4.0）
bazel --version
```

### 2. 安装代码格式化工具

本项目根目录包含 `.clang-format` 文件，定义了缩进（2 空格）、列宽（100 字符）等规范。请在 Ubuntu 中安装格式化核心程序：

Bash

```
sudo apt update
sudo apt install clang-format
```

### 3. 编辑器配置 (VS Code)

如果你使用 VS Code 进行开发，建议安装官方的 **`C/C++`** 插件，并在工程根目录下创建 `.vscode/settings.json`，填入以下配置以开启“保存自动格式化”：

JSON

```
{
    "[cpp]": {
        "editor.defaultFormatter": "ms-vscode.cpptools",
        "editor.formatOnSave": true
    },
    "[c]": {
        "editor.defaultFormatter": "ms-vscode.cpptools",
        "editor.formatOnSave": true
    },
    "C_Cpp.formatting": "Default",
    "C_Cpp.clang_format_fallbackStyle": "none",
    "C_Cpp.clang_format_style": "file"
}
```

## 编译与测试 (Build & Test)

本项目采用 Bazel 最新的 **Bzlmod** 包管理（由 `MODULE.bazel` 驱动），首次构建时会自动从镜像库拉取并编译 `GoogleTest` 等第三方依赖，无需手动干预。

Bash

```
# 1. 编译全部模块
bazel build //...

# 2. 运行所有单元测试 (测试失败时打印详细日志)
bazel test //... --test_output=errors

# 3. 运行 CLI 主程序
bazel run //src:vecdb_cli
```

## 工程目录结构 (Structure)
```
MiniVecDB/
├── MODULE.bazel          # Bazel 7 现代依赖管理配置
├── .bazelversion         # 锁定 Bazel 版本
├── .clang-format         # C++ 代码格式化规范
├── src/                  # 核心源码
│   ├── main.cpp          # CLI / Server 入口
│   ├── metric/           # 距离计算模块 (L2, Cosine)
│   ├── storage/          # 向量数据与连续内存管理 (VectorStore)
│   ├── index/            # 检索索引接口与实现 (Flat, HNSW)
│   └── utils/            # 基础工具类 (Timer, Logging)
├── tests/                # 基于 GTest 的单元测试集
└── docs/                 # 架构设计文档与 Benchmark 报告
```