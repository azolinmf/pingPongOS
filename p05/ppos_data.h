// GRR20185067 Bruno Henrique K. de Carvalho

// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.1 -- Julho de 2016

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas


#define STACKSIZE 32768


enum Task_status{
	READY,
	RUNNING,
	SUSPENDED,
	TERMINATED,
	NEW
};



// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next ;	// ponteiros para usar em filas
   int id ;						// identificador da tarefa
   ucontext_t context ;			// contexto armazenado da tarefa
   
   int state,exitCode,s_prio,d_prio,sys_task;
   
   unsigned int created_at,exec_time,proc_time,activations;
   
   // ... (outros campos serão adicionados mais tarde)
} task_t ;


// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif

