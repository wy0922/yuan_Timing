# 🏫 14班电脑助手 (Classroom Computer Assistant)

[![Language](https://img.shields.io/badge/Language-C++-blue.svg)](https://isocpp.org/)
[![Version](https://img.shields.io/badge/Version-v4.0.2.debug1-green.svg)]()
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)]()

一个为高中班级多媒体电脑量身打造的**自动化管理系统**。轻量化、极简风格，基于 C++ 和 Win32 API 开发。

> **🤖 关于项目开发**
> 本项目由 **城南 (Dev)** 与 **Google Gemini (AI)** 协作完成。
> 这是 AI 辅助开发日常效率工具的实践尝试：我提供核心架构与业务需求，AI 负责繁琐的 API 实现与逻辑调试。证明了只要有想法，AI 就能帮你把创意落地。

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

*(更多详细界面请参考仓库截图区)*

---

## 🚀 快速开始

1.  **环境准备**：使用 Visual Studio，安装 C++ 桌面开发工作负载。
2.  **编译运行**：新建 Windows 桌面向导项目，将 `main.cpp` 放入源码，代码已内嵌库链接，直接构建即可。
3.  **初始化**：首次运行后，程序将在 `D:\yuan_Timing` 生成配置文件。请按需填充 `seats.txt` 和 `duty.txt`。

> **💡 说明文档**
> 本着极简主义原则，文档目前处于“草稿”状态。如遇使用疑问，欢迎直接提交 [Issue](你的仓库Issues链接) 咨询开发者。

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
欢迎 Fork 本项目，或通过 QQ 反馈 Bug！

<p align="center">Made by ❤️  城南</p>
