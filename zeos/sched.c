/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

#define QUANTUM 100

struct task_struct *idle_task=NULL;

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));


struct task_struct *list_head_to_task_struct(struct list_head *l)
{
	return (int)l&0xFFFFF000;
  //return list_entry( l, struct task_struct, list);
}


extern struct list_head blocked;
extern struct list_head freequeue;
extern struct list_head readyqueue;
int number_PID;


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

int get_quantum(struct task_struct *t){
	return t->quantum;
}

void set_quantum(struct task_struct *t, int new_quantum)
{
	t->quantum = new_quantum;
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

int get_PID(){
	return number_PID;
}

int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
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
	struct list_head * e = list_first( &freequeue );
	list_del( e );
	struct task_struct *c = list_head_to_task_struct(e);
	union task_union *uni = (union task_union *) c;
	c->PID = 0;
	allocate_DIR(e);
	uni->stack[KERNEL_STACK_SIZE-1]=(unsigned long)&cpu_idle; /* Return address */
  	uni->stack[KERNEL_STACK_SIZE-2]=0; /* register ebp */
	c->register_esp=(int)&(uni->stack[KERNEL_STACK_SIZE-2]); /* top of the stack */
	set_quantum(c, QUANTUM);
	c->estado.user_ticks = 0;
	c->estado.system_ticks = get_ticks();
	c->estado.blocked_ticks = 0;
	c->estado.ready_ticks = 0;
	c->estado.elapsed_total_ticks = get_ticks();
	c->estado.total_trans = 0;
	c->estado.remaining_ticks = get_quantum(c);
	idle_task=c;
}

void init_task1(void){
	struct list_head * e = list_first( &freequeue );
	list_del( e );
	struct task_struct *c = list_head_to_task_struct(e);
	union task_union *uni = (union task_union *) c;
	c->PID = set_PID();
	allocate_DIR(c);
	set_user_pages(c);
	tss.esp0=(DWord)&(uni->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175,(DWord)&(uni->stack[KERNEL_STACK_SIZE]));
	set_cr3(c->dir_pages_baseAddr);
	number_PID = 100;
	set_quantum(c, QUANTUM);
	c->estado.user_ticks = 0;
	c->estado.system_ticks = get_ticks();
	c->estado.blocked_ticks = 0;
	c->estado.ready_ticks = 0;
	c->estado.elapsed_total_ticks = get_ticks();
	c->estado.total_trans = 0;
	c->estado.remaining_ticks = get_quantum(c);
}


void init_sched(){
	INIT_LIST_HEAD(&freequeue);
	INIT_LIST_HEAD(&readyqueue);
	for (int i = 0;i<NR_TASKS;++i){
		list_add_tail(&(task[i].task.list),&freequeue);
	}
	number_PID=0;
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

void change_context(int change_esp,int actual_kernel_esp);

void task_switch(union task_union*t){
	struct task_struct *actual = current();
	struct task_struct *pcb = (struct task_struct *) t;

	tss.esp0 = KERNEL_ESP(t);
	writeMSR(0x175,tss.esp0);

	page_table_entry *dir_actual = get_DIR(actual);
	page_table_entry *dir_new = get_DIR(t);
	//set_cr3(dir_new);
	change_context(pcb,&(actual));
}
