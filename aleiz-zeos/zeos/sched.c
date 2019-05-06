/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* Vector de tasques */

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}

struct task_struct * idle_task;
struct task_struct * init_task;
union task_union * idle_taskk;
union task_union * init_taskk;

struct semaphore semf[MAX_NUM_SEMAPHORES];
int cont_dir[NR_TASKS];
char keyboardbuffer[KEYBOARDBUFFER_SIZE];
int nextKey, firstKey;

int number_PID;

struct list_head freequeue;
struct list_head readyqueue;
struct list_head keyboardqueue;

extern page_table_entry dir_pages[NR_TASKS][TOTAL_PAGES];


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}

int get_quantum(struct task_struct *t)
{
	return t->quantum;
}

void set_quantum(struct task_struct *t, int new_quantum)
{
	t->quantum = new_quantum;
}

int allocate_DIR(struct task_struct *t) 
{
	/*int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; */
	
	int it;
	for(it = 0; it < NR_TASKS; ++it) {
		if(cont_dir[it] == 0) {
			t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[it];
			return 1;
		}
	}

	return 1;
}

void update_sched_data_rr(void) 
{
	
	
	struct task_struct * actual = current();
	--actual->estado.remaining_ticks;	
	if(needs_sched_rr()){
		++actual->estado.total_trans;
		if(list_empty(&readyqueue) != 1) {
			update_process_state_rr(actual, &readyqueue);
			sched_next_rr();
		}
		else actual->estado.remaining_ticks = get_quantum(actual);
	}
	
}

int needs_sched_rr() 
{
	
	return current()->estado.remaining_ticks == 0;
}

void sched_next_rr(){
	
	struct task_struct * pcb;
	pcb = list_head_to_task_struct(list_first(&readyqueue));
	list_del(list_first(&readyqueue));

	update_process_state_rr(pcb, NULL);

	task_switch((union task_union*) pcb);
	
}

void update_process_state_rr(struct task_struct *t, struct list_head *dest) 
{
	int global_ticks;
	if (dest == &readyqueue) {
		t->process_state = ST_READY;
		t->estado.remaining_ticks = get_quantum(t);
		global_ticks = get_ticks();
		t->estado.user_ticks += global_ticks - t->estado.elapsed_total_ticks;
		t->estado.elapsed_total_ticks = global_ticks;
		list_add_tail(&t->list, &readyqueue);
	}
	else if (dest == NULL) {
		t->process_state = ST_RUN;
		global_ticks = get_ticks();
		t->estado.ready_ticks += global_ticks - t->estado.elapsed_total_ticks;
		t->estado.elapsed_total_ticks = global_ticks;
	}
	else if (dest == &keyboardqueue) {
		t->process_state = ST_BLOCKED;
		t->estado.remaining_ticks = get_quantum(t);
		global_ticks = get_ticks();
		t->estado.user_ticks += global_ticks - t->estado.elapsed_total_ticks;
		t->estado.elapsed_total_ticks = global_ticks;
		list_add_tail(&t->list, &keyboardqueue);
	}
	//este else es para cuando el destino son las listas de los semaforos
	else {
		t->process_state = ST_BLOCKED;
		t->estado.remaining_ticks = get_quantum(t);
		global_ticks = get_ticks();
		t->estado.user_ticks += global_ticks - t->estado.elapsed_total_ticks;
		t->estado.elapsed_total_ticks = global_ticks;
		list_add_tail(&t->list, dest);
	}
}

void actualizar_system_ticks() {
	struct task_struct *t = current();
	int global_ticks = get_ticks();
	t->estado.system_ticks += global_ticks - t->estado.elapsed_total_ticks;
	t->estado.elapsed_total_ticks = global_ticks;
}

int set_PID() {
	return number_PID++;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");
	

	while(1)
	{
	;
	}
}


void init_idle (void)
{
	
	struct task_struct * pcb;
  	pcb = list_head_to_task_struct(list_first(&freequeue));
	pcb->PID = set_PID();
	allocate_DIR(pcb);
	cont_dir[calculate_DIR(pcb)] = 1;
	pcb->info_key.buffer = NULL;
        pcb->info_key.toread = 0;
	pcb->inici_heap = NULL;
	pcb->bytesHeap = 0;
 	pcb->numPagesHeap = 0;
	set_quantum(pcb, QUANTUM);
	pcb->estado.user_ticks = 0;
	pcb->estado.system_ticks = get_ticks();
	pcb->estado.blocked_ticks = 0;
	pcb->estado.ready_ticks = 0;
	pcb->estado.elapsed_total_ticks = get_ticks();
	pcb->estado.total_trans = 0;
	pcb->estado.remaining_ticks = get_quantum(pcb);

	list_del(list_first(&freequeue));

	

	union task_union * t = (union task_union*)  pcb;
	t->stack[1023] = (int)&cpu_idle;
	t->stack[1022] = 0;
	pcb->kernel_esp= (int) &t->stack[1022];

	idle_task = pcb;
	idle_taskk = t;

}

void writeMSR(int num, int value);

void init_task1(void)
{
	
	
	struct task_struct * pcb;	
  	pcb = list_head_to_task_struct(list_first(&freequeue));
	pcb->PID = set_PID();
	allocate_DIR(pcb);
	cont_dir[calculate_DIR(pcb)] = 1;
	pcb->info_key.buffer = NULL;
        pcb->info_key.toread = 0;
	pcb->inici_heap = NULL;
	pcb->bytesHeap = 0;
 	pcb->numPagesHeap = 0;
	number_PID = 100;
	set_quantum(pcb, QUANTUM);
	pcb->estado.user_ticks = 0;
	pcb->estado.system_ticks = get_ticks();
	pcb->estado.blocked_ticks = 0;
	pcb->estado.ready_ticks = 0;
	pcb->estado.elapsed_total_ticks = get_ticks();
	pcb->estado.total_trans = 0;
	pcb->estado.remaining_ticks = get_quantum(pcb);
	
	list_del(list_first(&freequeue));

	

	set_user_pages(pcb);	
	set_cr3(get_DIR(pcb));

	union task_union * new = (union task_union*)  pcb;
	tss.esp0 = KERNEL_ESP(new);
	writeMSR(0x175, tss.esp0);

	init_task = pcb;
	init_taskk = new;
}


void change_context(int change_esp, int actual_kernel_esp);

void task_switch(union task_union * new){
	struct task_struct * actual = current();
	struct task_struct * pcb = (struct task_struct *) new;

	tss.esp0 = KERNEL_ESP(new);
	writeMSR(0x175, tss.esp0);
	
	page_table_entry * dir_actual= get_DIR(actual);
	page_table_entry * dir_new = get_DIR(pcb);
	if(dir_new != dir_actual) set_cr3(dir_new);//gracias al clone optimizamos el flush!!
	change_context(pcb->kernel_esp, &(actual->kernel_esp));
	
}

int calculate_DIR(struct task_struct *t) {
	int ini, pcb;
	ini = (int)((page_table_entry*) &dir_pages[0]);
	pcb = (int)t->dir_pages_baseAddr;
	return (pcb - ini)/sizeof(dir_pages[0]);
}

void init_sched(){
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);
	INIT_LIST_HEAD(&keyboardqueue);
	nextKey = 0;
	firstKey = 0;
	int i;
	for(i=0; i< NR_TASKS; ++i) {
		list_add_tail(&task[i].task.list, &freequeue);
		cont_dir[i] = 0;
	}
	number_PID = 0;
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

