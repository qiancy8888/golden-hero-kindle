# Kindle PW1《白金英雄坛说》v1.0.0

适用设备为已越狱并安装 KUAL 的初代 Kindle Paperwhite，固件 5.6.1.1。游戏使用 [GMUD_GB](https://github.com/wangkira/GMUD_GB) 的 GBC 版，KUAL 安装包内含 ARM 运行器、启动脚本、对应源码和许可证。`hero.gbc` 需单独放入 Kindle。

## 安装与操作

1. 通过 USB 连接 Kindle，将 `Kindle-Platinum-Hero-PW1-v1.0.0.zip` 解压到 Kindle USB 根目录。
2. 使用 v0.1.1 修复过和尚对话的 `platinum_hero/hero.gbc`。首次安装或从 v0.1.0 升级时，请单独复制这份 ROM。已有的 `platinum_hero/hero.sav` 可继续使用。
3. 安全弹出 Kindle，在 KUAL 选择「白金英雄坛说 → 开始游戏」。
4. 用方向键移动，用 `A` 确认或交谈，`B` 返回。`START` 和 `SELECT` 对应游戏的原始 GBC 按键。首轮可选预设名字，并直接确认默认属性。
5. 用游戏菜单保存进度，再点右下角 `EXIT`。重新启动后选择「继续游戏」。两个游戏存档槽都在 `platinum_hero/hero.sav` 内，可通过 USB 备份整个文件。

正式版可持续游玩，点 `EXIT` 返回 Kindle。运行器定期向启动器报活，停止报活时启动器会尝试结束游戏并恢复 Kindle 界面。能读取原屏保设置时，游玩期间会暂缓自动屏保，退出时恢复此前设置。日志位于 `platinum_hero/logs/platinum.log`。

走路、菜单和对话使用 DU4 四阶灰度局刷。若屏幕驱动拒绝 DU4，会切到 DU 黑白局刷。画面变化后最早在距上次提交 300 毫秒时更新，按键后的首帧直接参与刷新调度。驱动忙时合并中间画面，下次提交最新位置。残影累积时点底部 `CLEAN`，游戏窗口会闪一次完成清刷。退出日志会记录运行时长、局刷和清刷次数及所用波形。

## 构建游戏 ROM

使用 GBDK 2020 4.5.0 和 Python 3。从 [固定的 GMUD_GB 提交](https://github.com/wangkira/GMUD_GB/tree/672f687bbc57b281ab3fbe597b4e041eb2505c6e) 取得源码。以下命令在临时源码副本里修复 NPC 加载器的跨银行调用，补齐缺失的菜单标志和三段快速滚动函数，并用上游的检查脚本核对 HOME 区。ROM 生成在 GMUD_GB 副本的 `build/hero.gbc`。

```sh
python3 scripts/build-platinum-rom.py --gmud /path/to/GMUD_GB --lcc /path/to/gbdk/bin/lcc
```

Windows 下使用 `python` 和 `lcc.exe`。

## 构建运行器

准备 CMake、Ninja、Python 3 和 Zig 0.15.2。下载 [固定的 Gambatte 源码 ZIP](https://codeload.github.com/libretro/gambatte-libretro/zip/77f2567d718b8973d02caa2daa8dce4953faf811)，保留为 `gambatte.zip`，解压为 `build-gambatte/`。以下命令在 `golden-hero-kindle` 项目根目录运行。Windows 上如果 Ninja 无法识别含非 ASCII 字符的路径，可使用纯 ASCII 的构建目录和源码副本。

```sh
cmake -S . -B build-platinum-host -G Ninja -DGOLDEN_PLATINUM=ON \
  -DPLATINUM_GAMBATTE="$PWD/build-gambatte" \
  -DPLATINUM_TEST_ROM=/absolute/path/to/hero.gbc
cmake --build build-platinum-host
ctest --test-dir build-platinum-host --output-on-failure
python3 tests/test_platinum_guard.py
```

省略 `PLATINUM_TEST_ROM` 时运行无需 ROM 的显示边界和刷新调度测试。提供新版 ROM 时，另测建角、重开读档、出门和道德和尚对话。以下命令构建 PW1 的静态 ARM EABI5 运行器并生成安装包。

```sh
ZIG=/absolute/path/to/zig
cmake -S . -B build-platinum-arm -G Ninja -DGOLDEN_PLATINUM=ON \
  -DPLATINUM_GAMBATTE="$PWD/build-gambatte" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR=arm \
  "-DCMAKE_C_COMPILER=$ZIG;cc;-target;arm-linux.2.6.31-musleabi;-mcpu=cortex_a8" \
  "-DCMAKE_CXX_COMPILER=$ZIG;c++;-target;arm-linux.2.6.31-musleabi;-mcpu=cortex_a8" \
  '-DCMAKE_EXE_LINKER_FLAGS=-static -fno-pie -no-pie'
cmake --build build-platinum-arm
python3 scripts/package-platinum.py --binary build-platinum-arm/platinum_hero \
  --gambatte-archive gambatte.zip --zig-dir "$(dirname "$ZIG")"
```
