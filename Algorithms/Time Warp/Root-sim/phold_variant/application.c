#include <stdlib.h>
#include <stdio.h>
#include <ROOT-Sim.h>
#include <stddef.h>
#include <math.h> // Для log

#include "application.h"

struct _topology_settings_t topology_settings = {.type = TOPOLOGY_OBSTACLES, .default_geometry = TOPOLOGY_GRAPH, .write_enabled = false};

// Инициализация генераторов случайных чисел
unsigned int gen = 0; // Генератор случайных чисел

double delay_dist(double lambda, unsigned int *gen) {
    double rand_value = (double)rand_r(gen) / RAND_MAX; // Генерация случайного числа
    return -log(1.0 - rand_value) / lambda; // Экспоненциальное распределение
}

int dst_dist(int min, int max, unsigned int *gen) {
    return rand_r(gen) % (max - min + 1) + min; // Равномерное целочисленное распределение
}

double uniform_dist(unsigned int *gen) {
    return (double)rand_r(gen) / RAND_MAX; // Равномерное распределение на [0, 1]
}


void ProcessEvent(int me, simtime_t now, int event_type, event_content_type *event_content, unsigned int size, void *state) {
    (void)size;

    simtime_t timestamp;
    lp_state_type *state_ptr = (lp_state_type*)state;

    switch (event_type) {

        case INIT:
            state_ptr = (lp_state_type *)malloc(sizeof(lp_state_type));
            if(state_ptr == NULL){
                exit(-1);
            }

            SetState(state_ptr);
            state_ptr->events = 0;
            state_ptr->gvt_ts = 0.0;
            gen = rand(); // Инициализация генератора случайных чисел

            // Планирование начальных событий
            for (int i = 0; i < phold_events_per_entity; i++) {
                double new_ts = phold_lookahead + delay_dist(phold_lambda, &gen); // Использование delay_dist с переданным указателем на gen
                ScheduleNewEvent(me, 0.0, LOOP, NULL, 0);
            }

            break;

        case LOOP:
            state_ptr->events++;
            int dst_id = me;

            // Расчет временной метки для следующего события
            if (uniform_dist(&gen) < phold_zero_delay_ratio) {
                timestamp = now + (simtime_t)(tau * Random());
            } else {
                timestamp = now + phold_lookahead + delay_dist(phold_lambda, &gen); // Использование delay_dist с переданным указателем на gen
            }

            // Определение назначения события
            if (uniform_dist(&gen) < phold_remote_ratio) {
                dst_id = (me + dst_dist(0, n_prc_tot - 2, &gen) + 1) % n_prc_tot; // Использование dst_dist с переданным указателем на gen
            } else {
                dst_id = me;
            }

            state_ptr->gvt_ts += timestamp; // Обновление временной метки GVT

            // Планирование нового события
            ScheduleNewEvent(dst_id, timestamp, LOOP, NULL, 0);

            break;

        default:
            break;
    }
}

bool OnGVT(unsigned int me, lp_state_type *snapshot) {
    (void)me;

    if(snapshot->events < phold_events_per_entity)
        return false;
    return true;
}
