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
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
static bool validate_user_ptr (const void *ptr);
static void *get_offset_ptr(void *ptr, int n);

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
      exit(*(int *)get_offset_ptr(f->esp, 1));
      break;
    case SYS_EXEC:
      f->eax = exec(*(int *)get_offset_ptr(f->esp, 1));
      break;
    case SYS_WAIT:
      wait(*(int *)get_offset_ptr(f->esp, 1));
      break;
    case SYS_CREATE:
    {
      char* file = (char *)get_offset_ptr(f->esp, 1);
      // if file name is null pointer or empty string, exit
      if(*file == '\0' || validate_user_ptr(file) == false){
        exit(EXIT_ERROR);
        break;
      }
      unsigned initial_size = *(unsigned *)get_offset_ptr(f->esp, 2);
      bool success = create(file, initial_size);
      if(!success)
        exit(EXIT_ERROR);
      break;
    }
    case SYS_REMOVE:
    {
      bool success = remove(*(char *)get_offset_ptr(f->esp, 1));
      if(!success)
        exit(EXIT_ERROR);
      break;
    }
    case SYS_OPEN:
    {
      int fd = open(*(char *)get_offset_ptr(f->esp, 1));
      if(fd == EXIT_ERROR)
        exit(EXIT_ERROR);
      f->eax = fd;
      break;
    }
    case SYS_FILESIZE:
    {
      int fd = *(int *)get_offset_ptr(f->esp, 1);
      f->eax = filesize(fd);
      break;
    }
    case SYS_READ:
    {
      int fd = *(int *)get_offset_ptr(f->esp, 1);
      void *buffer = *(int *)get_offset_ptr(f->esp, 2);
      unsigned length = *(int *)get_offset_ptr(f->esp, 3);
      f->eax = read(fd, buffer, length);
      break;
    }
    case SYS_WRITE:
    {
      int fd = *(int *)get_offset_ptr(f->esp, 1);
      const void *buffer = *(int *)get_offset_ptr(f->esp, 2);
      unsigned length = *(int *)get_offset_ptr(f->esp, 3);
      f->eax = write(fd, buffer, length);
      break;
    }
    case SYS_SEEK:
    {
      int fd = *(int *)get_offset_ptr(f->esp, 1);
      unsigned position = *(int *)get_offset_ptr(f->esp, 2);
      seek(fd, position);
      break;
    }
    case SYS_TELL:
    {
      int fd = *(int *)get_offset_ptr(f->esp, 1);
      f->eax = tell(fd);
      break;
    }
    case SYS_CLOSE:
    {
      int fd = *(int *)get_offset_ptr(f->esp, 1);
      close(fd);
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
    return EXIT_ERROR;

  return child_tid;
}

/* Waits for a child process pid and 
 * retrieves the child's exit status */
int wait(pid_t pid)
{
  return process_wait(pid);
}

/* 
 * Creates a new file called file initially initial_size bytes in size. 
 * Returns true if successful, false otherwise. 
 * */
bool create(const char *file, unsigned initial_size)
{
  lock_acquire(&file_system_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&file_system_lock);
  return success;
}

/* 
* Deletes the file called file. 
* Returns true if successful, false otherwise. 
* A file may be removed regardless of whether it is open or closed, and removing an open file does not close it.
*/
bool remove (const char *file){
  lock_acquire(&file_system_lock);
  bool success = filesys_remove(file);
  lock_release(&file_system_lock);
  return success;
}

/* Opens the file called file. 
* Returns a nonnegative integer handle called a "file descriptor" (fd), 
* or -1 if the file could not be opened. */
int open (const char *file){

  lock_acquire(&file_system_lock);
  struct file *f = filesys_open(file);
  lock_release(&file_system_lock);
  if(f == NULL)
    return EXIT_ERROR;
  
  // add file to fd_table
  struct thread *curr_t = thread_current();
  curr_t->fd_table[curr_t->next_fd] = f;
  curr_t->next_fd++;
  return curr_t->next_fd - 1;
}

/* Returns the size, in bytes, of the file open as fd.*/
int filesize (int fd){
  struct thread *curr_t = thread_current();
  struct file *f = curr_t->fd_table[fd];
  if(f == NULL)
    return EXIT_ERROR;
  return file_length(f);
}


/* Reads size bytes from the file open as fd into buffer. 
Returns the number of bytes actually read (0 at end of file), 
or -1 if the file could not be read (due to a condition other than end of file). 
Fd 0 reads from the keyboard using input_getc(). */
int read (int fd, void *buffer, unsigned size)
{
  int read_size = 0;
  if(fd == STDIN){
    uint8_t *buf = (uint8_t *)buffer;
    for(unsigned i = 0; i < size; i++){
      buf[i] = input_getc();
    }
    read_size = size;
  }
  else if(fd == STDOUT){
    exit(EXIT_ERROR);
    return 0;
  }
  else{
    struct thread *curr_t = thread_current();
    struct file *f = curr_t->fd_table[fd];
    if(f == NULL)
      return EXIT_ERROR;
    lock_acquire(&file_system_lock);
    read_size = file_read(f, buffer, size);
    lock_release(&file_system_lock);
  }
  return read_size;
}

int 
write (int fd, const void *buffer, unsigned length)
{
  int written_size = 0;
  if(fd == STDOUT){
    putbuf(buffer, length);
    written_size = length;
  }
  else if(fd == STDIN){
    exit(EXIT_ERROR);
    return 0;
  }
  else{
    struct thread *curr_t = thread_current();
    struct file *f = curr_t->fd_table[fd];
    if(f == NULL)
      return EXIT_ERROR;
    written_size = file_write(f, buffer, length);
  }
  return written_size;
};

/*
 * Changes the next byte to be read or written in open file fd to 
 * position, expressed in bytes from the beginning of the file. 
 * (Thus, a position of 0 is the file's start.) */
void seek (int fd, unsigned position)
{
  struct thread *curr_t = thread_current();
  struct file *f = curr_t->fd_table[fd];
  if(f == NULL)
    return EXIT_ERROR;
  lock_acquire(&file_system_lock);
  file_seek(f, position);
  lock_release(&file_system_lock);
}

// Returns the position of the next byte to be read or written in open file fd, expressed in bytes from the beginning of the file.
unsigned tell (int fd)
{
  struct thread *curr_t = thread_current();
  struct file *f = curr_t->fd_table[fd];
  if(f == NULL)
    return EXIT_ERROR;
  lock_acquire(&file_system_lock);
  off_t offset = file_tell(f);
  lock_release(&file_system_lock);
  return offset;
}
/* Closes file descriptor fd. 
 * Exiting or terminating a process implicitly closes all 
 * its open file descriptors, as if by calling this function for each one.
 */
void close (int fd)
{
  struct thread *curr_t = thread_current();
  struct file *f = curr_t->fd_table[fd];
  if(f == NULL)
    return EXIT_ERROR;
  lock_acquire(&file_system_lock);
  file_close(f);
  lock_release(&file_system_lock);
}

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
    exit(EXIT_ERROR);

  if(is_kernel_vaddr(ptr))
    exit(EXIT_ERROR);
  
  struct thread *curr_t;
  curr_t = thread_current();
  if(!pagedir_get_page(curr_t->pagedir, ptr))
    exit(EXIT_ERROR);

  return true;
}

static
void *get_offset_ptr(void *ptr, int n)
{
  void *next_ptr = ptr + 4*n;
  validate_user_ptr((void *)next_ptr);
  // Validate case in which part of the value is in valid address space
  validate_user_ptr((void *)(next_ptr + 4));
  return next_ptr;
}