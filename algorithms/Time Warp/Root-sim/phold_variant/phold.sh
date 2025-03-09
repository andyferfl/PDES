#!/bin/bash

# Параметры запуска
WORKER_THREADS=(2)  # Число рабочих потоков для параллельных тестов
LP_VALUES=(2 4)     # Число логических процессов для тестов
NUM_RUNS=${1:-2}    # Количество запусков (по умолчанию 2, если не указано)
output_dir="phold_test_results"
mkdir -p "$output_dir"
output_table="table.csv"
make

# Функция для создания и записи в файл для каждой комбинации
run_simulation() {
  local wt=$1
  local lp=$2
  local output_file="$output_dir/test_tw_${wt}_${lp}.o"

  # Очистка выходного файла перед началом записи
  > "$output_file"
  
  for i in $(seq 1 $NUM_RUNS); do
    echo "Запуск $i: --wt $wt --lp $lp"
    
    # Запуск симуляции и запись в соответствующий файл
    ./phold_variant --wt "$wt" --lp "$lp" >> "$output_file"
    cat "./outputs/execution_stats" >> "$output_file"
  done
}

# Запуск симуляций для каждой комбинации WORKER_THREADS и LP_VALUES
for lp in "${LP_VALUES[@]}"; do
  for wt in "${WORKER_THREADS[@]}"; do
    # Проверка, чтобы количество рабочих потоков не превышало количество LP
    if [[ $wt -le $lp ]]; then
      run_simulation "$wt" "$lp"
    else
      echo "Пропуск: --wt $wt не может превышать --lp $lp"
    fi
  done
done

# Парсинг и создание таблицы для всех запусков
parse_and_create_table() {
  
  # Очистка файла таблицы перед началом записи
  > "$output_table"
  
  # Заголовок таблицы
  header="Metric"
  for lp in "${LP_VALUES[@]}"; do
    for wt in "${WORKER_THREADS[@]}"; do
      if [[ $wt -le $lp ]]; then
        header="$header\t--wt $wt --lp $lp"
      fi
    done
  done
  echo -e "$header" > "$output_table"
  
  # Функция для вычисления среднего значения
  calculate_average() {
    local metric_name="$1"
    local pattern="$2"
    local unit="$3"
    
    row="$metric_name"
    for lp in "${LP_VALUES[@]}"; do
      for wt in "${WORKER_THREADS[@]}"; do
        if [[ $wt -le $lp ]]; then
          # Извлекаем значения для текущего сочетания параметров
          values=$(grep -oP "$pattern" "$output_dir/test_tw_${wt}_${lp}.o" | awk '{print $1}')
          average=$(echo "$values" | awk '{sum += $1; count++} END {if (count > 0) print sum / count; else print "N/A"}')
          row="$row\t$average $unit"
        fi
      done
    done
    echo -e "$row" >> "$output_table"
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
  
  # Выравнивание таблицы по столбикам
  column -t -s $'\t' "$output_table" > "${output_table}.tmp"
  mv "${output_table}.tmp" "$output_table"
}

# Парсинг и создание таблицы для всех файлов
for lp in "${LP_VALUES[@]}"; do
  for wt in "${WORKER_THREADS[@]}"; do
    if [[ $wt -le $lp ]]; then
      parse_and_create_table
    fi
  done
done

echo "Таблица создана в файле $output_table"
