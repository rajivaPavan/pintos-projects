# PROJECT 2: USER PROGRAMS DESIGN DOCUMENT

By R. P. Pitiwaduge 210479L (rajiva.21@cse.mrt.ac.lk)

## ARGUMENT PASSING

###  ALGORITHMS 

> A2: Briefly describe how you implemented argument parsing.  

When a new process is created using the `process_execute()` function, the name of the thread to be created is extracted from the filename parameter. This is done by using the `strtok_r()` function to tokenize the filename parameter and extract the first token, which represents the name of the executable file. The extracted name is then used as the name of the new thread that will be created to run the executable.

Moving on to the `start_process()` function; Technically, the parameter `file_name_` in `static void start_process (void *file_name_)` consists of the file name of the executable and arguements passed along with it. Therefore, it was renamed to `command_` which was a more suitable variable name.

> How do you arrange for the elements of argv[] to be in the right order?

The function written to handle stacking arguments: `stack_arguments()` function takes an array of strings `args`, the number of arguments `argc`, and a pointer to the stack pointer `esp`.

The function first calculates the total size of the arguments by iterating over the `args` array and adding the length of each argument plus one for the null terminator. It then allocates an array of pointers `arg_ptr` to store the address of each argument on the stack.

The function then iterates over the args array in reverse order and saves each argument on the stack using `memcpy()`. It also saves the address of each argument on the stack in the `arg_ptr` array.

After saving the arguments on the stack, the function aligns the stack pointer to a multiple of 4 bytes by adding padding if necessary. It then adds a null pointer to the stack to mark the end of the argument list.

The function then iterates over the 	 array in reverse order and saves the address of each argument on the stack using `*(int *)*esp = (unsigned)arg_ptr[i - 1]`.

The function then saves the address of the pointer to the array of pointers to arguments on the stack using `memcpy()`. It also saves the number of arguments on the stack using `memcpy()`.

Finally, the function saves a fake return address on the stack and frees the memory allocated for the `arg_ptr` array.

> How do you avoid overflowing the stack page?

Not handled

### RATIONALE

> A3: Why does Pintos implement strtok_r() but not strtok()?

Unlike `strtok`, `strtok_r` is thread safe.

> A4: In Pintos, the kernel separates commands into a executable name
and arguments.  In Unix-like systems, the shell does this
separation.  Identify at least two advantages of the Unix approach.

Memory used by the kernel to seperate the commands into executable name and arguments is saved, when this is handled by the shell.

## SYSTEM CALLS

### DATA STRUCTURES

- thread.h

```c
struct thread
{
	...

	/* Used for processes */
    struct list child_list;             /* List of child threads */
    struct list_elem child_elem;        /* List element for child threads list */
    int exit_status;                    /* For parent process that wait for this. This is set when exit system call is made */
    struct semaphore pre_exit_sema;     /* Semaphore for parent process to wait for child to begin exiting and set its exit_status */
    struct semaphore post_exit_sema;    /* Semaphore for child to wait for parent to get exit status */
    struct semaphore file_load_sema;    /* Semaphore for parent to wait for child to load file */
    bool load_success;                  /* True if child successfully loaded file */

    /* Used for file system */
    struct file **fd_table;             /* File descriptor table */
    int next_fd;                        /* Next available file descriptor */
    struct file *running_file;          /* Running file of process */

	...
}
```

The above code snippet shows the data structures used for process and file system calls. Each attribute is explained in the comments.


- syscall.h
```c
/* 
 * File system lock
 * Used to synchronize file system operations
 */
struct lock file_system_lock;   
```
The above code snippet shows the data structure used to synchronize file system operations.

> B2: Describe how file descriptors are associated with open files. Are file descriptors unique within the entire OS or just within a single process?

The `struct thread` has a member
```c
struct file **fd_table;
``` 
 which is an array of pointers to `struct file*` pointers. The file descriptor is the index of the array. Therefore, file descriptors are unique within a single process.

### ALGORITHMS 

> B3: Describe your code for reading and writing user data from the
kernel.

The read and write functions in `syscall.c` are used to read and write user data from the kernel.

The read function takes three arguments: `fd`, `buffer`, and `size`. The `fd` argument specifies the file descriptor to read from. If `fd` is `STDIN`, the function reads from the keyboard using `input_getc()`. Otherwise, the function looks up the file associated with the file descriptor in the current thread's file descriptor table and reads from the file using `file_read()`. The function returns the number of bytes actually read, or -1 if an error occurs.

The write function takes three arguments: `fd`, `buffer`, and `length`. The `fd` argument specifies the file descriptor to write to. If `fd` is STDOUT, the function writes to the console using `putbuf()`. Otherwise the function looks up the file associated with the file descriptor in the current thread's file descriptor table and writes to the file using `file_write()`. The function returns the number of bytes actually written, or -1 if an error occurs.

Both functions are given a valid buffer, validated using `validate_user_buffer()` function to check that the buffer argument has a valid user pointers. This ensures that the kernel does not read or write to invalid memory locations.

The `read()` and `write()` function also uses the `file_system_lock` lock to ensure that file reads are performed atomically. This prevents multiple threads from reading from the same file at the same time and corrupting the file data.

> B4: Suppose a system call causes a full page (4,096 bytes) of data to be copied from user space into the kernel.  What is the least and the greatest possible number of inspections of the page table (e.g. calls to pagedir_get_page()) that might result?  What about for a system call that only copies 2 bytes of data?  Is there room for improvement in these numbers, and how much?

> B5: Briefly describe your implementation of the "wait" system call and how it interacts with process termination.

The `wait()` system call takes a single argument: `pid` and directly calls the `process_wait()` function. This function first looks up the thread associated with the given `pid` in the current thread's child list. If the thread is not found, the function returns -1. Otherwise, the function removes the child thread list element from the child list and waits for the child thread to begin exiting by waiting on the child thread's `pre_exit_sema`. The function then returns the child thread's exit status. The child thread's exit status is set when the child thread calls `exit()`. The child thread waits for the parent to get the exit status by waiting on the `post_exit_sema`. The child thread then exits by calling `thread_exit()`. The parent thread then returns the child thread's exit status. 

> B6: Any access to user program memory at a user-specified address can fail due to a bad pointer value.  Such accesses must cause the process to be terminated.  System calls are fraught with such accesses, e.g. a "write" system call requires reading the system call number from the user stack, then each of the call's three arguments, then an arbitrary amount of user memory, and any of these can fail at any point.  This poses a design and error-handling problem: how do you best avoid obscuring the primary function of code in a morass of error-handling?  Furthermore, when an error is detected, how do you ensure that all temporarily allocated resources (locks, buffers, etc.) are freed?  In a few paragraphs, describe the strategy or strategies you adopted for managing these issues.  Give an example.

- Validation of User Pointers:

The `validate_user_ptr()` function checks whether a given pointer is valid or not. It ensures that the pointer is not a null pointer, not pointing to kernel virtual address space, and not pointing to unmapped virtual memory. This validation is crucial because it prevents unauthorized access to the kernel's memory space.

- Validation of User Buffers and Strings:

The `validate_user_buffer()` function and validate_user_string function check the validity of user buffers and strings, respectively. These checks make sure that the entire range of memory used for reading or writing data is valid.

- Handling Bad Pointers:

If a bad pointer is detected during a system call, the validate_user_ptr function ensures that the process is terminated with an error status (usually `EXIT_ERROR`). 

- Specific System Calls:

The code implements specific error-checking for various system calls such as exec, wait, create, remove, open, read, write, and others. These checks ensure that the inputs are valid, and if not, they lead to process termination with an error status.

Certainly, let's take the `SYS_WRITE` system call as an example of how the code manages issues related to user memory access and error handling:

```c
case SYS_WRITE:
{
  int* fd = (int *)get_offset_ptr(f->esp, 1);
  const void *buffer = *(void **)get_offset_ptr(f->esp, 2);
  unsigned* length = (unsigned *)get_offset_ptr(f->esp, 3);
  if(!validate_user_buffer(buffer, *length)){
    exit(EXIT_ERROR);
    break;
  }
  f->eax = write((int)*fd, buffer, (unsigned)*length);
  break;
}
```

In this example:

1. The system call number is checked, and the code executes the `SYS_WRITE` case.

2. The code retrieves the file descriptor (`fd`), buffer, and length of data to be written from the user stack using `get_offset_ptr`.

3. It then calls `validate_user_buffer(buffer, *length)` to check if the provided buffer is valid. This is essential to ensure that the code doesn't write to or read from unauthorized or invalid memory locations.

4. If `validate_user_buffer` returns `false`, indicating a bad pointer, the `exit(EXIT_ERROR)` function is called. This causes the process to be terminated with an error status, preventing any further execution.

5. If the buffer is valid, the code proceeds to call the `write` function, passing the file descriptor, buffer, and length as arguments.

Consider take the SYS_WRITE system call as an example of how the code manages issues related to user memory access and error handling.

```c
case SYS_WRITE:
{
  int* fd = (int *)get_offset_ptr(f->esp, 1);
  const void *buffer = *(void **)get_offset_ptr(f->esp, 2);
  unsigned* length = (unsigned *)get_offset_ptr(f->esp, 3);
  if(!validate_user_buffer(buffer, *length)){
    exit(EXIT_ERROR);
    break;
  }
  f->eax = write((int)*fd, buffer, (unsigned)*length);
  break;
}
```
1. The system call number is checked, and the code executes the `SYS_WRITE` case.

2. The code retrieves the file descriptor (`fd`), `buffer`, and length of data to be written from the user stack using `get_offset_ptr`.

3. It then calls `validate_user_buffer(buffer, *length)` to check if the provided buffer is valid. This is essential to ensure that the code doesn't write to or read from unauthorized or invalid memory locations.

4. If `validate_user_buffer` returns false, indicating a bad pointer, the exit(EXIT_ERROR) function is called. This causes the process to be terminated with an error status, preventing any further execution.

5. If the buffer is valid, the code proceeds to call the write function, passing the file descriptor, buffer, and length as arguments.

### SYNCHRONIZATION

> B7: The "exec" system call returns -1 if loading the new executable fails, so it cannot return before the new executable has completed loading.  How does your code ensure this?  How is the load success/failure status passed back to the thread that calls "exec"?

- Synchronization Mechanism:

The exec system call uses a semaphore named `file_load_sema` to synchronize between the parent and child threads.
The parent thread waits for the child to signal successful loading using `sema_down(&child_t->file_load_sema)`.
If loading fails, the semaphore count remains at 0, blocking the parent thread.

- Passing Load Status Back:

The child process sets a `load_success` flag to indicate loading success.
The parent thread checks this flag to determine loading status.
If `load_success` is `true`, exec returns the child's PID.
If `load_success` is `false`, exec returns -1 to indicate failure.

> B8: Consider parent process P with child process C.  How do you ensure proper synchronization and avoid race conditions when P calls wait(C) before C exits?  After C exits?  How do you ensure that all resources are freed in each case?  How about when P terminates without waiting, before C exits?  After C exits?  Are there any special cases?

The above synchronization problem is solved using the two semaphores `pre_exit_sema` and `post_exit_sema`. The `pre_exit_sema` is used to ensure that the parent thread waits for the child thread to begin exiting. The `post_exit_sema` is used to ensure that the child thread waits for the parent thread to get the exit status. The `exit_status` attribute of the child thread is set when the child thread calls `exit()`. The child thread waits for the parent to get the exit status by waiting on the `post_exit_sema`. The child thread then exits by calling `thread_exit()`. The parent thread then returns the child thread's exit status.

### RATIONALE

> B9: Why did you choose to implement access to user memory from the kernel in the way that you did?

The `validate_user_ptr()` function checks whether a given pointer is valid or not. It ensures that the pointer is not a null pointer, not pointing to kernel virtual address space, and not pointing to unmapped virtual memory. This validation is crucial because it prevents unauthorized access to the kernel's memory space. This method was chosen because it is simple and efficient.

>B10: What advantages or disadvantages can you see to your design
>for file descriptors?

Using just `struct file**` is simple to use and understand. However, it is not very efficient as the array has to be resized when the number of open files exceeds the size of the array. This can be improved by using a hash table instead of an array.

> B11: The default tid_t to pid_t mapping is the identity mapping. If you changed it, what advantages are there to your approach?

Not changed

---