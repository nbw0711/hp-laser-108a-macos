# 来源、许可证和本地修改

取样日期：2026-10-30（Asia/Shanghai）。
下载地址：https://codeload.github.com/paviladtudk/hp-laser-107a-macos/tar.gz/refs/heads/main
原始压缩包保留为 `vendor/hp107a-upstream.tar.gz`，SHA-256：
`94e5e827e5f13ce2351f37ddb65beac5e0a9d92f0b221050795c6e8dd13e44f6`。
该地址的 main 分支可变；复现本次版本应使用保留压缩包和哈希，不要依赖未来的 main。
逐文件原始哈希见 `vendor/upstream-sha256.json`。

- SpliX：`vendor/splix`，GPL-2.0；版权声明保留在源码，许可证见根目录 LICENSE 和 vendor/splix/COPYING。
- USB 后端：改写自 hp107a-usb.c，其声明源自 Kuberwastaken/hp-laser-1008a-macos 的 MIT 代码。MIT 文本与版权保留于 vendor/LICENSE-usb-MIT；本地衍生文件为 src/hp108a-usb.c。
- PPD、mkraster.c、splcheck.py、测试 PDF：取自上述 107a 项目，其未单独标识部分采用 GPL-2.0。
- 本地脚本、文档和集成测试按根目录 GPL-2.0 发布。HP 与 Samsung 是各自权利人的商标。

本地修改：

1. PPD 改为 HP108a 路径和名称，初始适配只提供 600dpi、单面和九种常用纸张，移除不适用的 A3 等选项。
2. USB 后端仅匹配原始 Windows 包给出的 03f0:e82a，多设备时拒绝发送；按流读取输入；禁用上游的网络守护进程、系统进程终止、设备复位与失败后重发逻辑；增加 alternate-setting 失败检查。
3. SpliX printer.cpp 的 HP 特殊路径清理 PJL 元数据中的控制符、引号和反斜杠，移除每次作业写入 DEFAULT SERVICEDATE。
4. 测试生成器改用 xorshift 产生可区分的密集噪声行，并显式模拟渲染器写入 NumCopies；检查器允许稀疏样本对齐。新增 Python 测试覆盖协议解码、像素比对和真实 macOS PDF 渲染链。
5. 新增本地/Universal 2 构建、安装、卸载和诊断脚本。原始补丁 vendor/splix/hp107a-pjl.patch 仅保留作上游溯源，不再应用它。

Windows EXE、提取的 DLL/INF/XML 不包含在本驱动发行包中。未反编译 DLL，也未复制其执行代码。
