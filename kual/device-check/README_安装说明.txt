Kindle PW1 设备检测小工具 v0.3

用途：只读取设备信息，确认 KPW1 的屏幕、触摸设备、系统 ABI 和动态库环境。
不会安装游戏，不会修改 WordScreen，不会更改系统电源设置。

安装：
1. 将 ZIP 解压到 Kindle USB 根目录 /mnt/us。
2. 确认文件位于 extensions/GoldenHeroCheck/。
3. 安全弹出 Kindle，打开 KUAL。
4. 选择「KPW1 Golden Hero Device Check」->「生成 KPW1 环境检测报告」。
5. 通过 USB 读取 golden_hero/logs/kpw1-diagnostic.txt。
6. 把这个 TXT 文件发回给 ChatGPT。

本检测工具不会启动游戏，也不需要 ROM。
