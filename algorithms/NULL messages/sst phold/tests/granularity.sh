#!/bin/bash

# Название файла для таблицы результатов (CSV)
TABLE_FILE="table_granularity.csv"

# Каталог для выходных файлов
RESULTS_DIR="tests results granularity"
mkdir -p "$RESULTS_DIR"  # Создание папки, если её нет

# Значения параметров
GRANULARITY_VALUES=(1 10000 100000000)  # Гранулярность для тестирования
LP_VALUES=(2 16)
WT_VALUES=(4)
RUNS=${1:-3}  # Количество запусков (по умолчанию 3, можно передать аргументом)

# Путь к библиотеке
LIB_PATH="$HOME/sst-core/sst-phold/lib"

# Путь к исходному файлу PHold
PHOLD_FILE="$HOME/sst-core/sst-phold/src/Phold.cc"

# Функция для изменения гранулярности в исходном коде и пересборки проекта
update_granularity() {
    local granularity=$1
    sed -i "s/for (int i = 0; i < [0-9]\+; i++)/for (int i = 0; i < $granularity; i++)/" "$PHOLD_FILE"
    echo "Гранулярность обновлена до $granularity"

    # Выполнение make и make install
    cd ~/sst-core/sst-phold || exit 1
    make
    make install
    echo "Проект пересобран и установлен"
    cd - || exit 1  # Возврат в исходную директорию
}

# Функция для выполнения одного теста
run_test() {
    local lp=$1
    local wt=$2
    local granularity=$3
    local run_id=$4

    echo "Запуск: LP=$lp, WT=$wt, Granularity=$granularity, Запуск $run_id"

    # Имя выходного файла (.o)
    OUTPUT_FILE="$RESULTS_DIR/result_lp${lp}_wt${wt}_granularity${granularity}.o"
    
    # Выполнение команды SST
    OUTPUT=$(sst --lib-path="$LIB_PATH" --num-threads="$wt" phold.py -- --number="$lp" 2>&1)

    # Запись вывода в файл .o
    echo "$OUTPUT" >> "$OUTPUT_FILE"

    # Извлечение нужных данных
    TOTAL_TIME=$(echo "$OUTPUT" | grep "Total time:" | awk '{print $3}')
    RECEIVES=$(echo "$OUTPUT" | grep "receives:" | awk '{print $5}' | sed 's/,//')

    # Сохранение данных для расчёта среднего
    echo "$lp,$wt,$granularity,$TOTAL_TIME,$RECEIVES" >> "$TEMP_RESULTS_FILE"
}

# Функция для расчета средних значений и событий в секунду
calculate_averages() {
    local lp=$1
    local wt=$2
    local granularity=$3

    # Фильтрация данных для текущей конфигурации LP, WT и GRANULARITY
    FILTERED_DATA=$(grep "^$lp,$wt,$granularity" "$TEMP_RESULTS_FILE")

    # Подсчёт средних значений
    TOTAL_TIME_AVG=$(echo "$FILTERED_DATA" | awk -F, '{sum += $4} END {if (NR > 0) print sum / NR}')
    RECEIVES_AVG=$(echo "$FILTERED_DATA" | awk -F, '{sum += $5} END {if (NR > 0) print sum / NR}')

    # Расчёт событий в секунду
    MEV_PER_S=$(echo "$RECEIVES_AVG $TOTAL_TIME_AVG" | awk '{if ($2 > 0) print $1 / $2}')

    # Запись результатов в таблицу CSV
    echo "$granularity,$wt,$lp,$TOTAL_TIME_AVG,$RECEIVES_AVG,$MEV_PER_S" >> "$TABLE_FILE"
}

main() {
    echo "Granularity,Threads,LP,Avg Time,Avg Receives,Avg Events Per Sec" > "$TABLE_FILE"

    TEMP_RESULTS_FILE=$(mktemp)

    # Выполнение тестов для каждой комбинации GRANULARITY, LP и WT
    for granularity in "${GRANULARITY_VALUES[@]}"; do
        update_granularity "$granularity"  # Обновление кода перед тестом
        for lp in "${LP_VALUES[@]}"; do
            for wt in "${WT_VALUES[@]}"; do
                for ((run=1; run<=RUNS; run++)); do
                    run_test "$lp" "$wt" "$granularity" "$run"
                done
                calculate_averages "$lp" "$wt" "$granularity"
            done
        done
    done

    rm -f "$TEMP_RESULTS_FILE"

    echo "Таблица с результатами сохранена в $TABLE_FILE"
}

main
