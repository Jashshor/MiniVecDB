明白了！你希望保留最开始那个**极具执行感、按优先级和时间线划分的原版结构**，同时把我们今天实际踩坑、升级的工程点（Bazel、C++17、GTest、大厂格式化规范、标准 Git 流水线）天衣无缝地揉进去。

这就为你生成一份**完美对标原版格式、但内核已全面升级为大厂标准**的 Day 1 记录清单：

---

## Part 1｜组长给你的 Day 1 任务清单（今天必须完成）

> **Day 1 目标**：把工程从 0 → 1 跑起来，形成“可编译、可测试、可提交”的最小闭环。
> 今天不追求功能多，追求工程规范和可持续迭代（全面拥抱 Bazel + C++17）。

✅ **P0（必须完成，完成即合格）**

* **仓库初始化与工程骨架**
* 建立目录结构：`src/` `tests/` `docs/` `tools/`
* 初始化 `.gitignore`（过滤 `bazel-*`, `.vscode`, `.cache` 等）
* 写一个最小 `README.md`：说明环境依赖（Bazelisk）、如何 build / run / test


* **Bazel 构建跑通（替代原定 CMake，适配大厂基建）**
* 顶层配置支持：`.bazelversion` (锁定 7.4.0), `MODULE.bazel` (引入 Bzlmod 管理依赖), `.bazelrc` (强制 C++17)
* `bazel build //...` 能够全局编译生效
* 输出一个可执行文件：`vecdb_cli`（打印 boot 信息即可）


* **接入单元测试框架 + 1 个测试用例**
* 使用 GoogleTest（通过 Bazel `bazel_dep` 极简引入）
* 写一个测试：`distance_l2`（使用 C++17 的 `const float*` 和 `size_t dim` 传参，抛弃 `std::span`）
* `bazel test //...` 一条命令全绿


* **代码规范与工具链（对标腾讯/大厂配置）**
* 开启编译警告：`.bazelrc` 中配置 `-Wall -Wextra -Wpedantic`
* 配置 `.clang-format`：100 列宽，2 空格缩进，指针靠左对齐
* 配置 VS Code `.vscode/settings.json`：实现 `Ctrl+S` 保存即自动格式化


* **CI（可选加分项 → 已完成）**
* GitHub Actions (`.github/workflows/bazel.yml`)：跑 build + test，带缓存优化



✅ **P1（建议完成，完成更像大厂节奏）**

* **写基础工具类 (`src/utils/`)**
* `Timer`：测函数耗时（ns/us/ms）
* `Logging`：简单输出宏（先 stdout 即可）


* **写“接口草稿”但不实现功能**
* `VectorStore` 头文件：`add/get/size/dim`（明确一维大数组扁平化存储 + `unordered_map` 的内存布局思路）
* `IIndex` 接口：`add/search/build`
* 不要求实现，要求接口清晰、配好 `BUILD.bazel`、可编译



✅ **P2（加分项，有时间再做）**

* **工程文档占位**
* `docs/design.md` 写目录标题
* `docs/benchmark.md` 写计划



---

## Part 2｜助手给你的“可直接复制”的今日执行版 Checklist（按小时推进）

**0）启动（10 分钟）**

* [x] 初始化 Git 并绑定 SSH 远端仓库：`git init` -> `git remote add origin git@github.com:...`
* [x] 空提交锚定主分支：`git commit --allow-empty -m "chore: init empty repository"`
* [x] 创建并切换开发分支：`git checkout -b day1-bootstrap`
* [x] 建目录：`src` `tests` `docs` `tools`

**1）工程跑起来（40–60 分钟）**

* [x] 配置 Bazel 引擎：新建 `.bazelversion`, `MODULE.bazel`, `.bazelrc`
* [x] 编写 `src/BUILD.bazel` 及 `src/main.cpp`：打印 `"[INFO] MiniVecDB boot ok."`
* [x] 本地编译与运行成功：`bazel run //src:vecdb_cli`

**2）接入测试与代码规范（40–60 分钟）**

* [x] 配置规范化引擎：根目录新建 `.clang-format` 和 `.vscode/settings.json`
* [x] 加 GoogleTest：在 `tests/BUILD.bazel` 配置测试目标
* [x] 新建 `tests/test_distance.cpp`：写 L2 距离的 GTest 断言测试

**3）基础模块骨架（30–50 分钟）**

* [x] `src/metric/distance.h/.cpp`：基于 C++17 裸指针实现 L2 计算，并配置独立 `BUILD.bazel`
* [x] `src/storage/vector_store.h`：只写 class 声明，体现扁平化内存思想
* [x] `src/index/iindex.h`：只写虚函数接口声明
* [x] `src/utils/`：完成 `timer.h` 和 `logging.h`

**4）规范化与提交流水线（20–30 分钟）**

* [x] 完善 `.gitignore` 和 `README.md`
* [x] 配置 GitHub CI：创建 `.github/workflows/bazel.yml`
* [x] 终极本地校验：`bazel build //...` + `bazel test //...` 全绿
* [x] **规范提交流程**：
* `git commit -m "chore: bootstrap project with bazel, gtest and c++17"`
* `git checkout main` -> `git merge day1-bootstrap` -> `git push origin main`
* `git checkout -b day2-vectorstore` (为明天清理战场)



---

## 今日验收（你发我这个就算 Day 1 过）

你今天结束时应当满足：

* `bazel build //...` 编译成功 ✅
* `bazel test //...` 全绿 ✅
* 运行 `bazel run //src:vecdb_cli` 输出 boot 信息 ✅
* GitHub 有清晰的 1 次 Commit，CI 流水线跑通，且本地终端停留在干净的 `day2-vectorstore` 分支 ✅