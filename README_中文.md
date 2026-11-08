# Kindle《黄金英雄坛说》KPW1 移植工程 v0.6

> v0.6 已能通过 GitHub Actions 构建 **无 ROM 的 KUAL 可安装测试包**（包含 ARM 游戏程序与墨水屏/触摸测试入口），但暂时不能直接游玩《黄金英雄坛说》。游戏 ROM/Flash 镜像不随项目提供。

## v0.6 安装与首次测试

1. 打开仓库的 Actions → Build Golden Hero for Kindle PW1，找到最近一次成功的运行记录。
2. 从 Artifacts 下载文件名包含 `v0.6-rom-free` 的压缩包。GitHub 的 Artifacts ZIP 里面还有一个真正的 Kindle 安装 ZIP，请先解压外层。
3. 将 `Kindle-Golden-Hero-KPW1-v0.6-ROM-free.zip` 解压到 KPW1 的 USB 根目录。
4. 安全弹出 Kindle，打开 KUAL → Golden Hero → **屏幕及触摸测试（无需 ROM，40 秒自动退出）**。
5. 检查 TOP 是否显示在屏幕上方、灰度条方向是否正常；触摸屏幕与 EXIT。报告写在 `golden_hero/logs/display-test.log`。
6. 画面测试无需 ROM；**请不要直接进入“启动游戏”**，它会要求镜像文件。测试不会修改 WordScreen。

测试会在最长 40 秒后退出，并在正常退出时尝试恢复进入测试前的 framebuffer 画面。若驱动不支持当前刷新调用，请报告日志；实机测试仍属于实验性质。

## 游戏镜像与存档

正式启动需要你有权使用的 `roms/obj_lu.bin`（24 MiB）和 `roms/nc1020.fls`（1 MiB）。
它们不能通过本项目自动下载或分发。游戏首次运行时会将初始 Flash 复制到 `data/nc1020.fls`，以免覆盖你的原始镜像。
v0.6 在上游源码中加入了临时文件写入后重命名的存档保护，并在读取存档时检查大小与版本。保存仍由文件系统具体行为决定，请定期备份 `data/`。

## 已确认的设备环境（来自两份实机只读报告）

- 设备：Kindle Paperwhite 1，MX50 YOSHIME
- 固件：5.6.1.1
- 内核：Linux 2.6.31-rt11-lab126，32 位 ARMv7
- CPU：ARMv7，Cortex-A8 系列
- 内存：256 MB
- Framebuffer：`mxc_epdc_fb`，`/dev/fb0`
- 实际屏幕：758×1024
- 虚拟缓冲区：768×6144
- 行跨度：768 bytes
- 像素格式：8 bpp，`grayscale=1`（8-bit grayscale）
- framebuffer 内存长度：4,718,592 bytes（4.5 MiB）
- `xoffset/yoffset`：0 / 0
- `rotate`：3
- 触摸设备：`cyttsp`，`/dev/input/event0`
- 触摸 X：0–758；触摸 Y：0–1024
- 动态加载器：`/lib/ld-linux.so.3`
- 系统 libc：glibc 2.12.1
- C++ 运行库：libstdc++.so.6.0.14

实测 framebuffer 和触摸轴均使用 758×1024 对应坐标。当前默认不交换 X/Y，也不翻转坐标。`rotate=3` 是驱动上报的旋转状态；由于 ioctl 的实际 xres/yres 与触摸轴范围一致，程序按 ioctl 坐标直接绘制，不再额外旋转。最终方向仍需第一次显示测试确认。

## KPW1 专用编译目标

使用 KOReader 官方 `koxtoolchain` 的 `kindle5` 目标：

- 支持 Kindle 4、Kindle Touch、Kindle PW1
- 工具链前缀：`arm-kindle5-linux-gnueabi`
- 编译器：`arm-kindle5-linux-gnueabi-g++`
- 目标 ABI：ARM EABI / GNU EABI（soft-float ABI）
- 预期动态加载器：`/lib/ld-linux.so.3`

**不要使用** `kindlepw2` 或 `kindlehf` 工具链。v0.3 文档中“设备 glibc 2.5”的描述不准确，已在本版更正：本机报告明确显示 glibc 2.12.1。交叉工具链的兼容基线与设备当前 libc 版本不是同一个概念。

官方资料：
- https://github.com/koreader/koxtoolchain
- https://github.com/koreader/koxtoolchain/releases/tag/2026.08

2026.08 预编译 `kindle5.tar.zst` 约 95.3 MB，解压后约 420 MB。`scripts/bootstrap-kindle5-toolchain.sh` 会校验 SHA-256 后安装到 `$HOME/x-tools/arm-kindle5-linux-gnueabi/`。

## 为什么选择 NC1020

采用 https://github.com/Wang-Yue/NC1020 的黄金英雄坛说模拟器核心。上游 README 明确说明该项目可运行《黄金英雄坛说》，并且 SDL2 是其桌面端唯一依赖。Kindle 端不使用 SDL2，而保留模拟器核心，替换显示和输入层。

上游核心接口包括：
- `Initialize`
- `LoadNC1020`
- `RunTimeSlice`
- `SetKey`
- `CopyLcdBuffer`
- `SaveNC1020`

`upstream/` 由 `scripts/fetch_upstream.py` 从固定提交下载并校验 Git blob SHA。该脚本只下载源码和许可证，不下载 ROM/固件二进制。校验通过后会修正上游初始化 ROM volume 2 时重复写入 `rom_volume1` 的一处指针笔误，否则第三个 ROM volume 会保持未初始化。

## v0.5 更新

- 已读取第二份实机探针报告，Framebuffer 实际分辨率和触摸轴范围均已确认。
- 适配代码记录实测 profile：758×1024、8 bpp、stride 768、virtual 768×6144、smem_len 4,718,592、rotate 3。
- 为 MXCFB 更新结构增加 16-byte rect / 72-byte update ABI 编译期检查。
- 默认触摸映射使用 X=0–758、Y=0–1024，不交换轴、不翻转。
- 触摸操作区增加完整按键：方向、FLY、MENU、OK、BACK、MAIN、GAME、X、Y、PGUP、PGDN、SAVE、EXIT。
- 修复 GOLDEN HERO 标题及触摸按键字库缺失的 C/H 字母。
- KUAL 启动脚本不再依赖旧 Kindle shell 缺失的 `command -v`。
- 屏保抑制仅在同时找到 LIPC get/set 且成功读取原属性值为 0/1 时才执行；退出时恢复原值，不猜测原状态。
- 本机 host mock 编译和工程脚本检查通过；仍未完成 KPW1 ARM ELF 与实机显示测试。

## 工程目录

- `kindle/main.cpp`：模拟器主循环、画面布局、触摸按键、保存退出
- `kindle/fb.cpp` / `fb.h`：`/dev/fb0` framebuffer 与 MXCFB 刷新
- `kindle/touch.cpp` / `touch.h`：Linux input 触摸事件
- `scripts/fetch_upstream.py`：固定提交、校验并只取开源模拟器源码；应用一处 ROM volume 2 指针修正
- `scripts/bootstrap-kindle5-toolchain.sh`：安装 PW1 专用编译器
- `scripts/build-armel.sh`：交叉编译并检查 ELF ABI / 动态加载器
- `scripts/package-kual.sh`：ROM 和 ARM 程序齐备后才生成正式游戏安装 ZIP
- `kual/device-probe/`：v0.4 framebuffer/触摸 ioctl 探针（参数已实测）
- `tests/mock_nc1020.cpp`：只用于桌面编译/链接检查，不含真实游戏逻辑

## 硬件探针状态

已完成。用户提供的 `fb-input-probe.txt` 已确认 framebuffer 758×1024、stride 768、8 bpp、smem_len 4,718,592，以及触摸 X=0–758 / Y=0–1024。无需再次安装探针。

## 交叉编译

在 Linux 或 WSL2 中：

```sh
./scripts/bootstrap-kindle5-toolchain.sh
./scripts/build-armel.sh
```

编译输出：`build-armel/golden_hero`。

脚本检查：
- ELF Machine 为 ARM
- 不使用 hard-float 参数传递
- ELF interpreter 为 `/lib/ld-linux.so.3`
- 不链接 SDL

这只是构建验证，不等于已经在 KPW1 实机运行成功。


触摸按键布局与 NC1020 原始键值对应关系见 [`docs/TOUCH-CONTROLS.md`](docs/TOUCH-CONTROLS.md)。

## 游戏数据

模拟器需要两份镜像：

- `roms/obj_lu.bin`：25,165,824 bytes（24 MiB）
- `roms/nc1020.fls`：1,048,576 bytes（1 MiB）

NC1020 上游仓库中公开列出了这两个文件，但上游 BSD-3-Clause 源码许可证不自动授予 ROM/固件镜像的再分发权。本工程不镜像、不打包这些文件。请仅使用你有权使用的镜像。上游 README 也提醒，其作者找到的 ROM 可能存在游戏物品保存问题。

## 正式安装包

只有在 ARM 程序和两份可合法使用的游戏镜像齐备后，才运行：

```sh
./scripts/package-kual.sh
```

生成 `Kindle-Golden-Hero-KPW1-install.zip`。安装目录独立于 WordScreen，不覆盖 WordScreen 文件。

## 当前尚未完成

1. 尚未产出真正的 KPW1 ARM 游戏 ELF。
2. 尚未进行 framebuffer 实际绘制与刷新实机测试。
3. 触摸坐标范围已确认，仍需实机点按测试验证按键热区。
4. 尚未取得可合法使用的游戏 ROM/Flash 镜像。
5. 尚未进行 KPW1 实机游戏、触摸、存档、休眠和长时间运行测试。
