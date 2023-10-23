#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "lib/user/syscall.h"
#include <stdio.h>
#include "lib/stdio.h"
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);
static bool validate_user_ptr (const void *ptr);
static int *get_nth_ptr(const void *ptr, int n);

#define STDOUT STDOUT_FILENO
#define STDIN STDIN_FILENO

void
syscall_init (void) 
{
  lock_init(&file_system_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // validate the stack pointer
  validate_user_ptr(f->esp);

  // retrieve the system call number
  int syscall_num = *(int *)f->esp;

  // switch on the system call number
  switch (syscall_num) {
    case SYS_HALT:
      halt();
      break;
    case SYS_EXIT:
      exit(*(int *)(f->esp + 4));
      break;
    case SYS_EXEC:
      // TODO: implement
      break;
    case SYS_WAIT:
      // TODO: implement
      break;
    case SYS_WRITE:
    {
      // TODO: validate the arguments
      int fd = *(int *)(f->esp + 4);
      const void *buffer = *(int *)(f->esp + 8);
      unsigned length = *(int *)(f->esp + 12);
      f->eax = write(fd, buffer, length);
      break;
    }
    default:
      printf("System call number %d is not implemented\n", syscall_num);
      break; 
  }
}

/* Shutdown pintos */ 
void halt(void)
{
  shutdown_power_off();
}

/* Exit process */
void exit(int status)
{
  struct thread *t = thread_current();
  t->exit_status = status; // set exit status for parent to retrieve
  printf("%s: exit(%d)\n", t->name, status);
  thread_exit();
}

/* Create child process and 
 * runs the executable whose name is given in cmd_line, 
 * passing any given arguments, and 
 * returns the new process's program id (pid) */
pid_t exec(const char *cmd_line)
{
  struct thread *curr_t = thread_current();
  struct thread *child_t;
  struct list_elem *child_elem;

  // execute cmd_args and make a child process
  // printf("## from %s sys exec %s \n", thread_current()->name, cmd_args);
  tid_t child_tid = process_execute(cmd_line);
  if (child_tid == TID_ERROR)
  {
    return child_tid;
  }

  // Check if child_tid is in current threads children.
  for (
      child_elem = list_begin(&curr_t->child_list);
      child_elem != list_end(&curr_t->child_list);
      child_elem = list_next(child_elem))
  {
    child_t = list_entry(child_elem, struct thread, child_elem);
    if (child_t->tid == child_tid)
    {
      break;
    }
  }
  // If child with child_tid was not in list, its not a child of the calling process
  if (child_elem == list_end(&curr_t->child_list))
    return EXIT_FAILURE;

  return child_tid;
}

/* Waits for a child process pid and 
 * retrieves the child's exit status */
int wait(pid_t pid)
{
  return process_wait(pid);
}

int 
write (int fd, const void *buffer, unsigned length)
{
  int written_size = 0;
  if(fd == STDOUT){
    putbuf(buffer, length);
    written_size = length;
  }
  else{
    // TODO: write to file
  }
  return written_size;
};


/*
* Checks if the pointer is a valid user pointer
* User pointer is valid if it is not null pointer, 
* a pointer to kernel virtual address space (above PHYS_BASE)
* or a pointer to unmapped virtual memory, 
*/
static bool
validate_user_ptr (const void *ptr)
{
  if(ptr == NULL)
    exit(EXIT_FAILURE);

  if(is_kernel_vaddr(ptr))
    exit(EXIT_FAILURE);
  
  struct thread *curr_t;
  curr_t = thread_current();
  if(!pagedir_get_page(curr_t->pagedir, ptr))
    exit(EXIT_FAILURE);

  return true;
}

static
int *get_nth_ptr(const void *ptr, int n)
{
  int *next_ptr = (int *)ptr + 4*n;
  validate_user_ptr((void *)next_ptr);
  // Catch the edge case where just a part of the value is in valid address space
  validate_user_ptr((void *)(next_ptr + 4));
  return next_ptr;
}