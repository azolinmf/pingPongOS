#include "ppos.h"
#include<stdio.h>

int UPPER_BOUND = 20;
int LOWER_BOUND = -20;

task_t *currentTask, *readyTasks;

task_t * scheduler() {
    task_t *tempTask = readyTasks;
    task_t *nextTask = readyTasks;

    // enquanto a lista não chega no fim e nem aponta para o 
    // proximo elemento (lista circular)
    while(tempTask != NULL && tempTask->next != readyTasks) {
        tempTask = tempTask->next;

        if (tempTask->prio > -20) {
            tempTask->prio--;
        }

        // pega tarefa com maior prioridade
        if (nextTask->prio < tempTask->prio) {
            nextTask = tempTask;
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
        currentTask->staticPrio = prioridade;
    }
    task->staticPrio = prioridade;
}

int task_getprio (task_t *task) {
    if (task == NULL) {
        return currentTask->staticPrio;
    }
    return task->staticPrio;
}