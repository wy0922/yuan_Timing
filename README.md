# 🏫 某班 电脑助手 (Classroom Computer Assistant)

[![Language](https://img.shields.io/badge/Language-C++-blue.svg)](https://isocpp.org/)
[![Version](https://img.shields.io/badge/Version-v4.0.2.debug1-green.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)]()

一个为高中班级多媒体电脑量身打造的**自动化管理系统**。轻量化、极简风格，基于 C++ 和 Win32 API 开发。

> **🤖 关于项目开发**
> 本项目的雏形由 **Google Gemini (AI)** 完成,我在此基础进行了引导和修改。
> 这是 AI 辅助开发日常效率工具的实践尝试：我提供核心架构和具体需求，AI 负责繁琐的 API 实现与逻辑调试。

---

## ✨ 核心功能

* **📅 智能值日悬浮窗**：零点自动按星期切换，半透明悬浮提示，双击隐藏。
* **🔊 定时播报系统**：自定义音频任务，覆盖早读、晚间关机等核心时间节点。
* **📖 早读调度自动化**：A/B 预案轮换，自动匹配桌面 PPT 并一键启动。
* **⚖️ 班级纪律管理**：可视化座次点击录入，联动惩罚系统，一键导出 **CSV 报表**。
* **🛠️ 班委工具箱**：剪贴板智能排版（去除时间戳/空格/断句）、一键锁屏、资源管理器重启。

---

## 🖼️ 功能展示

| 主界面 | 惩罚系统 | 托盘菜单 |
| :---: | :---: | :---: |
| <img src="https://github.com/user-attachments/assets/db1ec858-d1fc-43ee-a534-5a884c6b7ba3" width="200" /> | <img src="https://github.com/user-attachments/assets/83a008de-e8ec-455c-b9bb-2c8b4e5bef40" width="200" /> | <img src="https://github.com/user-attachments/assets/5b239536-db2c-4de2-afff-05d062acf53e" width="100" /> |

*(更多详细界面请参考本文档底部)*

---

## 🚀 快速开始

1.  **环境准备**：使用 Visual Studio，安装 C++ 桌面开发工作负载。
2.  **编译运行**：新建 Windows 桌面向导项目，将 `main.cpp` 放入源码，代码已内嵌库链接，直接构建即可。
3.  **初始化**：首次运行后，程序将在 `D:\yuan_Timing` 生成配置文件。请按需填充 `seats.txt` 和 `duty.txt`。



---

## ⛓ 版本迭代历史

<details>
<summary>点击查看详细演进记录</summary>

* **v1.0 - v2.0**: 基础雏形，实现定时打铃与强制锁屏（黑框终端模式）。
* **v3.0 - v3.8**: 视觉化升级，引入 Win32 控制面板、悬浮窗、早读轮换与排版工具。
* **v4.0.0**: 数字化管理飞跃，引入座次解析、违纪登记与 CSV 报表导出。
* **v4.0.2 (当前)**: 底层重构，修复幽灵复选 Bug，引入日期防呆机制，UI 极简美学重排。

</details>

---

## 🤝 贡献与反馈
欢迎 Fork 本项目，或通过 QQ 反馈 Bug！如遇使用疑问，欢迎直接提交 [Issue](你的仓库Issues链接) 咨询开发者(其实开发者也不懂)。

---

## 🔞 关于使用文档

<font color= #DCDCDC>太懒了所以没写,自行研究吧</font><br /><br /><br />

---
## 🖼️ 详细的截图

* 主界面截图<br />
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/db1ec858-d1fc-43ee-a534-5a884c6b7ba3" /><br />
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/6eef9403-77f1-4fbb-85c5-2b7889a93b65" /><br />
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/df267bc3-c905-46c4-a91f-8485970069a4" /><br />
<img width="521" height="603" alt="image" src="https://github.com/user-attachments/assets/ae5ddcc5-2bad-414b-b46b-e75a0e6fce51" /><br />

* 惩罚系统<br />
<img width="1073" height="657" alt="image" src="https://github.com/user-attachments/assets/83a008de-e8ec-455c-b9bb-2c8b4e5bef40" /><br />
<img width="586" height="493" alt="image" src="https://github.com/user-attachments/assets/63306f75-7fa3-40a3-8cae-57db54142c4d" /><br />
<img width="1904" height="990" alt="image" src="https://github.com/user-attachments/assets/2e9b551a-19ed-4ccc-85dd-6fb201cf089f" /><br />

* 托盘菜单<br />
<img width="219" height="295" alt="image" src="https://github.com/user-attachments/assets/5b239536-db2c-4de2-afff-05d062acf53e" /><br />

* 值日悬浮窗(DUTY)<br />
<img width="340" height="520" alt="image" src="https://github.com/user-attachments/assets/9dae5f99-18ea-4444-9c86-2056ece66fa5" /><br />

* 定时关机<br />
<img width="244" height="188" alt="image" src="https://github.com/user-attachments/assets/9f799e82-5f1d-4e00-b29b-39d97242c45f" /><br />

<p align="center">Made by ❤️  Gemini,<font color= #DCDCDC>城南</font></p>
