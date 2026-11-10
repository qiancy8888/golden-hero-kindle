# 数据与来源

| 白金版数据 | 位置 | 用途 |
| --- | --- | --- |
| 公开安装包 | GitHub Release `platinum-v1.0.0` | KUAL 运行器、启动脚本、对应源码和许可证 |
| 本地 GBC ROM | `/mnt/us/platinum_hero/hero.gbc` | `GMUD_GB` 构建产物，只读加载 |
| 卡带 SRAM | `/mnt/us/platinum_hero/hero.sav` | 32 KiB，含两个游戏存档槽。通过同目录临时文件和重命名更新 |
| KUAL 日志 | `/mnt/us/platinum_hero/logs/platinum.log` | ROM 校验、运行时长、退出原因和 DU4／DU 波形。分别统计驱动接受的局刷和手动清刷，以及遇忙重试次数 |
| 会话状态 | `/tmp/golden-hero-kpw1-session.lock/` | 记录启动器和游戏进程号及报活信号，供独占运行和异常恢复使用 |

## 黄金版独立试玩

| 数据 | 位置 | 用途 |
| --- | --- | --- |
| 镇西、镇中、镇东 | `game/town.cpp` | 19×11 地块、碰撞、NPC 位置和东西出口 |
| 会话状态 | `game/town.h` | 角色位置、朝向、任务阶段、实战经验、潜能 |
| 画面和触摸区 | `game/view.cpp` | 376×512 单色画面、16×16 图块、按键坐标和变化矩形 |
| 字形 | `game/opening_glyphs.inc`、`game/FONT-OFL.txt` | Unifont 16.0.04 的中文字形与 ASCII 子集 |
| KUAL 日志 | `/mnt/us/golden_hero_standalone/logs/opening-v0.1.0.log` | 设备识别、退出原因、刷新及恢复结果 |

用户提供的固件、Flash 镜像和状态仅供离线核对。已在电脑上用这组文件启动黄金版，查看序章和选角。Flash 中已确认「平安镇」「石料」、送往工地和奖励经验／潜能。原始地图坐标、完整对话和奖励公式还需核对。试玩地图、图块和对白为本项目编写。

当前角色预置 1000 实战经验，完成一次运石奖励 40 实战经验与 20 潜能。数值为试玩配置。游戏状态保存在进程内存中。
