#include <libc.h>

char buff[24];

int pid;
int myVar;

void printMyPid() {
	
	write(1, "clone", 5);
	exit();
}

int add(int par1, int par2) 
{
	return par1+par2;
}

long inner(long n)
{
	int i;
	long suma;
	suma = 0;
	for (i=0; i<n; i++) suma = suma + i;
	return suma;
}

long outer(long n)
{
	int i;
	long acum;
	acum=0;
	for (i=0; i<n; i++) acum = acum + inner(i);
	return acum;
}

int add2(int par1, int par2);
//int write(int fd, char *buffer, int size); NO HACE FALTA YA QUE ESTA EN LIBC.H

int __attribute__ ((__section__(".text.main")))
  main(void)
{
	/*char stack[1024];
	int pid = clone(printMyPid,&stack[1024]);
	char buffer[128];*/
	//read(0,buffer,4);
	int puntero = sbrk(4096);
	while(1) {
		/*int num = read(0,buffer,128);	
		itoa(num, buffer);
		write(1, buffer, 10);*/
	}
	return 0;
}
