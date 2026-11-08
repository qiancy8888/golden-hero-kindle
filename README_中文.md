# Kindle《黄金英雄坛说》KPW1 移植工程 v0.9.1

> v0.9.1 在已经通过实机显示验证的 160×80 视口中加入一个可用触摸方向键移动的小镇演示，测试局部刷新、长按移动、人物碰撞和菜单交互。演示场景是我们自行编写的诊断内容，并非《黄金英雄坛说》原版游戏；原版仍需要合法可用的 NC1020 镜像。

## v0.9.1 可行走小镇交互测试（不需要 ROM）

你拍摄的 KPW1 实机照片已显示 v0.9 的 160×80 游戏区域（4 倍整数放大）和全部触摸按键，界面未被 Kindle 原生书库遮挡。

本版把原本静态的视口升级为一张**自制微型小镇**。它不是《黄金英雄坛说》游戏内容，也不读取任何 ROM。
- 按方向键：小方块人物在小镇中移动。对房屋、树木和 NPC 设置简单碰撞。
- 长按方向键：第一次立即走一步，约 400 毫秒后自动重复，每步间隔约 280 毫秒。
- 初始位置紧邻 NPC，点击 OK 可弹出 HELLO 对话，BACK 关闭。
- MENU：显示或隐藏提示，BACK 返回地图。
- EXIT：松开手指后结束测试，恢复之前的 framebuffer，返回 Kindle 原生界面。
- 最大测试时间 45 秒，KUAL 65 秒恢复保护仍然保留。
- 墨水屏优化：比较每次相邻的 1600 字节 LCD 位图，只向 EPDC 提交变化区域的矩形刷新，而不必每次刷新完整 640×320 的屏幕区域。

### 测试方法

1. 从 GitHub Actions 下载 v0.9.1 ROM-free 安装包，解压到 Kindle USB 根目录，保留 WordScreen。
2. KUAL → Golden Hero KPW1 Fullscreen Test → **小镇行走互动测试（无需ROM，45秒自动恢复）**。
3. 按 RIGHT（旁边是 NPC，应该无法通过），再按 DOWN 移动，长按约 2 秒看人物是否连续移动。
4. 点击 OK、MENU、BACK 测试对话和菜单。
5. 点击 EXIT，确认 Kindle 主界面恢复。
6. 测试日志：`golden_hero/logs/fullscreen-v0.9.1-demo.log`。

这一步是为了在没有 ROM 的情况下验证真实游戏需要的移动、按键和局部刷新链路；不能据此宣称原版黄金英雄坛说已移植成功。

## v0.9.0 游戏界面布局预览与 LCD 渲染优化

本版新增 KUAL → Golden Hero KPW1 Fullscreen Test → **游戏界面布局预览（无需 ROM，45秒自动恢复）**。

预览在 Kindle 实际 758×1024 屏幕上，使用与正式游戏相同的 160×80 LCD 视口、4 倍整数缩放和完整触摸按键布局，显示一张合成的单色示例画面。它不读取 ROM、不初始化游戏核心，也不修改 WordScreen。

同时把原先逐个像素调用 framebuffer 矩形绘制的 LCD 输出，改为按行扩展并直接写入 8bpp framebuffer，保留原有部分刷新机制。新增桌面单元测试，覆盖 1bpp 位序、整数缩放、framebuffer offset 和灰度反转。

### v0.9.0 实机测试

1. 从 GitHub Actions 下载 `golden-hero-kpw1-v0.9.0-rom-free` artifact 并解压外层 ZIP。
2. 将 `Kindle-Golden-Hero-KPW1-v0.9.0-ROM-free.zip` 解压到 Kindle USB 根目录，合并 `extensions/`，保留 WordScreen 文件。
3. KUAL → **Golden Hero KPW1 Fullscreen Test** → **游戏界面布局预览（无需 ROM，45秒自动恢复）**。
4. 检查上方 160×80 屏幕预览是否完整、文字是否方向正确、灰度是否正常；再点击方向键和功能键，确认按下反馈。
5. 点击 EXIT，确认返回原来的 Kindle 书库界面。
6. 如有问题，上传 `golden_hero/logs/fullscreen-v0.9-ui.log`。

本次只验证实际游戏视口的位图显示、布局、触摸反馈和恢复流程；示例画面不是游戏截图，不能据此判断游戏 ROM 是否能启动。

## v0.8.1 退出画面恢复修复

上一版日志已证明 EXIT 事件被识别且程序以退出码 0 正常结束；问题在于 controls test 未像 display test 一样保存和恢复原始 framebuffer。v0.8.1 在绘制测试界面前备份当前可见 framebuffer，在 EXIT、45 秒超时或 SIGINT/SIGTERM 时恢复像素并发送全屏 EPDC 刷新，然后才返回 KUAL wrapper 恢复 Awesome 与状态栏。EXIT 现在在手指抬起时确认，避免按下瞬间提前退出。

日志新增 `Original framebuffer snapshot captured` 和 `Previous framebuffer restore refresh: OK/FAILED`。

## v0.8 触摸交互测试与长按支持

实机已确认触摸标记准确落在手指点击的位置，坐标映射固定为 X=0–758、Y=0–1024，不交换轴、不翻转。

本版将输入读取改为按 Linux input 的 SYN_REPORT 分组，分别识别按下、移动和释放。游戏运行时，手指按住方向键会持续保持 NC1020 的 SetKey 按下状态，松开时释放；拖动到另一个按键区域时，会先释放旧键再按下新键。SAVE 只在首次按下时执行，EXIT 在手指抬起时确认。

新增 `--controls-test`，不需要 ROM。它使用与正式游戏相同的虚拟按键布局，显示按键按下状态，并记录 DOWN / DRAG / UP、按键名称、NC1020 key code 和屏幕坐标。

### v0.8 安装与测试

1. 从最新成功的 GitHub Actions 构建下载 `golden-hero-kpw1-v0.8-rom-free` artifact，并解压外层 ZIP。
2. 将其中的 `Kindle-Golden-Hero-KPW1-v0.8-ROM-free.zip` 解压到 Kindle USB 根目录，合并 `extensions/`，不要删除已有 WordScreen 文件。
3. 安全弹出 Kindle，打开 KUAL → **Golden Hero KPW1 Fullscreen Test** → **触摸按键操作测试（45秒，自动恢复）**。
4. 按住方向键约 2–3 秒，再松开；观察按键是否变灰/恢复。再试 OK、BACK、MENU 和 EXIT。
5. EXIT 可提前结束；否则测试 45 秒后自动退出。65 秒独立 watchdog 会恢复 Awesome 和 Kindle 状态栏。
6. 日志位于 `golden_hero/logs/fullscreen-v0.8-controls.log`，请上传给我。

全屏保护流程沿用已在 PW1 5.6.1.1 上验证的 v0.7 流程：检查固件和 `mxc_epdc_fb`，启动 65 秒恢复 watchdog，使用 Pillow nativeBridge 隐藏状态栏，暂停 Awesome（不停止 Xorg 或 framework），运行测试后恢复 Awesome 和状态栏。不会修改 WordScreen。

注意：本版只验证按键区域、按下/拖动/松开事件和按键映射日志；尚未接入真实游戏 ROM，因此不能验证人物在游戏中移动或战斗。

## v0.7 增强诊断：处理只有 TOP 残留的问题

在第一次 v0.6 实机测试中，用户的照片显示 Kindle 原生书库界面仍在前台，仅看到 TOP 等图案残影。日志只证明程序正常退出并识别设备，不能证明整屏显示成功，也没有触摸事件坐标记录。

v0.7 新增：
- 从 40 秒延长为 75 秒，便于在**运行期间**拍照。**请启动后立即（10 秒内）拍照，不要等它自动退出以后拍。**
- 每隔 2.5 秒采样 framebuffer 的多个位置，记录是否被其他进程改写，从而区分系统桌面覆盖与墨水屏刷新问题。
- 把前 160 个原始 EV_ABS、EV_KEY 和 EV_SYN 事件记录到日志，区分触摸设备是否真正收到事件和点击识别是否失效。
- 记录是点击 EXIT、自动超时还是信号中断导致退出，同时记录真实运行毫秒数。
- 保留原本的正常退出时恢复 framebuffer 的机制，避免在屏幕上永久留下诊断图案。

使用方法：从最新成功的 GitHub Actions 构建下载 **v0.7 ROM-free** 安装 ZIP，解压到 Kindle USB 根目录；打开 KUAL → Golden Hero → 屏幕及触摸诊断。请在前 10 秒拍摄测试画面，并点击几处屏幕后等待测试自动退出。复制 `golden_hero/logs/display-test.log` 上传。

如果日志有 `Framebuffer sample changed`，说明 framebuffer 内存内容确实被其他进程更改；如果没有变化，但仍显示书库，则优先排查 EPDC 刷新时序或 KUAL/X11 画面覆盖，而不是断言像素被覆盖。当前尚未确认具体原因。

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
