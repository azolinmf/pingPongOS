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

// fila de operacoes a serem realizadas pelo disco
struct diskOperation* operationsQueue;
// fila das tasks suspensas
task_t* suspendedQueue;
// instancia do gerenciador do disco
disk_t disk;

// controladoras do sinal do disco
struct sigaction action;
int sair = 0;

void tratadorDisco() {
    sair = 1;
    int endTime = systime();
    disk.execTime += (endTime - disk.startAt);
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
    // variaveis para identificar se o gerenciador de disco
    // esta sem receber operacoes novas (terminou a execucao)
    int init = systime();
    int time = systime();
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
            
            // resume a execução de uma tarefa suspensa
            task_resume(disk.executedTask);

            time = 0; // inicializa timer
        }

        // se o disco estiver livre e houver pedidos de leitura e escrita na fila
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

            // atualiza posicao atual no disco
            disk.position = executedOp->block;
            // atualiza task em execucao
            disk.executedTask = executedOp->task;
            // inicializa tempo de execucao
            disk.startAt = systime();
            // solicita ao disco a operação de leitura/escrita
            disk_cmd(executedOp->cmd, executedOp->block, executedOp->buffer);

            free(executedOp);

            time = 0; // inicializa timer
        }

        // se ficou 5s sem receber operacoes
        if (time - init > 5000) {
            printf("Tempo de execucao: %d, numero de blocos percorridos: %d\r\n", disk.execTime, disk.countBlocks);
            task_exit(0);
        }
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
    // salva o numero de blocos do disco
    disk.numBlocks = result;

    result = disk_cmd(DISK_CMD_BLOCKSIZE, 0, NULL);
    if (result < 0) {
        return -1;
    }
    *blockSize = result;

    // inicializa a lista de operacoes de leitura e escrita
    operationsQueue = NULL;

    // cria o semaforo do disco
    sem_create(&disk.semaphoreDisk, 1);

    // inicializa o tratador de sinal
    action.sa_handler = tratadorDisco;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction (SIGUSR1, &action, 0) < 0){
        perror ("Erro em sigaction: ") ;
        return -1;
    }

    // inicializa posicao do disco
    disk.position = 0;

    // contadores para o tempo de execucao e numero de 
    // blocos percorridos
    disk.execTime = 0;
    disk.countBlocks = 0;

    // cria a task gerente do disco
    task_create(&taskManager, DiskDriverBody, 0);

    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer) {

    // inclui o pedido na fila_disco
    diskOperation* operation = (diskOperation*) malloc(sizeof(diskOperation));
    operation->block = block;
    operation->buffer = buffer;
    operation->cmd = DISK_CMD_READ;
    operation->task = taskExec;
	operation->prev = NULL;
	operation->next = NULL;

    // suspende task e adiciona na fila das operacoes
    sem_down(&disk.semaphoreDisk);
    queue_append((queue_t **)&(operationsQueue), (queue_t *)operation);
    queue_remove((queue_t **)&readyQueue, (queue_t *)&taskExec);
    sem_up(&disk.semaphoreDisk);

    // verifica se gerente do disco esta suspensa
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

    // suspende a tarefa corrente (retorna ao dispatcher)
    task_suspend(taskExec, &suspendedQueue);
    task_yield();

    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer) {

    // inclui o pedido na fila_disco
    diskOperation* operation = (diskOperation*) malloc(sizeof(diskOperation));
    operation->block = block;
    operation->buffer = buffer;
    operation->cmd = DISK_CMD_WRITE;
    operation->task = taskExec;
	operation->prev = NULL;
	operation->next = NULL;

    // suspende task e adiciona na fila das operacoes
    sem_down(&disk.semaphoreDisk);
    queue_append((queue_t **)&(operationsQueue), (queue_t *)operation);
    queue_remove((queue_t **)&readyQueue, (queue_t *)&taskExec);
    sem_up(&disk.semaphoreDisk);

    // verifica se gerente do disco esta suspensa
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

    // suspende a tarefa corrente (retorna ao dispatcher)
    task_suspend(taskExec, &suspendedQueue);
    task_yield();

    return 0;
}