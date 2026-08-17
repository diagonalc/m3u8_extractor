# 使用官方的 Ubuntu 基础镜像
FROM ubuntu:latest

# # 避免在安装 tzdata 等包时卡在交互界面
# ENV DEBIAN_FRONTEND=noninteractive

# 安装 gcc, make, gdb 等基础编译和调试工具
RUN apt-get update && \
    apt-get install -y build-essential gdb && \
    rm -rf /var/lib/apt/lists/*

# 设置容器内的工作目录
WORKDIR /app

# 将当前宿主机目录下的所有文件复制到容器的 /app 目录下
COPY . /app

# 编译你的代码 (注意：CSAPP 的代码通常需要链接 pthread 库)
# 如果你没有用 csapp.c，可以把 csapp.c -lpthread 删掉
RUN gcc -o server server.c csapp.c -lpthread
RUN gcc -o cgi-bin/extractor cgi-bin/extractor.c csapp.c -lpthread
RUN apt update && apt install -y curl netcat-openbsd
#RUN apt-get update && apt-get install -y iputils-ping


# 默认命令（可以留空，我们在 docker run 时指定执行什么）
CMD ["/bin/sh"]