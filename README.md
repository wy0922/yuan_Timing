# 🏫 某班电脑助手 (Classroom Computer Assistant)

**开发者：** Gemini,城南  
**最新版本：** v4.0.2.debug1

## 💡 项目简介

这是一个专为高中班级多媒体电脑量身定制的自动化管理系统。
该项目基于 C++ 和 Win32 API 打造，轻量化、极简风格 UI 设计，可解决班级电脑日常管理中的诸多痛点。
每天定时播放打铃音频，到自动化打开早读课件，再到值日生名单的透明悬浮展示，以及一整套数字化的班级纪律与惩罚登记系统，它都能在后台默默地为你高效完成。

在最新版本 v4.0.2.debug1 中，项目进行了 UI 极简重构。

> **🤖 AI 辅助开发声明**
>
> 本项目的全部代码逻辑不是我一行行手敲出来的。作为开发者，我主要负责提供核心思路、业务需求以及系统架构，而具体的代码编写、复杂的 Win32 API 调用（包括透明悬浮窗绘制、日历算法、系统托盘等）几乎完全是在 **Google Gemini** 的辅助下共同完成的。
> 这是一次 AI 赋能日常效率工具开发的极佳实践，也证明了只要你有好的想法，AI 就能帮你把创意变成现实。(甚至连这个readme初版也是Gemini写的,我进行了编辑)

## ✨ 核心功能

* **📅 智能值日悬浮窗**
    * 零点自动按星期切换值日组别。
    * 在特定的班级考勤时间段自动弹出半透明的悬浮提示，双击即可隐藏。
* **🔊 定时播报与打铃系统**
    * 支持添加自定义音频任务（如特定时间播放指定音乐）。
    * 内置早晨静音铃、中午自动关机、傍晚活动清场铃、晚上静默关机等默认时间节点控制。
* **📖 自动化早读调度**
    * 每天自动轮换 A/B（先语文/先英语）预案。
    * 根据预案自动在桌面上扫描匹配的 PPT 课件并自动打开。
* **⚖️ 班级惩罚与纪律管理系统**
    * 可视化全班座次表点击录入，告别纸质账本。
    * 严格的单选联动逻辑，结合三周交互式日历组件，精准分配（如“打扫厕所”、“写检讨”）的具体执行日期。
    * 一键将所有的违纪记录生成为包含汇总的 UTF-8 CSV 报表，并自动调用 Excel 唤醒查看。
* **🛠️ 实用班委工具箱**
    * **剪贴板智能排版**：一键去除 QQ 聊天记录时间戳，智能剔除空行并对中英文进行断句排版。
    * 一键锁屏、重启资源管理器等快捷操作。

## 🚀 编译与使用

1.  环境准备：推荐使用 Visual Studio，确保安装了 C++ 桌面开发工作负载。
2.  新建一个空白的 Windows 桌面向导项目，将 `main.cpp` 放入源码中。
3.  由于使用了 `winmm.lib`、`comctl32.lib` 等系统库，代码中已通过 `#pragma comment(lib, "xxx.lib")` 自动链接，直接编译运行即可。
4.  运行后，程序将常驻托盘，初次启动会自动在 `D:\yuan_Timing` 生成配置文件目录。你需要根据 `seats.txt` 和 `duty.txt` 内的指引填充班级真实名单。

## 🤝 贡献与反馈

欢迎提交 Issue 添加我的QQ帮助改进该项目！

### 🔞 关于使用文档

<font color= #DCDCDC>太懒了所以没写,自行研究吧</font>
主界面截图
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/db1ec858-d1fc-43ee-a534-5a884c6b7ba3" />
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/6eef9403-77f1-4fbb-85c5-2b7889a93b65" />
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/df267bc3-c905-46c4-a91f-8485970069a4" />
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/ae5ddcc5-2bad-414b-b46b-e75a0e6fce51" />

惩罚系统
<img width="1073" height="657" alt="image" src="https://github.com/user-attachments/assets/83a008de-e8ec-455c-b9bb-2c8b4e5bef40" />
<img width="586" height="493" alt="image" src="https://github.com/user-attachments/assets/63306f75-7fa3-40a3-8cae-57db54142c4d" />
<img width="226" height="172" alt="image" src="https://github.com/user-attachments/assets/dd3324d9-0f59-4eec-b373-f0dc1e9f8f2d" />
<img width="1904" height="990" alt="image" src="https://github.com/user-attachments/assets/2e9b551a-19ed-4ccc-85dd-6fb201cf089f" />

托盘菜单
<img width="219" height="295" alt="image" src="https://github.com/user-attachments/assets/5b239536-db2c-4de2-afff-05d062acf53e" />

值日悬浮窗(DUTY)
<img width="340" height="520" alt="image" src="https://github.com/user-attachments/assets/9dae5f99-18ea-4444-9c86-2056ece66fa5" />

定时关机
<img width="244" height="188" alt="image" src="https://github.com/user-attachments/assets/9f799e82-5f1d-4e00-b29b-39d97242c45f" />

