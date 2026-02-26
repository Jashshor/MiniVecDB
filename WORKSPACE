workspace(name = "minivecdb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# 拉取 GTest 1.14.0
http_archive(
    name = "googletest",
    urls = ["https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz"],
    strip_prefix = "googletest-1.14.0",
)