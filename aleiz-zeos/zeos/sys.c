/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int zeos_ticks;

extern int number_PID;

extern struct list_head freequeue;
extern struct list_head readyqueue;

int check_fd(int fd, int permissions)
{
  if (fd!=1 && fd!=0) return -9; /*EBADF*/
  if ((fd == 1 && permissions!=ESCRIPTURA) || (fd == 0 && permissions!=LECTURA)) return -13; /*EACCES*/
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

int ret_from_fork() {
	return 0;
}

int sys_fork()
{
	actualizar_system_ticks();
	struct task_struct * pcb;
	struct task_struct * actual = current();	
	//miramos si hay un task_struct libre para crear el nuevo proceso hijo
	if(list_empty(&freequeue) != 1){
		pcb = list_head_to_task_struct(list_first(&freequeue));
		list_del(list_first(&freequeue));
	}
	else return -1;
	
	//le asignamos las paginas fisicas de datos al proceso hijo 
	int i;
	int j;	
	int pag_data[NUM_PAG_DATA];

	for(i=0; i<NUM_PAG_DATA; ++i){
		pag_data[i] = alloc_frame();
	}
	//si no hay suficientes paginas libres para los datos del proceso cancelamos la creacion
	if( i!= NUM_PAG_DATA){
		for(j=0; j<i; ++j) free_frame(pag_data[j]);
		list_add_tail(&(pcb->list), &freequeue);
		return -1;
	}

	//copiamos el "task_struct" del padre al "task_struct" del hijo
	copy_data(actual, pcb, KERNEL_STACK_SIZE*sizeof(int));
	//le asignamos un directorio base diferente que el del padre
	allocate_DIR(pcb);
	
	page_table_entry * parent_PT = get_PT(actual);
	page_table_entry * child_PT = get_PT(pcb);
	//copiamos la tabla de paginas de codigo del padre a la del hijo
	for(i=0;i<NUM_PAG_CODE; ++i)
		child_PT[PAG_LOG_INIT_CODE + i].entry = parent_PT[PAG_LOG_INIT_CODE +i].entry;
	//le asignamos una pagina fisica a cada pagina logica para que se produzca bien las traducciones de direcciones
	for(i=0; i<NUM_PAG_DATA; ++i)
		set_ss_pag(child_PT, PAG_LOG_INIT_DATA + i, pag_data[i]);

	

	int pag_temp_child[NUM_PAG_DATA];
	i=28;
	j=0;
	//paginas logicas del padre libres
	while(i<TOTAL_PAGES && j<NUM_PAG_DATA){
		if( parent_PT[PAG_LOG_INIT_CODE + i].entry == 0){
			pag_temp_child[j] = i;
			++j;
		}
		++i;
	}
	//cancelamos la creacion del proceso si no hay suficientes paginas logicas del padre libres para las paginas del proceso hijo
	if(j!=NUM_PAG_DATA){
		for(i=0; i<NUM_PAG_DATA; ++j)
			free_frame(pag_data[i]);
		list_add_tail(&pcb->list, &freequeue);
		return -1;
	}
	
	for(i=0; i<NUM_PAG_DATA; ++i){
		//linkar pagina logica libre del padre con fisica del hijo
		set_ss_pag(parent_PT, PAG_LOG_INIT_DATA + pag_temp_child[i], pag_data[i]);
		//pagina de datos del padre	
		unsigned int start = PAGE_SIZE * ( PAG_LOG_INIT_DATA + i );
		//pagina linkada del hijo al padre
		unsigned int dest = PAGE_SIZE*(PAG_LOG_INIT_DATA + pag_temp_child[i] );
		copy_data((void*)start, (void*)dest, PAGE_SIZE);
		del_ss_pag(parent_PT,PAG_LOG_INIT_DATA + pag_temp_child[i] );
	}

	set_cr3(get_DIR(actual));

	//modificamos los campos que no son comunes entre padre e hijo
	
	pcb->PID = set_PID();
	set_quantum(pcb, QUANTUM);
	pcb->estado.user_ticks = 0;
	pcb->estado.system_ticks = get_ticks();
	pcb->estado.blocked_ticks = 0;
	pcb->estado.ready_ticks = 0;
	pcb->estado.elapsed_total_ticks = get_ticks();
	pcb->estado.total_trans = 0;
	pcb->estado.remaining_ticks = get_quantum(pcb);
  	//creamos el task_union con el nuevo proceso

	union task_union * t = (union task_union*)  pcb;
	t->stack[1006] = (int)&ret_from_fork;
	t->stack[1005] = 0;
	pcb->kernel_esp = (int) &t->stack[1005];

	//ponemos el nuevo proceso en la cola de ready
	list_add_tail(&(pcb->list), &readyqueue);
	pcb->process_state = ST_READY;

	//retornamos el pid del proceso creado
	actualizar_system_ticks();
  	return pcb->PID;
}

void sys_exit()
{  
	actualizar_system_ticks();
	struct task_struct * pcb = current();
	
	int i;
	for (i = 0; i < MAX_NUM_SEMAPHORES; ++i) {
	    if (semf[i].owner == pcb) {
	      sys_sem_destroy(i);
	    }
	}
	--cont_dir[calculate_DIR(pcb)];
	if(cont_dir[calculate_DIR(pcb)] <= 0) free_user_pages(pcb);

	if( list_empty(&readyqueue) != 1 ){
		pcb = list_head_to_task_struct(list_first(&readyqueue));
		list_del(list_first(&readyqueue));
		task_switch((union task_union*) pcb);
	}
	else task_switch((union task_union*) idle_task);
}

int sys_write(int fd, char * buffer, int size)
{
	actualizar_system_ticks();	
	int size_total = size;
	int i = check_fd(fd, ESCRIPTURA);
	if (buffer == NULL) return -1;
    	if (size < 0) return -1;
    	if (i != 0) return i;
	
	char buff[4];
	int bytes_escritos = 0;
	while (size >=4) {
		i = copy_from_user(buffer, buff, 4);
		bytes_escritos += sys_write_console(buff, 4);
		size -= 4;
		buffer +=4;
	}
	i = copy_from_user(buffer, buff, 4);
	bytes_escritos += sys_write_console(buff, 4);
	actualizar_system_ticks();
	return bytes_escritos;


}

int sys_read(int fd, char * buffer, int count) {
      actualizar_system_ticks();
      int size_original = count;
      int check = check_fd(fd, LECTURA);
      if(check_fd(fd,LECTURA) != 0) return -1;
      if (buffer == NULL) return -1;
      if (count < 0) return -1;
      if (count == 0) return -1;
      int num = sys_read_keyboard(buffer,count);	
     printk("Finishing the read"); 
      actualizar_system_ticks();
      return num;
}

int minim(int a, int b) {
    if (a <= b) return a;
    return b;
}


int sys_read_keyboard(char * buffer, int count) {
    int check;
    current()->info_key.toread = count;
    current()->info_key.buffer = buffer;
    if (list_empty(&keyboardqueue)) {  
        if (count <= nextKey) {
            int tmp = minim(KEYBOARDBUFFER_SIZE - firstKey, count);
            check = copy_to_user(&keyboardbuffer[firstKey], buffer, tmp);
            if (check < 0) return check;
            nextKey -= tmp;
            firstKey = (firstKey + tmp)%KEYBOARDBUFFER_SIZE;

            check = copy_to_user(&keyboardbuffer[firstKey], &buffer[tmp], count - tmp);
            if (check < 0) return check;
            tmp = count - tmp;
            nextKey -= tmp;
            firstKey = (firstKey + tmp)%KEYBOARDBUFFER_SIZE;

            current()->info_key.toread = 0;
            current()->info_key.buffer =  NULL;
        }
        else {
            while (current()->info_key.toread > 0) {
                int tmp = minim(KEYBOARDBUFFER_SIZE - firstKey, nextKey);
                tmp = minim(tmp, current()->info_key.toread);
                check = copy_to_user(&keyboardbuffer[firstKey], current()->info_key.buffer, tmp);
                if (check < 0) return check;
                nextKey -= tmp;
                firstKey = (firstKey + tmp)%KEYBOARDBUFFER_SIZE;
                
                int tmp2 = min(nextKey, current()->info_key.toread - tmp);
                check = copy_to_user(&keyboardbuffer[firstKey], &current()->info_key.buffer[tmp], tmp2);
                if (check < 0) return check;
                tmp += tmp2;
                nextKey = nextKey - tmp;
                firstKey = (firstKey + tmp2)%KEYBOARDBUFFER_SIZE;
    
                current()->info_key.toread -= tmp;
                current()->info_key.buffer = &(current()->info_key.buffer[tmp]);
	    	update_process_state_rr(current(), &keyboardqueue);
                sched_next_rr();
            }
        }
    }
    else {
	update_process_state_rr(current(), &keyboardqueue);
        sched_next_rr();
        while (current()->info_key.toread > 0) {
                int tmp = minim(KEYBOARDBUFFER_SIZE - firstKey, nextKey);
                tmp = minim(tmp, current()->info_key.toread);
                check = copy_to_user(&keyboardbuffer[firstKey], current()->info_key.buffer, tmp);
                if (check < 0) return check;
                nextKey -= tmp;
                firstKey = (firstKey + tmp)%KEYBOARDBUFFER_SIZE;
                
                int tmp2 = min(nextKey, current()->info_key.toread - tmp);
                check = copy_to_user(&keyboardbuffer[firstKey], &current()->info_key.buffer[tmp], tmp2);
                if (check < 0) return check;
                tmp += tmp2;
                nextKey = nextKey - tmp;
                firstKey = (firstKey + tmp2)%KEYBOARDBUFFER_SIZE;
    
                current()->info_key.toread -= tmp;
                current()->info_key.buffer = &(current()->info_key.buffer[tmp]);
	    	update_process_state_rr(current(), &keyboardqueue);
                sched_next_rr();
            }
    }
    return count;
}

void * sys_sbrk(int increment) {
    int HeapStart = (NUM_PAG_KERNEL + NUM_PAG_CODE + NUM_PAG_DATA)*PAGE_SIZE;
    if (current()->inici_heap == NULL) {
	    int frame = alloc_frame();
	    if (frame < 0) return frame;
	    set_ss_pag(get_PT(current()), HeapStart/PAGE_SIZE, frame);
	    current()->inici_heap = HeapStart;
	    current()->numPagesHeap = 1;
    }
    if (increment == 0) {
	    return (current()->inici_heap + current()->bytesHeap);
    }
    else if (increment > 0) {
        void * old = current()->inici_heap + current()->bytesHeap;
	    if ((current()->bytesHeap)%PAGE_SIZE + increment < PAGE_SIZE) {
	    	current()->bytesHeap += increment;
	    }
   	    else {
	    	current()->bytesHeap += increment;
	    	while((current()->numPagesHeap*PAGE_SIZE) < current()->bytesHeap) {
	    		int frame = alloc_frame();
	    		if (frame < 0) {
	    			current()->bytesHeap -= increment;
	    			while((current()->numPagesHeap*PAGE_SIZE)-current()->bytesHeap > PAGE_SIZE) {
                        free_frame(get_frame(get_PT(current()), HeapStart/PAGE_SIZE + current()->numPagesHeap - 1));
	    				del_ss_pag(get_PT(current()), ((HeapStart/PAGE_SIZE) + current()->numPagesHeap)- 1);
	    				current()->numPagesHeap--;
	    			}
	    			return frame;
	    		}
	    		set_ss_pag(get_PT(current()), ((HeapStart/PAGE_SIZE) + current()->numPagesHeap), frame);
	    		current()->numPagesHeap++;
	    	}
	    }
	    return old;
    }
    else if (current()->bytesHeap + increment < 0) {
        current()->bytesHeap = 0;
        while((current()->numPagesHeap) > 0) {
            free_frame(get_frame(get_PT(current()), HeapStart/PAGE_SIZE + current()->numPagesHeap - 1));
	    	del_ss_pag(get_PT(current()), ((HeapStart/PAGE_SIZE) + current()->numPagesHeap)- 1);
	    	current()->numPagesHeap--;
	    }
        return current()->inici_heap;
    }
    else {
	    current()->bytesHeap += increment;
	    while((current()->numPagesHeap*PAGE_SIZE)-current()->bytesHeap > PAGE_SIZE) {
            free_frame(get_frame(get_PT(current()), HeapStart/PAGE_SIZE + current()->numPagesHeap - 1));
	    	del_ss_pag(get_PT(current()), ((HeapStart/PAGE_SIZE) + current()->numPagesHeap)- 1);
	    	current()->numPagesHeap--;
	    }
	    return current()->inici_heap + current()->bytesHeap;
    }
}

int sys_gettime() 
{
	actualizar_system_ticks();
	return zeos_ticks;
}

int sys_get_stats(int pid, struct stats *st) 
{
	actualizar_system_ticks();
	if (st == NULL) return -1;
	if (pid < 0 || pid > number_PID) return -1;
	int i = 0;
	int found = 0;
	while (i<NR_TASKS && found == 0){
		if(task[i].task.PID==pid) found = 1;
		++i;
	}
	if(found){
		--i;
		copy_to_user(&task[i].task.estado, st, sizeof(struct stats));
	}
	else return -1;
	actualizar_system_ticks();
	return 0;
}

void create_thread(int *function, int *stack);

int sys_clone (void (*function) (void), void*stack) {
	actualizar_system_ticks();
	struct task_struct * pcb;
	struct task_struct * actual = current();	
	//miramos si hay un task_struct libre para crear el nuevo proceso hijo
	if(list_empty(&freequeue) != 1){
		pcb = list_head_to_task_struct(list_first(&freequeue));
		list_del(list_first(&freequeue));
	}
	else return -1;

	copy_data(actual,pcb,sizeof(union task_union));	

	++cont_dir[calculate_DIR(actual)];
	pcb->info_key.buffer = NULL;
        pcb->info_key.toread = 0;
	//modificamos los campos que no son comunes entre padre e hijo
	
	pcb->PID = set_PID();

	set_quantum(pcb, QUANTUM);
	pcb->estado.user_ticks = 0;
	pcb->estado.system_ticks = get_ticks();
	pcb->estado.blocked_ticks = 0;
	pcb->estado.ready_ticks = 0;
	pcb->estado.elapsed_total_ticks = get_ticks();
	pcb->estado.total_trans = 0;
	pcb->estado.remaining_ticks = get_quantum(pcb);
  	//creamos el task_union con el nuevo proceso
	union task_union * t = (union task_union*)  pcb;
	t->stack[1022] = stack;
	t->stack[1019] = function;
	t->stack[1006] = (int)&ret_from_fork;
	t->stack[1005] = 0;
	pcb->kernel_esp = (int) &t->stack[1005];
	
	//ponemos el nuevo proceso en la cola de ready
	list_add_tail(&(pcb->list), &readyqueue);
	pcb->process_state = ST_READY;
	//retornamos el pid del proceso creado
	actualizar_system_ticks();
  	return pcb->PID;
}

int sys_sem_init (int n_sem, unsigned int value) {
	actualizar_system_ticks();
	if (n_sem < 0 || n_sem >= MAX_NUM_SEMAPHORES) return -1;
	else if (semf[n_sem].owner != NULL) return -1;
	semf[n_sem].cont = value;
	semf[n_sem].owner = current();
	INIT_LIST_HEAD(&semf[n_sem].tasks);
	actualizar_system_ticks();
	return 0;
}

int sys_sem_wait (int n_sem) {
	actualizar_system_ticks();
	if (n_sem < 0 || n_sem >= MAX_NUM_SEMAPHORES) return -1;
	else if (semf[n_sem].owner == NULL) return -1;
	--semf[n_sem].cont;
	if (semf[n_sem].cont < 0) {
		current()->info_semf = 0;
		update_process_state_rr(current(), &semf[n_sem].tasks);
		sched_next_rr();
	}
	actualizar_system_ticks();
	return 0;
}

int sys_sem_signal (int n_sem) {
	actualizar_system_ticks();
	if (n_sem < 0 || n_sem >= MAX_NUM_SEMAPHORES) return -1;
	else if (semf[n_sem].owner == NULL) return -1;
	++semf[n_sem].cont;
	if (!list_empty(&semf[n_sem].tasks)) {
		struct list_head * l = list_first(&semf[n_sem].tasks);
		struct task_struct *t = list_head_to_task_struct(l);
		t->process_state = ST_READY;
		list_del(l);
		list_add_tail(l, &readyqueue);
	}
	actualizar_system_ticks();
	return 0;
}

int sys_sem_destroy (int n_sem) {
	actualizar_system_ticks();
	if (n_sem < 0 || n_sem >= MAX_NUM_SEMAPHORES) return -1;
	else if (semf[n_sem].owner == NULL) return -1;
	else if (current() != semf[n_sem].owner) return -1;
	while (!list_empty(&semf[n_sem].tasks)) {
		struct list_head * l = list_first(&semf[n_sem].tasks);
		struct task_struct *t = list_head_to_task_struct(l);
		t->process_state = ST_READY;
		list_del(l);        
		list_add_tail(l, &readyqueue);
	}
	semf[n_sem].owner = NULL;
	actualizar_system_ticks();
	return 0;
}






