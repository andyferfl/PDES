#!/bin/bash

INPUT_FILE="lookahead_results_10000.txt"

THREADS=4
LPS=64
LOOKAHEAD=1

OUTPUT_FILE="tw_wt${THREADS}_lp${LPS}_la${LOOKAHEAD}.txt"

awk -v threads="$THREADS" -v lps="$LPS" -v output_file="$OUTPUT_FILE" '
  BEGIN {
    count = 0;
    sim_time_sum = 0;
    exec_time_sum = 0;
    events_sum = 0;
    efficiency_sum = 0;
    rollbacks_sum = 0;
  }
  /^Simulation time:/     { sim_time_sum += $3 }
  /^Execution Time:/      { exec_time_sum += $3 }
  /^Events processed:/    { events_sum += $3 }
  /^Efficiency:/          { efficiency_sum += $2 }
  /^Rollbacks:/           { rollbacks_sum += $2; count++ }
  END {
    if (count > 0) {
      print "Threads: " threads > output_file;
      print "LPs: " lps >> output_file;
      print "Average Simulation time: " sim_time_sum/count >> output_file;
      print "Average Execution Time: " exec_time_sum/count >> output_file;
      print "Average Events processed: " events_sum/count >> output_file;
      print "Average Efficiency: " efficiency_sum/count >> output_file;
      print "Average Rollbacks: " rollbacks_sum/count >> output_file;
    }
  }
' "$INPUT_FILE"

echo "Средние результаты сохранены в файл $OUTPUT_FILE"
