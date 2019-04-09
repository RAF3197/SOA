#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */

  char text [10];
  fork();

  itoa(getpid(),text);
  write(1,text,sizeof(text));
  fork();

  itoa(getpid(),text);
  write(1,text,sizeof(text));
    
  while(1) {
   }
}
