# 项目概况

白金版 v1.0.0 面向已越狱的 Kindle Paperwhite 1（固件 5.6.1.1）。通过 KUAL 和 Gambatte 运行用户放入的 NC2000 白金版 GBC 移植 ROM，提供 300 毫秒间隔的低闪局刷、手动清残影、虚拟 GBC 按键和两槽 SRAM 存档。正式版可持续游玩，KUAL 通过游戏循环的报活信号检测卡死。见 [白金版安装与构建](docs/PLATINUM.md)。

正式版安装包通过 [GitHub Releases](https://github.com/qiancy8888/golden-hero-kindle/releases/tag/platinum-v1.0.0) 发布。游戏 ROM 在本地构建。

目标是在 Kindle Paperwhite 1（固件 5.6.1.1）上运行《黄金英雄坛说》独立复刻。游戏以文曲星 NC1020 黄金版为参照，使用本地 C++ 游戏规则和适配墨水屏的触摸界面。

| 版本 | 范围 |
| --- | --- |
| 独立版 v0.1.0 | 平安镇三张小地图、中文交谈、搬石料及一次性结算、KUAL 启动 |
| 后续 | 核对原版地图和数值、角色创建、存档、战斗、拜师与更多区域 |

独立版使用自己的 KUAL 扩展和安装目录。PW1 实机验证的项目包括触摸坐标、旧测试入口的局部刷新和退出恢复。独立版有自己的设备验证步骤，见 [实机试玩](docs/STANDALONE.md)。
