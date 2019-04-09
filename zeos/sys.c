/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#define LECTURA 0
#define ESCRIPTURA 1

#define QUANTUM 100

extern int zeos_ticks;

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
  actualizar_system_ticks();
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
  actualizar_system_ticks();
	return current()->PID;
}



void sys_exit()
{  
   actualizar_system_ticks();
}

int sys_write(int fd, char * buffer, int size){
  actualizar_system_ticks();
  int size_total = size;
  int i = check_fd(fd, ESCRIPTURA);
  if (buffer == NULL) return -1;
  if (size<0)return -1;
  if (i!=0)return i;
  char buff[4];
  int bytes_escritos = 0;
  while(size>=4){
    i = copy_from_user(buffer,buff,4);
    bytes_escritos += sys_write_console(buff,4);
    size -=4;
    buffer +=4;
  }
  i = copy_from_user(buffer,buff,4);
  bytes_escritos += sys_write_console(buff,4);
  return bytes_escritos;
}

int sys_time(){
  actualizar_system_ticks();
  return zeos_ticks;
}

int sys_fork(){

  actualizar_system_ticks();
  struct task_struct * pcb;
  struct task_struct * me = current();
  //si hay un proceso en lista de free lo assignamos
  //si no hay error
  if (list_empty(&freequeue) != 1){
    struct list_head* e = list_first(&freequeue);
    list_del(e);
    pcb = list_head_to_task_struct(e);
  }
  else return -1;

  //1024 -> Tamaño del stack, en numero de int's
  copy_data(me,pcb,1024*sizeof(int));

  //inicializamos dir_pages_baseAddr del proces hijo
  allocate_DIR(pcb);

   int data[NUM_PAG_DATA];
  //asignamos las paginas fisicas de datos para el proceso hijo
  int i;
  for (i=0;i<NUM_PAG_DATA;++i){
    data[i]=alloc_frame();
  }
  //en caso de que no hay suficientes, deshacemos la operacion i retornamos un error
  if(i!=NUM_PAG_DATA){
    for (int i=0;i<NUM_PAG_DATA;++i){
      free_frame(data[i]);
    }
    //volvemos a meter el proceso a la freequeue
    list_add_tail(&(pcb->list), &freequeue);
    return -1;
  }
  page_table_entry * hijo = get_PT(pcb);
  page_table_entry * padre = get_PT(me);

  //copiar paginas logicas del padre al hijo
  for(int i=0;i<NUM_PAG_CODE;++i){
    hijo[PAG_LOG_INIT_CODE+i].entry = padre[PAG_LOG_INIT_CODE+i].entry;
  }

  //assignar paginas logicas del hijo a pagina fisica del hijo 
  for (int i=0;i<NUM_PAG_DATA;++i){
    set_ss_pag(hijo,PAG_LOG_INIT_CODE+i,data[i]);
  }

  int paginas_hijo_temp[NUM_PAG_DATA];
  //las primeras 20 son de codigo + 8 de datos, así que las libres estaran a partir de ahi
   i=28;
  int j=0;
  while(i<TOTAL_PAGES && j<NUM_PAG_DATA){
    if(padre[PAG_LOG_INIT_CODE+i].entry==0){
      paginas_hijo_temp[j]=i;
      ++j;
    }
    ++i;
  }
  //si no hay paginas libres suficientes, deshacemos 
  if(j!=NUM_PAG_DATA){
		for(i=0; i<NUM_PAG_DATA; ++j)
			free_frame(data[i]);
		list_add_tail(&pcb->list, &freequeue);
		return -1;
	}

  for (int i=0;i<NUM_PAG_DATA;++i){
    //linkar pagina logica libre del padre con fisica del hijo
    set_ss_pag(padre,PAG_LOG_INIT_DATA+paginas_hijo_temp[i],data[i]);
    //pagina de datos del padre	
		unsigned int start = PAGE_SIZE * ( PAG_LOG_INIT_DATA + i );
		//pagina linkada del hijo al padre
		unsigned int dest = PAGE_SIZE*(PAG_LOG_INIT_DATA + paginas_hijo_temp[i] );
		copy_data((void*)start, (void*)dest, PAGE_SIZE);
		del_ss_pag(padre,PAG_LOG_INIT_DATA + paginas_hijo_temp[i] );
	}

  set_cr3(get_DIR(me));

  pcb->PID = set_PID();

  set_quantum(pcb, QUANTUM);
	pcb->estado.user_ticks = 0;
	pcb->estado.system_ticks = get_ticks();
	pcb->estado.blocked_ticks = 0;
	pcb->estado.ready_ticks = 0;
	pcb->estado.elapsed_total_ticks = get_ticks();
	pcb->estado.total_trans = 0;
	pcb->estado.remaining_ticks = get_quantum(pcb);

  union task_union * t = (union task_union*)  pcb;
	t->stack[1006] = (int)0;
	t->stack[1005] = 0;
	pcb->register_esp=(int) &t->stack[1005];
	//ponemos el nuevo proceso en la cola de ready
	list_add_tail(&(pcb->list), &readyqueue);
	
  	actualizar_system_ticks();
	//retornamos el pid del proceso creado
	
  	return pcb->PID;
}

