#!/bin/bash

NUM_RUNS=${1:-2}

# Параметры для компиляции
GRANULARITY_VALUES=(1 10000 100000000)  # Значения гранулярности
GRANULARITY_LABELS=("Low" "Medium" "High")  # Соответствующие метки для GRANULARITY
WORKER_THREADS=(2)  # Число рабочих потоков для параллельных тестов
LP_VALUES=(2 16)  # Число логических процессов для тестов
output_dir="phold_test_granularity"
mkdir -p "$output_dir"
make

# Функция для обновления программы с разными значениями LOOP_COUNT
update_program() {
  local granularity=$1
  local loop_count=$2
  echo "Обновление программы с LOOP_COUNT=$loop_count"
  
  # Очищаем старую сборку и собираем заново с новым значением LOOP_COUNT
  cd ~/pdes/ROOT-Sim/models/phold_variant
  sed -i "s/#define LOOP_COUNT .*/#define LOOP_COUNT $loop_count/" application.h  # Заменяем LOOP_COUNT в application.h
  make
  cd -  # Возвращаемся в начальную директорию
}

# Функция для создания и записи в файл для каждой комбинации
run_simulation() {
  local wt=$1
  local lp=$2
  local granularity=$3
  local output_file="$output_dir/test_tw_${wt}_${lp}_${granularity}.o"

  # Очистка выходного файла перед началом записи
  > "$output_file"

  for i in $(seq 1 $NUM_RUNS); do
    echo "Запуск $i: --wt $wt --lp $lp --granularity $granularity"
    
    # Запуск симуляции и запись в соответствующий файл
    ./phold_variant --wt "$wt" --lp "$lp" >> "$output_file"
    cat "./outputs/execution_stats" >> "$output_file"
  done
}

# Обновление программы для каждого значения GRANULARITY
for i in "${!GRANULARITY_VALUES[@]}"; do
  granularity=${GRANULARITY_VALUES[$i]}
  label=${GRANULARITY_LABELS[$i]}
  update_program "$granularity" "$granularity"  # Используем значение GRANULARITY как LOOP_COUNT

  # Запуск симуляций для каждой комбинации WORKER_THREADS и LP_VALUES
  for lp in "${LP_VALUES[@]}"; do
    for wt in "${WORKER_THREADS[@]}"; do
      # Проверка, чтобы количество рабочих потоков не превышало количество LP
      if [[ $wt -le $lp ]]; then
        run_simulation "$wt" "$lp" "$label"
      else
        echo "Пропуск: --wt $wt не может превышать --lp $lp"
      fi
    done
  done
done

# Парсинг и создание таблицы для всех запусков
parse_and_create_table() {
  local output_table="table_granularity.csv"
  
  # Очистка файла таблицы перед началом записи
  > "$output_table"
  
  # Заголовок таблицы (разделитель - запятая)
  header="Metric"
  for i in "${!GRANULARITY_VALUES[@]}"; do
    for lp in "${LP_VALUES[@]}"; do
      for wt in "${WORKER_THREADS[@]}"; do
        if [[ $wt -le $lp ]]; then
          label=${GRANULARITY_LABELS[$i]}
          header="$header,--wt $wt --lp $lp $label"
        fi
      done
    done
  done
  echo "$header" > "$output_table"
  
  # Функция для вычисления среднего значения
  calculate_average() {
    local metric_name="$1"
    local pattern="$2"
    local unit="$3"
    
    row="\"$metric_name\""
    for i in "${!GRANULARITY_VALUES[@]}"; do
      for lp in "${LP_VALUES[@]}"; do
        for wt in "${WORKER_THREADS[@]}"; do
          if [[ $wt -le $lp ]]; then
            label=${GRANULARITY_LABELS[$i]}
            # Извлекаем значения для текущего сочетания параметров
            values=$(grep -oP "$pattern" "$output_dir/test_tw_${wt}_${lp}_${label}.o" | awk '{print $1}')
            average=$(echo "$values" | awk '{sum += $1; count++} END {if (count > 0) print sum / count; else print "N/A"}')
            row="$row,\"$average $unit\""
          fi
        done
      done
    done
    echo "$row" >> "$output_table"
  }
  
  # Извлечение данных для каждой метрики и вычисление среднего
  calculate_average "Total simulation time" 'TOTAL SIMULATION TIME\s*\.* : \K[0-9.]+ seconds' "seconds"
  calculate_average "Total executed events" 'TOTAL EXECUTED EVENTS\s*\.* : \K[0-9]+' ""
  calculate_average "Total committed events" 'TOTAL COMMITTED EVENTS\s*\.* : \K[0-9]+' ""
  calculate_average "Total rollbacks" 'TOTAL ROLLBACKS EXECUTED\s*\.* : \K[0-9]+' ""
  calculate_average "Rollback frequency" 'ROLLBACK FREQUENCY\s*\.* : \K[0-9.]+ %' "%"
  calculate_average "Number of GVT reductions" 'NUMBER OF GVT REDUCTIONS\s*\.* : \K[0-9]+' ""
  calculate_average "Average memory usage" 'AVERAGE MEMORY USAGE\s*\.* : \K[0-9.]+ [A-Za-z]+' ""
  calculate_average "Peak memory usage" 'PEAK MEMORY USAGE\s*\.* : \K[0-9.]+ [A-Za-z]+' ""
  calculate_average "Average event cost" 'AVERAGE EVENT COST\s*\.* : \K[0-9.]+ us' "us"
}

# Парсинг и создание таблицы для всех файлов
for i in "${!GRANULARITY_VALUES[@]}"; do
  for lp in "${LP_VALUES[@]}"; do
    for wt in "${WORKER_THREADS[@]}"; do
      if [[ $wt -le $lp ]]; then
        parse_and_create_table
      fi
    done
  done
done

echo "Таблица создана в файле table_granularity.csv"
