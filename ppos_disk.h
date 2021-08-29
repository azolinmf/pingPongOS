// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#include "ppos_data.h"

#ifndef __DISK_MGR__
#define __DISK_MGR__

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// fila de operacoes do disco
typedef struct diskOperation
{
  struct diskOperation *prev;
  struct diskOperation *next;
  task_t* task; // task da operacao
  int cmd; // comando da operacao (escrita/leitura)
  int block; // bloco do disco
  void *buffer; // buffer a ser preenchido ou a ser inserido no disco
} diskOperation;


// estrutura que representa um disco no sistema operacional
// gerenciador do disco virtual
typedef struct
{
  // completar com os campos necessarios
  
  struct task_t* executedTask; // task qem execucao
  int status;
  semaphore_t semaphoreDisk; // semaforo do disco
  int position; // posicao atual da cabeca do disco
  int numBlocks; // numero de blocos do disco
  int countBlocks; // numero de blocos percorridos pela cabeca do disco

  int execTime; // soma total do tempo de execucao
  int startAt; // tempo inicial de uma execucao

} disk_t ;


task_t taskManager; // gerente do disco

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

#endif
