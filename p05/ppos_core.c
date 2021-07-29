// GRR20185067 Bruno Henrique K. de Carvalho

#include "ppos.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>


//Definicoes e variaveis globais
int prox_id;
#define new_id() ((prox_id >= 0)? (prox_id++):1)
#define alpha -1	//Fator de envelhecimento
#define TRUE  1
#define FALSE 0
#define TICK 1000		//tempo em micro de um tick
#define QUANTUM_SIZE 20	//Quantidade de ticks em cada quantum

task_t * Main_task,*Dispatcher_task;
task_t * Current_task;			//Tarefa no estado READY

int userTasks;					//Contador de tarefas do usuario
queue_t *ready_queue;			//Fila das tarefas prontas
//queue_t *suspended_queue;		//Fila das tarefas supensas

struct sigaction action ;		// estrutura que define um tratador de sinal
struct itimerval timer ;		// estrutura de inicialização to timer

unsigned int sys_clock;
int tick_counter;


void dispatcher();				//Declaracao da funcao do dispatcher


// funções gerais ==============================================================

void print_task_id(void * ptr)
{
	task_t * elem = ptr;
	if(!elem)
		return;
		
	printf(" %d(%d) ",elem->id,elem->d_prio);
}


//Funcao que trata dos sinais UNIX
void tick_interrupt_handler(int signum)
{

	//Se interrupcao do tick
	if(signum == SIGALRM)
	{
		sys_clock++;
		if(Current_task != NULL)
		{
			Current_task->proc_time++;
			tick_counter--;
			//Se eh uma tarefa do usuario e acabou o seu quantum
			if((!Current_task->sys_task) && (!tick_counter))	
				task_yield();			//Preempta tarefa por tempo
			
		}
	}


}



//Informa o valor corrente do relogio
unsigned int systime ()
{
	return sys_clock;
}

// Inicializa o sistema operacional
void ppos_init ()
{
	#ifdef DEBUG
	printf("ppos_init: Inicializando PPOS...\n");
	#endif

	//---- Inicia o 'relogio do sistema' ----//
	sys_clock = 0;					//Reseta o contador
	
	// registra a acao para o sinal de timer SIGALRM
	action.sa_handler = tick_interrupt_handler;
	sigemptyset (&action.sa_mask) ;
	action.sa_flags = 0 ;
	if (sigaction (SIGALRM, &action, 0) < 0)
	{
		fprintf(stderr,"Erro em sigaction: \n");
		exit (1) ;
	}

	// ajusta valores do temporizador
	timer.it_value.tv_usec = TICK;	// primeiro disparo, em micro-segundos
	timer.it_value.tv_sec  = 0;		// primeiro disparo, em segundos
	timer.it_interval.tv_usec = TICK;// disparos subsequentes, em micro-segundos
	timer.it_interval.tv_sec  = 0;	// disparos subsequentes, em segundos

	// arma o temporizador ITIMER_REAL
	if (setitimer (ITIMER_REAL, &timer, 0) < 0)
	{
		fprintf(stderr,"Erro em setitimer: \n");
		exit (1) ;
	}

	
	//---- Criacao de tarefas do sistema ----//
	prox_id = 1;					//Proximo ID de tarefa serah 1
	
	//desativa o buffer da saida padrao (stdout), usado pela função printf
	setvbuf(stdout, 0, _IONBF, 0);
	
	//Cria a tarefa main
	Main_task = malloc(sizeof(task_t));
	getcontext(&(Main_task->context));	//Salva o contexto da main
	Main_task->id = 0;					//ID da tarefa main eh sempre 0
	
	Current_task = Main_task;
	Current_task->state = RUNNING;		//Estado da tarefa que esta rodando(main)
		
	//Cria despachante
	Dispatcher_task = (task_t *)malloc(sizeof(task_t));
	task_create(Dispatcher_task,dispatcher,0);
	Dispatcher_task->sys_task = TRUE;	//Dispatcher eh uma tarefa do sistema
	queue_remove (&ready_queue,(queue_t *)Dispatcher_task);
	

	userTasks = 0;						//Nenhuma tarefa de usuario

	#ifdef DEBUG
	printf("ppos_init: PPOS Inicializado!\n");
	#endif

}


// gerência de tarefas =========================================================


//Escalonador
task_t * scheduler()
{
	#ifdef DEBUG
	printf("\n\nscheduler: Entrando no scheduler\n");
	queue_print ("Fila de prontas ", ready_queue, print_task_id) ;
	#endif

//	task_t* next = (task_t*)ready_queue;			FCFS
	
	//Algoritmo de envelhecimento
	task_t* next = (task_t*)ready_queue;			//Proxima tarefa a executar
	task_t* aux = (task_t*)ready_queue;

	if(aux != NULL)						//Se a fila de proximas nao esta vazia
	{
	
		//Escolhe a tarefa mais prioritaria e envelhece as demais
		while(aux->next != (task_t*)ready_queue)
		{
			aux = aux->next;
			#ifdef DEBUG
			printf ("Tarefa %d(%d) comparando com %d(%d)\n",aux->id,aux->d_prio,next->id,next->d_prio) ;
			#endif
		
			//Se a tarefa tem menos prioridade ou ela eh igual
			if(aux->d_prio < next->d_prio)
			{
				next->d_prio--;		//Envelhece a tarefa q nao sera escolhida
				next = aux;
			}
			else //Se eh menos prioritaria
			{
				//Envelhece
				aux->d_prio--;
				if(aux->d_prio < -20)
					aux->d_prio = -20;	//Valor minimo eh -20
			}
		}
	}
	else 								//Fila de proximas vazia
	{
		#ifdef DEBUG
		printf("scheduler: Sem tarefas para receber o controle\n");
		#endif
		return NULL;
	}

	#ifdef DEBUG
	printf("scheduler: A tarefa %d vai receber o controle\n",next->id);
	#endif
	next->d_prio = next->s_prio;	//Corrige prioridade dinamica
	return next;
}

//Dispachante
void dispatcher()
{

	#ifdef DEBUG
	printf("\n\ndispatcher: Entrando no dispatcher\n");
	#endif
	
	task_t * next_task;
	
	//enquanto ( userTasks > 0 )
	while(userTasks > 0)
	{
		
		#ifdef DEBUG
		printf("\ndispatcher: userTasks %d\n",userTasks);
		#endif
	
		// escolhe a próxima tarefa a executar
		next_task = scheduler();
		
		// escalonador escolheu uma tarefa?      
		if(next_task != NULL)
		{
			// transfere controle para a próxima tarefa
			task_switch (next_task);
			 
			#ifdef DEBUG
			printf("\ndispatcher: A tarefa %d esta no estado %d\n",next_task->id,next_task->state);
			#endif
			 
			// voltando ao dispatcher, trata a tarefa de acordo com seu estado
			switch (next_task->state)
			{
				case (RUNNING):
					break;
					
				case (READY):
					//ajusta os ponteiros prev e next
					next_task->prev = NULL;
					next_task->next = NULL;

					//Tarefa volta para a fila de prontas
					queue_append(&ready_queue, (queue_t *)next_task);
					break;
					
				case (SUSPENDED):
					break;
					
				case (TERMINATED):
				
					//Desaloca a pilha da tarefa
					free((next_task->context).uc_stack.ss_sp);
					break;
					
				case (NEW):
					break;
					
				default:
					fprintf(stderr,"\nEstado da tarefa inválido\n");
					break;
			}
		}
	}
	
	// encerra a tarefa dispatcher
	task_exit(0);
   
	//Volta para a main
	task_switch(Main_task);
}



// Cria uma nova tarefa.
//retorno: o ID da task ou valor negativo, se houver erro
int task_create (task_t *task,void (*start_func)(void *),void *arg)
{
	task->state = NEW;				//Tarefa sendo criada(estado NEW)
	getcontext(&(task->context));	//Salva contexto atual em task->context

	char * stack = malloc(STACKSIZE);	//Aloca a pilha da nova tarefa
	if (stack)
	{
		//Ajusta valores das variaveis de context
		(task->context).uc_stack.ss_sp = stack;
		(task->context).uc_stack.ss_size = STACKSIZE;
		(task->context).uc_stack.ss_flags = 0;
		(task->context).uc_link = NULL;	//Encerra a thread apos executar start_func
		task->id = new_id();
		makecontext(&(task->context), (void*)start_func, 1,arg);
		
		
		//Valores default
		task->s_prio = 0;				//Set valores de prioridade
		task->d_prio = task->s_prio;
		
		task->sys_task = FALSE;				//Cria tarefa como se fosse do usuario

		task->created_at = sys_clock;
		task->proc_time = 0;
		task->activations = 0;

		userTasks++;	
	}
	else
	{
		fprintf(stderr,"Erro na criação da pilha da tarefa ");
		return -1;
	}
	
	#ifdef DEBUG
	printf ("task_create: Tarefa %d criou  a tarefa %d\n",task_id(), task->id) ;
	#endif
	
	//Adiciona a nova tarefa a fila de prontas
	task->state = READY;
	queue_append(&ready_queue, (queue_t *)task) ;
	
	
	return task->id;
}


// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exitCode)
{
	#ifdef DEBUG
	printf("task_exit: Encerrando a tarefa %d\n", task_id());
	#endif
	
	Current_task->state = TERMINATED;	//Encerra a tarefa atual
	Current_task->exitCode = exitCode;

	Current_task->exec_time = sys_clock - Current_task->created_at;
	printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",Current_task->id,Current_task->exec_time,Current_task->proc_time,Current_task->activations);

	userTasks--;
	task_switch(Dispatcher_task);		//Retorna para o dispatcher	
	return;
}


// alterna a execução para a tarefa indicada
int task_switch (task_t *task)
{
	//Se vai trocar para uma tarefa valida
	if((task != NULL) && (task != Current_task))
	{
		#ifdef DEBUG
		printf("task_switch: tarefa %d -> tarefa %d\n",task_id(),task->id);
		#endif
	
		ucontext_t * aux = &(Current_task->context);	//Salva endereco do contexto
		
		//Tira da fila de prontas se for uma tarefa do usuario
		if((task != Dispatcher_task) && (task != Main_task))
			queue_remove (&ready_queue,(queue_t*)task);
		
		Current_task = task;							//Muda a tarefa atual
		Current_task->state = RUNNING;
		
		Current_task->activations++;					//Tarefa ativada
		tick_counter = QUANTUM_SIZE;					//Reseta contador
		
		return swapcontext(aux,&(task->context));		//Troca de contexto (em baixo nivel)
	}
	return 0;
}


// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id ()
{
	return Current_task->id;
}


// operações de escalonamento ==================================================

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield ()
{
		
	#ifdef DEBUG
	printf("task_yield: tarefa %d entregando o processador\n",task_id());
	printf("tick_interrupt_handler: sys_clock %d \n",sys_clock);
	#endif
	
	Current_task->state = READY;		//Tarefa sai do estado RUNNING
	task_switch(Dispatcher_task);
}


// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio)
{
	#ifdef DEBUG
	printf("task_setprio: mudando prioridade estatica da tarefa %d para %d\n",task->id,prio);
	#endif

	if(task != NULL)
	{
		if((prio >= -20) &&(prio <= 20))
		{
			task->s_prio = prio;
			task->d_prio = task->s_prio;
		}
		else
			fprintf(stderr,"Erro: prioridade invalida\n");
	
	}
	else
		Current_task->s_prio = prio;
}


// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task)
{	
	if(task != NULL)
		return task->s_prio;
	return Current_task->s_prio;
}
