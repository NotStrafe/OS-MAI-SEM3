# Используем официальный образ Ubuntu
FROM ubuntu:22.04

# Устанавливаем необходимые инструменты
RUN apt update && apt install -y \
    build-essential \
    gdb \
    strace \
    man \
    nano \
    git \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Создаем папку /labs (если ее нет)
RUN mkdir -p /os-sem3

# Указываем рабочую директорию внутри контейнера
WORKDIR /os-sem3

# Открываем контейнер в интерактивном режиме
CMD ["/bin/bash"]
