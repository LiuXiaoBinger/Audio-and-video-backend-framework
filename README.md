# 从零设计并开发可靠的高性能的音视频安防流媒体服务器

#### 介绍
在该项目中，我按照 GB28181 公共安全视频监控联网系统的技术要求，开发了一个完整的上下级信令和流媒体服务器系统。项目旨在实现监控设备之间的注册、保活、设备资源管理、回放记录以及实时/回放流的获取和推送，构建一个高效的监控设备间交互框架

#### 软件架构
使用 CMake 构建和管理整个 C/C++ 项目，确保项目结构清晰、模块化开发。
- 开发 SIP 信令服务器，处理设备之间的 SIP 信令交互和控制信息传输，使用 PJSIP 作为 SIP 协议栈。
- 实现流媒体服务器，负责音视频实时流和回放流的传输及处理，深入解析 H.264 编解码规则。
- 使用 RTP/PS 协议对裸流进行封装和解封装，处理 RTP 负载打包、完整帧组包、以及 RTCP RR 包的检测。
- 设计并实现高效的数据结构与算法，优化内存管理和并发编程，提升系统性能。
- 使用网络 socket 和多路 IO 复用技术，确保系统能够处理高并发连接和数据传输。
- 进行代码优化和性能分析，确保系统在高负载下的稳定性和高效性。
- 深入解析 H.264 中的 SPS 和 PPS 序列参数集，从码流中计算出视频分辨率和帧率。
- 通过模块化开发，确保各技术模块解耦合，便于未来项目中的不同模块搭配开发。

#### 技术栈
- 编程语言：C/C++
- 构建工具：CMake
- 网络编程：Socket 编程、多路 IO 复用（select/poll/epoll）
- 协议栈：PJSIP（SIP 协议）、RTP/PS 协议
- 视频编解码：H.264
- 并发编程：线程、锁、读写锁、条件变量，线程池
- 性能优化：代码优化、内存管理、性能分析工具
#### 安装教程

1.  xxxx
2.  xxxx
3.  xxxx

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request



