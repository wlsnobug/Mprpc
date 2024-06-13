# Mprpc

Mprpc 是一个基于 muduo 网络库实现的分布式网络通信框架。它具有以下特点：

- **分布式部署：** 基于 ZooKeeper 分布式协调服务，管理组织服务与方法节点，获取RPC服务目标IP及PORT。
- **通信协议：** 使用 Protobuf 序列化和反序列化作为私有化通信协议，定义RPC服务接口类和方法。

  
## 编译方式

使用脚本自动编译
```bash
./autobuild.sh
```


## 注意
在运行 ChatServer 之前，请确保以下服务已启动：
- **ZooKeeper 服务：** 项目使用 ZooKeeper 实现分布式部署，请确保 ZooKeeper 服务已启动。
