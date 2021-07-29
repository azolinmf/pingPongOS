#include "ppos.h"
#include <stdio.h>
#include "ppos-core-globals.h"

#define UPPER_BOUND 20
#define LOWER_BOUND -20

task_t * scheduler() {
    task_t *tempTask = readyQueue;
    task_t *nextTask = readyQueue;

    // enquanto a lista não chega no fim e nem aponta para o 
    // proximo elemento (lista circular)
    while(tempTask->next != readyQueue) {
        tempTask = tempTask->next;

        if (nextTask->prio > tempTask->prio) {
            nextTask->prio--;
            nextTask = tempTask;
        } else if (tempTask->prio > LOWER_BOUND) {
            tempTask->prio--;
        }
    }

    nextTask->prio = nextTask->staticPrio;
    return nextTask;
}

void task_setprio (task_t *task, int prio) {
    int prioridade = prio;
    // ajusta prioridade para range permitido
    if (prio < LOWER_BOUND) {
        prioridade = LOWER_BOUND;
    } else if (prio > UPPER_BOUND) {
        prioridade = UPPER_BOUND;
    }

    if (task == NULL) {
        taskExec->staticPrio = prioridade;
    }
    task->staticPrio = prioridade;
    task->prio = prioridade;
}

int task_getprio (task_t *task) {
    if (task == NULL) {
        return taskExec->staticPrio;
    }
    return task->staticPrio;
}