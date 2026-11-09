# 数据与来源

| 数据 | 位置 | 说明 |
| --- | --- | --- |
| 地点与居民 | `game/world.cpp` | 场景编号、地形、NPC、相邻出口和八坛解锁 |
| 角色与道具 | `game/rpg.*` | 四项先天属性、年龄、气血内力、食饮、钱、经验、装备和门派 |
| 进度 | `game/town.*` | 位置与朝向、石料和除恶差事、八坛进度、家园标记与随机种子 |
| 存档编码 | `game/save.*` | `GHKP` 标记、版本 1、递增代号、固定小端字段与 CRC32 |
| 存档文件 | `golden_hero_standalone/saves/journey.0` 和 `journey.1` | 由 `kindle/save_file.cpp` 交替写入，读档时选代号最新的有效槽 |
| 图块与字体 | `game/view.cpp`、`game/opening_glyphs.inc` | 自制图块与 GNU Unifont 字形子集 |
| 设备日志 | `golden_hero_standalone/logs/standalone.log` | KUAL 启停、刷新和恢复结果 |

原始 NC1020 固件、Flash 和状态文件留在原位置供离线核对。已核对及待核对条目分别记录在 [核对清单](docs/ORIGINAL-COVERAGE.md)。
