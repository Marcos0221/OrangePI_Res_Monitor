# 简介
这是适用于**香橙派Zero3**的资源监视器，然后显示到SSD1306驱动的OLED屏幕上。

显示的信息有CPU温度、内存总大小、内存使用率、剩余内存大小、运行时长、IP地址

## 使用
1.开启i2c-3

2.运行make编译

3.复制编译完的res_monitor到/usr/bin/目录下

4.创建service文件并启用服务