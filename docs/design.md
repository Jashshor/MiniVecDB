# MiniVecDB 架构设计

## 持久化设计 (Save / Load)
计划提供 `storage::Serializer` 模块处理落盘。
文件 Layout 规划：
- [8 bytes] Magic Word (`MVDB_V1`)
- [8 bytes] Dimension
- [8 bytes] N (Total vectors)
- [N * 8 bytes] IDs array
- [N * dim * 4 bytes] Flattened Raw Float Data