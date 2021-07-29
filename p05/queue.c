// GRR20185067 Bruno Henrique K. de Carvalho

#include "queue.h"
#include <stdio.h>


//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila

void queue_append (queue_t **queue, queue_t *elem)
{
	if((queue != NULL ) && (elem != NULL))	//Caso ambos, a fila e o elemento existam
	{
	
		if((elem->next == NULL) && (elem->prev == NULL))	//O elemento nao esta em outra fila
		{

			if((*queue) == NULL)	//Se a fila estiver vazia
			{
				//Faz o elemento apontar para ele mesmo, pois ele eh o unico elemento na fila
				elem->next = elem;
				elem->prev = elem;
				
				//Corrige o inicio da fila
				(*queue) = elem;
			
			}
			else					//Se a fila nao esta vazia
			{
				//Faz o elemento apontar para o primeiro elemento e para o penultimo elemento da nova fila
				elem->next = (*queue);
				elem->prev = (*queue)->prev;
				
				
				elem->next->prev = elem;		//Corrige o ponteiro do primeiro elemento da fila
				elem->prev->next = elem;	//Corrige o ponteiro do penultimo elemento da nova fila
				
			}
			
		}
		else 												//O elemento esta em outra fila
		{
			fprintf(stderr,"Erro: O elemento esta em outra fila \n");
		}
	
	}
	else
	{
		if(queue == NULL)	//Caso a fila nao exista
		{
			fprintf(stderr,"Erro: A fila nao existe \n");
		}
		if(elem == NULL)	//Caso o elemento nao exista
		{
			fprintf(stderr,"Erro: O elemento nao existe \n");
		}
	
	}
	
}

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: apontador para o elemento removido, ou NULL se erro

queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
	
	if((queue != NULL ) && (elem != NULL))	//Caso ambos, a fila e o elemento existam
	{
		if((*queue) != NULL)		//Caso a fila nao esteja vazia
		{
			
			queue_t * elem_search = (*queue)->next;
			
			//Busca o elemento na fila			
			while((elem_search != (*queue)) && (elem_search != elem))
			{
				if(elem_search == NULL)
				{
					fprintf(stderr,"Erro: Erro no encadeamento da fila.A fila nao eh circular \n");
					return NULL;
				}
				
				elem_search = elem_search->next;
			}
			
			if (elem == elem_search)	//Caso o elemento esteja na fila
			{
				//Remove o elemento da lista
				elem->prev->next = elem->next;
				elem->next->prev = elem->prev;
				
				if(elem == (*queue))	//Caso o elemento a ser removido eh o primeiro da fila
				{
					if (elem->next != elem)	//Caso o elemento a ser removido nao eh o unico item da fila
					{
						(*queue) = elem->next;	//Atualiza o inicio da fila
					}
					else 				//Caso o elemento seja o unico item da fila
					{
						(*queue) = NULL;		//A fila ficou vazia
					}
					
				}
				
				//Atualiza os ponteiros do elemento a ser excluido
				elem->prev = NULL;
				elem->next = NULL;
				
				
				return elem;
			}
			else 						//Caso o elemento nao esteja na fila
			{
				fprintf(stderr,"Erro: O elemento nao esta na fila \n");
			}
			
			
		}
		else 						//Caso a fila esteja vazia
		{
			fprintf(stderr,"A fila esta vazia \n");
		}
		
	}
	else
	{
		if(queue == NULL)	//Caso a fila nao exista
		{
			fprintf(stderr,"Erro: A fila nao existe \n");
		}
		if(elem == NULL)	//Caso o elemento nao exista
		{
			fprintf(stderr,"Erro: O elemento nao existe \n");
		}
	
	}	

	return NULL;

}

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue)
{
	int size = 1;
	queue_t * aux;

	if(queue == NULL)	//Caso a fila esteja vazia
	{
		return 0;
	}
	else 				//Caso nao esteja vazia
	{
		aux = queue->next;
		while(aux != queue)
		{
			aux = aux->next;
			size++;
		}
		
		return size;
	
	}
	

}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca.
//
// Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir

void queue_print (char *name, queue_t *queue, void print_elem (void*) )
{
	queue_t * aux;
	
	printf("%s: [",name);

	if(queue != NULL)	//Caso a fila nao esteja vazia
	{
		aux = queue;
		
		//Imprime o primeiro elemento
		print_elem(aux);
		aux = aux->next;
		
		while(aux != queue)	//Caso tenha um proximo	elemento
		{
			printf(" ");
			print_elem(aux);
			aux = aux->next;
		}
	}
	
	printf("]\n");
	
	return;
}

