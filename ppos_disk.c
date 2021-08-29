#include "ppos_disk.h"
#include "ppos.h"
#include "ppos_data.h"
#include "ppos-core-globals.h"
#include "disk.h"
#include <signal.h>
#include <time.h>
#include <stdlib.h>

#define FCFS 1
#define SSTF 0
#define CSCAN 0

struct diskOperation* operationsQueue;
task_t* suspendedQueue;
disk_t disk;
struct sigaction action;
int sair = 0;

void tratadorDisco() {
    sair = 1;
    int endTime = systime();
    disk.execTime += (endTime - disk.startAt);
    // task_switch(&taskManager);
}

diskOperation* fcfs() {
    disk.countBlocks += abs(disk.position - operationsQueue->block);
    return (diskOperation *)queue_remove((queue_t **)&operationsQueue, (queue_t *)operationsQueue);
}

diskOperation* sstf() {
    int distance = disk.numBlocks + 1;
    diskOperation* nextOp = operationsQueue;
    diskOperation* op = operationsQueue;
    for (int i = 0; i < queue_size((queue_t *)operationsQueue); i++) {
        if (abs(disk.position - op->block) < distance) {
            distance = abs(disk.position - op->block);
            nextOp = op;
        }
        op = op->next;
    }
    disk.countBlocks += distance;

    return (diskOperation *)queue_remove((queue_t **)&operationsQueue, (queue_t *)nextOp);
}

diskOperation* cscan() {
    int distance = disk.numBlocks + 1;
    int currentDistance = disk.numBlocks + 1;
    diskOperation* nextOp = operationsQueue;
    diskOperation* op = operationsQueue;
    for (int i = 0; i < queue_size((queue_t *)operationsQueue); i++) {
        if (disk.position > op->block) {
            currentDistance = abs(disk.position - disk.numBlocks) + op->block;
        } else {
            currentDistance = op->block - disk.position;
        }
        if (currentDistance < distance) {
            distance = currentDistance;
            nextOp = op;
        }
        op = op->next;
    }

    disk.countBlocks += distance;

    return (diskOperation *)queue_remove((queue_t **)&operationsQueue, (queue_t *)nextOp);
}

void DiskDriverBody(void *arg) {
    while (1) {

        // se foi acordado devido a um sinal do disco
        if (sair) {

            sair = 0;
            // obtém o semáforo de acesso ao disco
            sem_down(&disk.semaphoreDisk);

            // acorda a tarefa cujo pedido foi atendido
            queue_remove((queue_t **)&suspendedQueue, (queue_t*)disk.executedTask);

            // libera o semáforo de acesso ao disco
            sem_up(&disk.semaphoreDisk);
            
            task_resume(disk.executedTask);
        }

        // se o disco estiver livre e houver pedidos de E/S na fila
        if (disk_cmd(DISK_CMD_STATUS, 0, 0) == 1 && (operationsQueue != NULL)) {
            // escolhe na fila o pedido a ser atendido
            diskOperation *executedOp;
            if (FCFS) {
                executedOp = fcfs();
            } else if (SSTF) {
                executedOp = sstf();
            } else if (CSCAN) {
                executedOp = cscan();
            }

            disk.position = executedOp->block;
            disk.executedTask = executedOp->task;
            disk.startAt = systime();
            // solicita ao disco a operação de E/S
            disk_cmd(executedOp->cmd, executedOp->block, executedOp->buffer);
            free(executedOp);
        }

        // suspende a tarefa corrente (retorna ao dispatcher)
        // task_yield();
    }
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize) {
    int result = disk_cmd(DISK_CMD_INIT, 0, NULL);

    // retorna -1 se houver erro na inicializacao
    if (result < 0) {
        return -1;
    }

    result = disk_cmd(DISK_CMD_DISKSIZE, 0, NULL);
    if (result < 0) {
        return -1;
    }
    *numBlocks = result;
    disk.numBlocks = result;

    result = disk_cmd(DISK_CMD_BLOCKSIZE, 0, NULL);
    if (result < 0) {
        return -1;
    }
    *blockSize = result;
    operationsQueue = NULL;

    sem_create(&disk.semaphoreDisk, 1);

    action.sa_handler = tratadorDisco;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGUSR1, &action, 0) < 0){
        perror ("Erro em sigaction: ") ;
        return -1;
    }

    disk.position = 95;
    disk.execTime = 0;
    disk.countBlocks = 0;

    task_create(&taskManager, DiskDriverBody, 0);

    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer) {
    // obtém o semáforo de acesso ao disco

    // inclui o pedido na fila_disco
    diskOperation* operation = (diskOperation*) malloc(sizeof(diskOperation));
    operation->block = block;
    operation->buffer = buffer;
    operation->cmd = DISK_CMD_READ;
    operation->task = taskExec;
	operation->prev = NULL;
	operation->next = NULL;
    sem_down(&disk.semaphoreDisk);
    queue_append((queue_t **)&(operationsQueue), (queue_t *)operation);
    queue_remove((queue_t **)&readyQueue, (queue_t *)&taskExec);
    sem_up(&disk.semaphoreDisk);

    task_t* sleepTask = suspendedQueue;
    int foundTask = 0;
    for (int i = 0; i < queue_size((queue_t*)suspendedQueue); i++) {
        if (sleepTask == &taskManager) {
            foundTask = 1;
            break;
        } else {
            sleepTask = sleepTask->next;
        }
    }
    if (foundTask) {
        // acorda o gerente de disco (põe ele na fila de prontas)
        queue_remove((queue_t **)&suspendedQueue, (queue_t *)&taskManager);
        queue_append((queue_t **)&readyQueue, (queue_t *)&taskManager);
    }

    // libera semáforo de acesso ao disco

    // suspende a tarefa corrente (retorna ao dispatcher)
    task_suspend(taskExec, &suspendedQueue);
    task_yield();
    // free(operation);

    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer) {
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.semaphoreDisk);

    // inclui o pedido na fila_disco
    diskOperation* operation = (diskOperation*) malloc(sizeof(diskOperation));
    operation->block = block;
    operation->buffer = buffer;
    operation->cmd = DISK_CMD_WRITE;
    operation->task = taskExec;
	operation->prev = NULL;
	operation->next = NULL;
    queue_append((queue_t **)&(operationsQueue), (queue_t *)operation);
    queue_remove((queue_t **)&readyQueue, (queue_t *)&taskExec);

    task_t* sleepTask = suspendedQueue;
    int foundTask = 0;
    for (int i = 0; i < queue_size((queue_t*)suspendedQueue); i++) {
        if (sleepTask == &taskManager) {
            foundTask = 1;
            break;
        } else {
            sleepTask = sleepTask->next;
        }
    }
    if (foundTask) {
        // acorda o gerente de disco (põe ele na fila de prontas)
        queue_remove((queue_t **)&suspendedQueue, (queue_t *)&taskManager);
        queue_append((queue_t **)&readyQueue, (queue_t *)&taskManager);
    }

    // libera semáforo de acesso ao disco
    sem_up(&disk.semaphoreDisk);

    // suspende a tarefa corrente (retorna ao dispatcher)
    task_suspend(taskExec, &suspendedQueue);
    task_yield();

    return 0;
}