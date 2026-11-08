KPW1 Framebuffer / Touch Probe v0.4

用途：
读取 KPW1 的 framebuffer 和 cyttsp 触摸输入 ioctl 参数，用于校准《黄金英雄坛说》移植版。

安装：
1. 解压本 ZIP 到 Kindle USB 根目录。
2. 安全弹出 Kindle。
3. 打开 KUAL。
4. 运行“KPW1 Framebuffer Touch Probe” -> “读取 framebuffer 与触摸坐标参数”。

输出：
/mnt/us/golden_hero_probe/logs/fb-input-probe.txt

运行条件：
需要已经安装 KOReader，并且 KOReader 的 LuaJIT 可执行文件位于常见的 /mnt/us/koreader 或 /mnt/us/extensions/koreader 路径下。脚本会尝试自动查找。

安全：
只打开 /dev/fb0 与 /dev/input/event0 并读取 ioctl 信息；
不写 framebuffer、不发送屏幕刷新命令、不读取或修改电源属性、不改动 WordScreen。
