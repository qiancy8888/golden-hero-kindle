# Kindle《黄金英雄坛说》独立版 v0.2.0

适用 Kindle Paperwhite 1（固件 5.6.1.1），通过 KUAL 启动。安装包自带运行所需的程序和数据。

## 安装和开始

1. 将 `Kindle-Golden-Hero-KPW1-Standalone-v0.2.0.zip` 解压到 Kindle USB 根目录。安装目录为 `extensions/GoldenHeroStandalone/` 和 `golden_hero_standalone/`。
2. 安全弹出 Kindle，在 KUAL 打开「黄金英雄坛说 · Kindle 独立版」→「开始游戏」。选「新建人物」，用上下选字段、左右调整。以后可选「继续江湖」。
3. 在镇中先找书童，打开「菜单 → 背包与装备」使用木剑。到镇北商业街，再从北口到衙门接差事。到镇东打恶人，回衙门结算潜能和钱。
4. 从「菜单 → 武学修为」提升基本剑法、内功等，到家中歇息。经验达到当前版本的门槛后找村长领青龙坛地图。走近地图边缘的方形路口可看到下一处地点。
5. 需要离开时点「菜单 → 保存进度」，再点「退出」。进度写到 `golden_hero_standalone/saves/`，调试日志在 `golden_hero_standalone/logs/standalone.log`。

当前战斗与路线为独立版配置。还原范围见仓库的 `docs/ORIGINAL-COVERAGE.md`。

## 八坛入口

| 据点 | 入口 |
| --- | --- |
| 青龙坛 | 商家堡北 |
| 地罡坛 | 五指山北 |
| 朱雀坛 | 冰火岛南 |
| 山岚坛 | 武当山北 |
| 玄武坛 | 大雪山南 |
| 紫煞坛 | 黑森林西 |
| 天徽坛 | 灵心观北 |
| 暗黑坛 | 玉女峰北 |

## 实机检查

在 PW1 上游玩超过 2 分钟，保存、退出、再启动并读取进度。确认恢复 Kindle 原界面，再单独测试休眠和唤醒。异常时保留 `standalone.log` 与两份存档供核对。

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

`Build standalone Golden Hero for Kindle PW1` 工作流生成 ARM 安装包。新增汉字后，可从 [Unifont 16.0.04](https://unifoundry.com/pub/unifont/unifont-16.0.04/font-builds/) 取得字库并运行 `python3 scripts/build-opening-glyphs.py /path/to/unifont-16.0.04.hex.gz` 更新随程序编译的字形。
