#include "ppos_disk.h"
#include "ppos.h"
#include "ppos_data.h"
#include "ppos-core-globals.h"
#include "disk.h"
#include <signal.h>

struct diskOperation* operationsQueue;
task_t* suspendedQueue;
disk_t disk;
struct sigaction action;
int sair = 0;

void tratadorDisco() {
    printf("recebeu sinal\r\n");

    sair = 1;
}

void DiskDriverBody(void *arg) {
    while (1) {
        // obtém o semáforo de acesso ao disco
        sem_down(&disk.semaphoreDisk);
        // printf("obteve semaforo\r\n");

        // se foi acordado devido a um sinal do disco
        if (sair) {
            printf("sair = 1\r\n");
            sair = 0;
            // acorda a tarefa cujo pedido foi atendido
            queue_remove((queue_t **)&suspendedQueue, (queue_t*)disk.executedTask);
            task_resume(disk.executedTask);
        }

        // se o disco estiver livre e houver pedidos de E/S na fila
        if (disk_cmd(DISK_CMD_STATUS, 0, 0) == 1 && (operationsQueue != NULL)) {
            // escolhe na fila o pedido a ser atendido, usando FCFS
            diskOperation *executedTask = (diskOperation *)queue_remove((queue_t **)&operationsQueue, (queue_t *)operationsQueue);
            disk.executedTask = executedTask->task;
            // solicita ao disco a operação de E/S
            printf("disk_cmd READ\r\n");
            disk_cmd(executedTask->cmd, executedTask->block, executedTask->buffer);
            printf("FREE\r\n");
            free(executedTask);
        }

        // libera o semáforo de acesso ao disco
        sem_up(&disk.semaphoreDisk);

        // suspende a tarefa corrente (retorna ao dispatcher)

        task_yield();
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

    task_create(&taskManager, DiskDriverBody, 0);

    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer) {
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.semaphoreDisk);

    // inclui o pedido na fila_disco
    diskOperation* operation = (diskOperation*) malloc(sizeof(diskOperation));
    operation->block = block;
    operation->buffer = buffer;
    operation->cmd = DISK_CMD_READ;
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
    // free(operation);

    printf("read return\r\n");

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
    operation->cmd = DISK_STATUS_WRITE;
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