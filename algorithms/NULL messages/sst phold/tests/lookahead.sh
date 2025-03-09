#!/bin/bash

# Название файла для таблицы
TABLE_FILE="table_lookahead.csv"

# Каталог для выходных файлов
RESULTS_DIR="tests result lookahead"
mkdir -p "$RESULTS_DIR"  # Создание папки, если её нет

# Значения параметров
LOOKAHEAD_VALUES=(1 1000 10000)
LP_VALUES=(2 16 64)
WT_VALUES=(4)
RUNS=${1:-3}  # Количество запусков (по умолчанию 3, можно передать аргументом)

# Путь к библиотеке
LIB_PATH="$HOME/sst-core/sst-phold/lib"

# Функция для выполнения одного теста
run_test() {
    local lp=$1
    local wt=$2
    local la=$3
    local run_id=$4

    echo "Запуск: LP=$lp, WT=$wt, Lookahead=$la, Запуск $run_id"

    # Имя выходного файла (.o)
    OUTPUT_FILE="$RESULTS_DIR/result_lp${lp}_wt${wt}_la${la}.o"

    # Выполнение команды SST
    OUTPUT=$(sst --lib-path="$LIB_PATH" --num-threads="$wt" phold.py -- --number="$lp" --minimum="$la" 2>&1)

    # Запись вывода в файл .o
    echo "$OUTPUT" >> "$OUTPUT_FILE"

    # Извлечение нужных данных
    TOTAL_TIME=$(echo "$OUTPUT" | grep "Total time:" | awk '{print $3}')
    RECEIVES=$(echo "$OUTPUT" | grep "receives:" | awk '{print $5}' | sed 's/,//')

    # Сохранение данных для расчёта среднего
    echo "$lp,$wt,$la,$TOTAL_TIME,$RECEIVES" >> "$TEMP_RESULTS_FILE"
}

# Функция для расчета средних значений и событий в секунду
calculate_averages() {
    local lp=$1
    local wt=$2
    local la=$3

    # Фильтрация данных для текущей конфигурации LP, WT и LOOKAHEAD
    FILTERED_DATA=$(grep "^$lp,$wt,$la" "$TEMP_RESULTS_FILE")

    # Подсчёт средних значений
    TOTAL_TIME_AVG=$(echo "$FILTERED_DATA" | awk -F, '{sum += $4} END {if (NR > 0) print sum / NR}')
    RECEIVES_AVG=$(echo "$FILTERED_DATA" | awk -F, '{sum += $5} END {if (NR > 0) print sum / NR}')

    # Расчёт событий в секунду
    MEV_PER_S=$(echo "$RECEIVES_AVG $TOTAL_TIME_AVG" | awk '{if ($2 > 0) print $1 / $2}')

    # Запись результатов в таблицу CSV
    echo "$la,$wt,$lp,$TOTAL_TIME_AVG,$RECEIVES_AVG,$MEV_PER_S" >> "$TABLE_FILE"
}

main() {
    echo "Lookahead,Threads,LP,Avg Time,Avg Receives,Avg Events Per Sec" > "$TABLE_FILE"

    TEMP_RESULTS_FILE=$(mktemp)

    # Выполнение тестов для каждой комбинации LOOKAHEAD, LP и WT
    for la in "${LOOKAHEAD_VALUES[@]}"; do
        for lp in "${LP_VALUES[@]}"; do
            for wt in "${WT_VALUES[@]}"; do
                for ((run=1; run<=RUNS; run++)); do
                    run_test "$lp" "$wt" "$la" "$run"
                done
                calculate_averages "$lp" "$wt" "$la"
            done
        done
    done

    rm -f "$TEMP_RESULTS_FILE"

    echo "Таблица с результатами сохранена в $TABLE_FILE"
}

main
