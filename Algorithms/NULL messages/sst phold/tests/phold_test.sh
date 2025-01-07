#!/bin/bash

# Название файлов для результатов
RESULTS_FILE="sst_results.txt"
TEMP_RESULTS_FILE="sst_2_512_10000_la.txt"

# Переменная для гранулярности
GRANULARITY=0
LOOKAHEAD=10000


# Очистка предыдущих результатов
# > "$RESULTS_FILE"
> "$TEMP_RESULTS_FILE"

# Функция для выполнения одного теста
run_test() {
    local lp=$1
    local wt=$2
    local run=$3

    echo "Запуск: LP=$lp, WT=$wt, Iteration=$run"

    # Выполнение команды SST
    OUTPUT=$(sst --lib-path=/home/alina/sst-core/sst-phold/lib --num-threads="$wt" phold.py -- --number="$lp" 2>&1)
#sst --lib-path=/home/alina/sst-core/sst-phold/lib --num-threads=2 phold.py -- --number=2
    # Извлечение нужных данных
    TOTAL_TIME=$(echo "$OUTPUT" | grep "Total time:" | awk '{print $3}')
    RECEIVES=$(echo "$OUTPUT" | grep "receives:" | awk '{print $5}' | sed 's/,//')

    # Сохранение данных для расчёта среднего
    echo "$lp $wt $TOTAL_TIME $RECEIVES" >> "$TEMP_RESULTS_FILE"

    # Также записываем в основной файл (для отслеживания прогресса)
    # echo "LP=$lp, WT=$wt, Iteration=$run, Total time=$TOTAL_TIME, Receives=$RECEIVES, Granularity=$GRANULARITY" >> "$TEMP_RESULTS_FILE"
}

# Функция для расчета средних значений и mev/s
calculate_averages() {
    local lp=$1
    local wt=$2

    # Фильтрация данных для текущей конфигурации LP и WT
    FILTERED_DATA=$(grep "^$lp $wt" "$TEMP_RESULTS_FILE")

    # Подсчёт средних значений
    TOTAL_TIME_AVG=$(echo "$FILTERED_DATA" | awk '{sum += $3} END {if (NR > 0) print sum / NR}')
    RECEIVES_AVG=$(echo "$FILTERED_DATA" | awk '{sum += $4} END {if (NR > 0) print sum / NR}')

    # Расчёт количества событий в секунду (mev/s)
    MEV_PER_S=$(echo "$RECEIVES_AVG $TOTAL_TIME_AVG" | awk '{if ($2 > 0) print $1 / $2 / 1000000}')

    # Сохранение результатов
    echo "LP=$lp, WT=$wt: Average Total time=$TOTAL_TIME_AVG, Average Receives=$RECEIVES_AVG, mev/s=$MEV_PER_S, LOOKAHEAD=$LOOKAHEAD " >> "$RESULTS_FILE"
}

# Главный цикл
main() {
    # Значения параметров
    LP_VALUES=(512)     # Примеры значений LP
    WT_VALUES=(2)     # Примеры значений WT
    RUN_COUNT=100     # Количество запусков для каждого параметра

    # Выполнение тестов
    for lp in "${LP_VALUES[@]}"; do
        for wt in "${WT_VALUES[@]}"; do
            for ((run=1; run<=RUN_COUNT; run++)); do
                run_test "$lp" "$wt" "$run"
            done
            # Расчёт средних значений для текущей конфигурации
            calculate_averages "$lp" "$wt"
        done
    done

    echo "Результаты сохранены в $RESULTS_FILE"
}

# Запуск скрипта
main
