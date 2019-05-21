#include <libc.h>

void print_stats(int pid)
{
	char buff[3000];
	struct stats st;
	int error = get_stats(pid, &st);
	if (error < 0)
		return;
	itoa(pid,buff);
	write(1, "\nStats for: ", strlen("\nStatus for: "));
	write(1,buff,strlen(buff));
	itoa(st.user_ticks, buff);
	write(1,"\nuser_ticks: ",strlen("\nuser_ticks: "));
	write(1,buff,strlen(buff));
	itoa(st.blocked_ticks, buff);
	write(1,"\nblocked_ticks: ",strlen("\nblocked_ticks: "));
	write(1,buff,strlen(buff));
	itoa(st.ready_ticks, buff);
	write(1,"\nready_ticks: ",strlen("\nready_ticks: "));
	write(1,buff,strlen(buff));
	itoa(st.system_ticks, buff);
	write(1,"\nsystem_ticks: ",strlen("\nsystem_ticks: "));
	write(1,buff,strlen(buff));
	write(1,"\n",strlen("\n"));
}

int fibonacci(int f)
{
	if (f <= 1) return 1;
	else
		return fibonacci(f-1)+fibonacci(f-2);
}

// Workload 1: Sample all CPU bursts
void workload_1()
{
  int i=fork();
	if(i!=0)i =fork();
	fibonacci(35);
	print_stats(getpid());
	if (i==0)exit();
}

void workload_2(void)
{
	int i = fork();
	if (i!=0){
		i = fork();
	}
	if (i==0){
		read(0,0,5000);
		print_stats(getpid());	
		exit();
	}
	else {
		read(0,0,5000);
		print_stats(0);	
		exit();
	}
}

void workload_3(void)
{
	 int i=fork();
	if(i!=0)i =fork();

	if (i==0)fibonacci(32);
	else read(0,0,500);
	print_stats(getpid());
	if (i==0)exit();
	else {
		read(0,0,5000);
		print_stats(0);
		exit();
	}
}


char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
    // 0 -> RR
		// 1 -> FCFS
	set_sched_policy(1);
	
	workload_2();
	
	read(0,0,5000);
	print_stats(0);
	while(1) { }
}
