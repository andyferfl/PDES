#!/bin/bash

ITERATIONS=100
LOOKAHEAD=1
RESULTS_FILE="lookahead_results_128_${LOOKAHEAD}.txt"

# Очистка старых сборок и файлов
rm -rf build install
rm -f "$RESULTS_FILE"
mkdir build
cd build

# Сборка симулятора
cmake .. -DCMAKE_INSTALL_PREFIX=../install
cmake --build . --config Release
cmake --install .

# Переход к сборке модели PHOLD
cd ../examples/phold/
rm -rf build
mkdir build
cd build

cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .

# Многократный запуск симуляции
for ((i=1; i<=ITERATIONS; i++)); do
    echo "[RUN] Итерация $i" >> "$RESULTS_FILE"
    ./phold_simulation >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
done

echo "Результаты сохранены в $RESULTS_FILE"
