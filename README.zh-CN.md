# HP Laser 108a macOS 驱动

[English](README.md) | 中文

基于 SpliX 的原生 CUPS 驱动，支持从 macOS 应用的打印对话框发送黑白页面。
这是开源协议实现的适配版本，不是将 Windows DLL 转换成 macOS 文件。
当前默认 600 dpi、A4、单面；另提供 Letter、Legal、A5、A6、B5、Executive、Folio、Oficio。
已在 M3 Max / macOS 27.0.1 / HP Laser 108a 上完成真实出纸、三页文档和连续作业验证，用户确认所有测试页内容正确。当前可日常试用；仍属于未经签名的适配版本，测试范围见 VALIDATION.md。

## 结构

应用 PDF → macOS `cgpdftoraster` → `rastertoqpdl` → `hp108a` USB 后端 → 打印机。
编码器和 USB 后端都编译为原生程序，无需 Windows 虚拟机或 Ghostscript。
USB 标识取自随附 Windows 驱动的 HPRestStub.inf：`03f0:e82a`，覆盖 103/107/108 系列。
当前仅支持连接一台该系列打印机；多台同型号连接会拒绝发送，避免选错设备。

## 编译与离线测试

在本目录运行，需已安装 Apple Command Line Tools，以及 Python 3（仅用于测试）：

```sh
./build.sh
python3 tests/run.py
```

默认编译本机架构；Universal 2 编译：

```sh
ARCHS='arm64 x86_64' ./build.sh
```

本地验证环境、测试结果见 `VALIDATION.md`。编译和离线测试不会安装系统驱动或向打印机发送数据。
PPD 校验时跳过尚未安装的过滤器路径和系统所有者检查，其他 PPD 一致性检查照常进行。

## 安装和出纸验证

连接 USB、打开打印机电源，并确保虚拟机没有占用此 USB 设备。运行：

```sh
./install.sh
lp -d HP_Laser_108a tests/testpage.pdf
lp -d HP_Laser_108a tests/three-pages.pdf
```

安装脚本会重新编译，使用 sudo 写入以下位置并建立独立队列 `HP_Laser_108a`：

- `/Library/Printers/HP108a/rastertoqpdl`
- `/Library/Printers/HP108a/hp108a.ppd`
- `/usr/libexec/cups/backend/hp108a`

脚本不会改动默认打印机。若系统不允许安装 CUPS 后端，会报错停止；当前未实现后台服务回退。
安装后的系统权限可用 `cupstestppd /Library/Printers/HP108a/hp108a.ppd` 验证。

需要确认：单页、连续五次作业、三页文档、两份打印、睡眠唤醒后打印，以及重插 USB 后打印。
先检查 A4 出纸、缩放、方向、文字边缘和页边距；通过后再测试其他纸张。
打印机可能需要预热，USB 字节传输成功不等于纸张实际完成输出。

## USB 被 macOS 占用时

先运行 `./diagnose.sh`，必要时执行 `sudo ./build/hp108a --probe`。
诊断可能包含设备序列号，分享前可以删除序列号。
同系列上游报告 macOS 的 IPP-over-USB 桥可能占用接口。后端会选择经典打印接口，
但不会强行终止系统进程、重置设备或重复发送部分已传输的作业。

若确认阻塞来自该设备的桥，用下面命令查看系统标签：

```sh
sudo launchctl list | grep -i 'com.apple.print.ippusb.*HP.*Laser'
```

仅在确认标签对应当前 108a 时，调用随附脚本，完整标签必须用引号包围：

```sh
sudo bash tools/prepare-usb.sh '这里替换为完整的 com.apple.print.ippusb.… 标签'
sudo cupsenable HP_Laser_108a
```

该命令持续停用并停止此设备的 IPP-USB 桥，记录恢复标签，打印由经典 USB 接口接管。
要恢复记录的变更：

```sh
sudo bash tools/restore-usb.sh
```

然后重新插拔 USB。卸载脚本也会恢复记录的变更。
不要禁用其他打印机的标签。设备繁忙时也需检查虚拟机占用。
修复原因后，可在打印机设置中恢复暂停的队列；检查实际出纸情况后再恢复，避免重复打印。

## 卸载

先完成或取消本队列作业，再执行：

```sh
./uninstall.sh
```

卸载会自动恢复本驱动 tools/prepare-usb.sh 记录的 IPP-USB 桥禁用项，然后重新插拔 USB。未由本驱动记录的手动变更，仍需按上文恢复。安装过程中途失败时也可以运行卸载脚本清理已写入的驱动文件。

## 范围与来源

当前没有实现扫描、耗材状态、网络发现、AirPrint、自动双面、签名安装包或 DriverKit 扩展。
驱动使用现有 macOS CUPS/PPD 接口；未来系统兼容性需要分别验证。
Windows 包只作为协议和型号证据，不需要安装 Windows DLL。

`vendor/splix`、PPD 和测试工具来自 [paviladtudk/hp-laser-107a-macos](https://github.com/paviladtudk/hp-laser-107a-macos)，
其编码器来自 [OpenPrinting SpliX](https://github.com/OpenPrinting/splix)。
上游主要报告 107a 测试；本适配已补充 108a 的实机测试。
来源、哈希、本地修改和许可证见 `THIRD_PARTY.md`。编码器与衍生适配遵循 GPL-2.0，USB 衍生代码保留 MIT 许可。
