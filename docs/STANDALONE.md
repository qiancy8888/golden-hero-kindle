# Kindle《黄金英雄坛说》独立版 v0.1.0

适用设备为 Kindle Paperwhite 1（固件 5.6.1.1），通过 KUAL 运行。游戏所需数据包含在安装 ZIP 中，可离线启动。每次试玩最多 45 秒，退出后进度随会话结束。

## 安装和试玩

1. 解压 `Kindle-Golden-Hero-KPW1-Standalone-v0.1.0.zip` 到 Kindle USB 根目录。会新增 `extensions/GoldenHeroStandalone/` 和 `golden_hero_standalone/`。
2. 安全弹出 Kindle，在 KUAL 中打开「黄金英雄坛说 · Kindle 独立版」，选择「平安镇试玩」，点击「进入平安镇」。
3. 可以先与身旁的村长交谈。沿大路按住「左」，从镇中走到镇西，靠近石料管事后点击「交谈」和「领石料」。
4. 沿大路往东，经过镇中到镇东。靠近工地管事后点「交谈」和「交石料」。观察经验和潜能结算。
5. 点「退出」，确认回到原界面。另开一次，验证超时能自动退出。日志在 `golden_hero_standalone/logs/opening-v0.1.0.log`。

这是第一段试玩。平安镇布局、对话和像素素材为本项目制作。试玩从具备接差事条件的角色开始，结算数值为暂定值。

## 构建与测试

在 Linux 或 WSL2 的仓库根目录运行：

```sh
cmake -S . -B build-standalone-host -DGOLDEN_STANDALONE=ON
cmake --build build-standalone-host
ctest --test-dir build-standalone-host --output-on-failure

sh scripts/bootstrap-kindle5-toolchain.sh
sh scripts/build-armel.sh --standalone
python3 scripts/package-standalone.py
```

`Build standalone Golden Hero for Kindle PW1` 工作流可生成对应 ARM 安装包。

需要新增界面用字时，从 [Unifont 16.0.04](https://unifoundry.com/pub/unifont/unifont-16.0.04/font-builds/) 取得 `unifont-16.0.04.hex.gz`，在仓库根目录执行 `python3 scripts/build-opening-glyphs.py /path/to/unifont-16.0.04.hex.gz`，并提交更新后的字形子集。字体许可随包提供。
