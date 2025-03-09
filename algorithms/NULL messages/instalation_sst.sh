#!/bin/bash

# Задайте путь установки
INSTALL_DIR="/opt/sst-core"  # Здесь укажите вашу директорию установки

# Проверка наличия необходимых зависимостей для Ubuntu
echo "Проверка зависимостей..."
if ! dpkg -l | grep -q "openmpi-bin"; then
    echo "Установка зависимостей для Ubuntu..."
    DEBIAN_FRONTEND=noninteractive sudo apt update
    DEBIAN_FRONTEND=noninteractive sudo apt install -y openmpi-bin openmpi-common libtool libtool-bin autoconf python3 python3-dev automake build-essential git
else
    echo "Зависимости уже установлены."
fi

# Проверка, установлен ли SST
if [ -f "$INSTALL_DIR/sst-core-install/bin/sst" ]; then
    echo "SST уже установлен. Пропуск установки."
else
    # Создание и переход в каталог для установки
    mkdir -p "$INSTALL_DIR"
    cd "$INSTALL_DIR" || exit 1

    # Клонирование репозитория SST
    if [ ! -d "sst-core-src" ]; then
        echo "Клонирование репозитория SST..."
        git clone https://github.com/sstsimulator/sst-core.git sst-core-src
    else
        echo "Репозиторий SST уже клонирован."
    fi

    # Переход в папку исходников и выполнение autogen.sh
    cd sst-core-src || exit 1
    if [ ! -f "autogen.sh" ]; then
        echo "Ошибка: файл autogen.sh не найден в репозитории!"
        exit 1
    else
        echo "Запуск autogen.sh..."
        ./autogen.sh || { echo "Ошибка при запуске autogen.sh!"; exit 1; }
    fi

    # Создание папки для сборки и конфигурация
    mkdir -p "$INSTALL_DIR/build"
    cd "$INSTALL_DIR/build" || exit 1
    echo "Запуск конфигурации сборки..."
    ../sst-core-src/configure --prefix=$PWD/../sst-core-install || { echo "Ошибка при конфигурации сборки!"; exit 1; }

    # Сборка и установка
    echo "Запуск сборки..."
    make || { echo "Ошибка при сборке!"; exit 1; }
    echo "Запуск установки..."
    make install || { echo "Ошибка при установке!"; exit 1; }

    # Проверка установки
    echo "Проверка установки SST..."
    "$INSTALL_DIR/sst-core-install/bin/sst-test-core" || { echo "Тестирование не прошло!"; exit 1; }

    echo "SST успешно установлен!"
fi

# Добавление пути SST в PATH
export PATH="$INSTALL_DIR/sst-core-install/bin:$PATH"
if ! grep -q "sst-core-install/bin" ~/.bashrc; then
    echo 'export PATH="/opt/sst-core/sst-core-install/bin:$PATH"' >> ~/.bashrc
    source ~/.bashrc
fi

# Проверка наличия sst-config
if ! command -v sst-config &> /dev/null; then
    echo "Ошибка: sst-config не найден. Проверьте установку SST."
    exit 1
fi

# Установка sst-phold
cd "$INSTALL_DIR" || exit 1
if [ ! -d "sst-phold-src" ]; then
    echo "Клонирование репозитория sst-phold..."
    git clone https://github.com/pdbj/sst-phold.git sst-phold-src
else
    echo "Репозиторий sst-phold уже клонирован."
fi

# Сборка sst-phold
cd sst-phold-src || exit 1
echo "Запуск сборки sst-phold..."

make || { echo "Ошибка при сборке sst-phold!"; exit 1; }

echo "sst-phold успешно установлен!"
