workspace(name = "minivecdb")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# 拉取 GTest 1.14.0
http_archive(
    name = "googletest",
    urls = ["https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz"],
    strip_prefix = "googletest-1.14.0",
)

# Hedron's Compile Commands Extractor for Bazel
# https://github.com/hedronvision/bazel-compile-commands-extractor
http_archive(
    name = "hedron_compile_commands",
    url = "https://github.com/hedronvision/bazel-compile-commands-extractor/archive/1e08f8e0507b6b6b1f4416a9a22cf5c28beaba93.tar.gz",
    strip_prefix = "bazel-compile-commands-extractor-1e08f8e0507b6b6b1f4416a9a22cf5c28beaba93",
)
load("@hedron_compile_commands//:workspace_setup.bzl", "hedron_compile_commands_setup")
hedron_compile_commands_setup()