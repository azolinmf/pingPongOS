#include "ppos_disk.h"
#include "ppos_data.h"
#include "ppos-core-globals.h"
#include "disk.h"
#include <signal.h>

disk_t disk;
struct sigaction action ;
int sair = 0;

void tratador (int signum)
{
   printf ("Recebi o sinal %d\n", signum) ;
   sair = (signum == SIGUSR1);
}

void DiskDriverBody (void* arg) {
    while (1) {
      // obtém o semáforo de acesso ao disco
      sem_down(&semaphoreDisk);
 
      // se foi acordado devido a um sinal do disco
      if (sigaction (SIGUSR1, &action, 0))
      {
         // acorda a tarefa cujo pedido foi atendido
         task_resume(disk.executedTask);
      }
 
      // se o disco estiver livre e houver pedidos de E/S na fila
      if (disk_cmd(DISK_CMD_STATUS, 0, 0) == 1 && (disk.suspendedQueue != NULL))
      {
         // escolhe na fila o pedido a ser atendido, usando FCFS
         diskOperation* executedTask = (diskOperation *) queue_remove((queue_t **)&disk.suspendedQueue, (queue_t *)disk.suspendedQueue);
         disk.executedTask = executedTask->task;
         // solicita ao disco a operação de E/S
         disk_cmd(executedTask->cmd, executedTask->block, executedTask->buffer);
      }
 
      // libera o semáforo de acesso ao disco
      sem_up(&semaphoreDisk);
 
      // suspende a tarefa corrente (retorna ao dispatcher)
      task_yield();
   }
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {
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
    disk.suspendedQueue = NULL;

    sem_create (&semaphoreDisk, 1);

    action.sa_handler = tratador ;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;

    task_create(taskManager, DiskDriverBody, 0);

    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) {
    // obtém o semáforo de acesso ao disco
    sem_down(&semaphoreDisk);
 
   // inclui o pedido na fila_disco
   diskOperation* operation;
   operation.block = block;
   operation.buffer = buffer;
   operation.cmd = DISK_STATUS_READ;
   queue_append((queue_t **)disk.suspendedQueue, (queue_t *)operation);
 
   if (gerente de disco está dormindo)
   {
      // acorda o gerente de disco (põe ele na fila de prontas)
   }
 
   // libera semáforo de acesso ao disco
 
   // suspende a tarefa corrente (retorna ao dispatcher)

    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) {
    // TODO: coloca na suspendedQueue

    int result = disk_cmd(DISK_CMD_WRITE, block, buffer);
    if (result < 0) {
        return -1;
    }
    
    // TODO: retira da suspendedQueue


    return 0;
}