# my-demo

这是一个用于熟悉 `C++ + gRPC + Redis + MySQL + Docker` 的学习项目。

## 目录

- `proto/`: RPC 协议定义
- `services/user_service/`: `user_service` 服务端实现
- `tools/user_client/`: 调试/测试客户端
- `mysql/001_init.sql`: MySQL 初始化建表脚本
- `config/user_service.yaml`: 服务默认配置
- `scripts/smoke_user_service.sh`: 端到端冒烟脚本

## 启动依赖（MySQL + Redis）

```bash
cd /home/zhouh/my-demo
docker compose up -d mysql redis
docker compose ps
```

## 构建

```bash
cd /home/zhouh/my-demo
cmake -S . -B build
cmake --build build -j
```

构建成功后会产出：
- `build/user_service`
- `build/user_client`

## 启动 user_service（宿主机方式）

```bash
cd /home/zhouh/my-demo
./build/user_service --config ./config/user_service.yaml
```

## 调试调用

```bash
# create user
./build/user_client --target 127.0.0.1:50051 create --name alice --email alice@example.com

# get user
./build/user_client --target 127.0.0.1:50051 get --user_id 1
```

## 运行端到端冒烟测试

```bash
cd /home/zhouh/my-demo
bash ./scripts/smoke_user_service.sh
```

## Docker 方式运行 user_service（可选）

```bash
cd /home/zhouh/my-demo
docker compose --profile full up -d --build user_service
docker compose ps
```

## 常见排障

1. MySQL 连接失败
- 检查 `docker compose ps` 是否 `healthy`
- 检查 `config/user_service.yaml` 中 mysql 端口是否为 `13306`

2. Redis 连接失败
- 检查 redis 端口是否为 `16379`

3. Proto 变更未生效
- 清理构建目录后重建：
```bash
rm -rf build
cmake -S . -B build
cmake --build build -j
```
