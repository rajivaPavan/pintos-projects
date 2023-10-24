		     +--------------------------+
       	     |		    CS 140		    |
		     | PROJECT 2: USER PROGRAMS	|
		     | 	   DESIGN DOCUMENT     	|
		     +--------------------------+

R. P. Pitiwaduge 210479L rajiva.21@cse.mrt.ac.lk

			   ARGUMENT PASSING
			   ================

---- ALGORITHMS ----

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

---- RATIONALE ----

> A3: Why does Pintos implement strtok_r() but not strtok()?

Unlike `strtok`, `strtok_r` is thread safe.

> A4: In Pintos, the kernel separates commands into a executable name
and arguments.  In Unix-like systems, the shell does this
separation.  Identify at least two advantages of the Unix approach.

Memory used by the kernel to seperate the commands into executable name and arguments is saved, when this is handled by the shell.

			     SYSTEM CALLS
			     ============

---- DATA STRUCTURES ----

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

> B2: Describe how file descriptors are associated with open files.
> Are file descriptors unique within the entire OS or just within a
> single process?

The `struct thread` has a member
```c
struct file **fd_table;
``` 
 which is an array of pointers to `struct file*` pointers. The file descriptor is the index of the array. Therefore, file descriptors are unique within a single process.

---- ALGORITHMS ----

> B3: Describe your code for reading and writing user data from the
kernel.

The read and write functions in `syscall.c` are used to read and write user data from the kernel.

The read function takes three arguments: `fd`, `buffer`, and `size`. The `fd` argument specifies the file descriptor to read from. If `fd` is `STDIN`, the function reads from the keyboard using `input_getc()`. Otherwise, the function looks up the file associated with the file descriptor in the current thread's file descriptor table and reads from the file using `file_read()`. The function returns the number of bytes actually read, or -1 if an error occurs.

The write function takes three arguments: `fd`, `buffer`, and `length`. The `fd` argument specifies the file descriptor to write to. If `fd` is STDOUT, the function writes to the console using `putbuf()`. Otherwise the function looks up the file associated with the file descriptor in the current thread's file descriptor table and writes to the file using `file_write()`. The function returns the number of bytes actually written, or -1 if an error occurs.

Both functions are given a valid buffer, validated using `validate_user_buffer()` function to check that the buffer argument has a valid user pointers. This ensures that the kernel does not read or write to invalid memory locations.

The `read()` and `write()` function also uses the `file_system_lock` lock to ensure that file reads are performed atomically. This prevents multiple threads from reading from the same file at the same time and corrupting the file data.

>> B4: Suppose a system call causes a full page (4,096 bytes) of data
>> to be copied from user space into the kernel.  What is the least
>> and the greatest possible number of inspections of the page table
>> (e.g. calls to pagedir_get_page()) that might result?  What about
>> for a system call that only copies 2 bytes of data?  Is there room
>> for improvement in these numbers, and how much?

>> B5: Briefly describe your implementation of the "wait" system call
>> and how it interacts with process termination.

>> B6: Any access to user program memory at a user-specified address
>> can fail due to a bad pointer value.  Such accesses must cause the
>> process to be terminated.  System calls are fraught with such
>> accesses, e.g. a "write" system call requires reading the system
>> call number from the user stack, then each of the call's three
>> arguments, then an arbitrary amount of user memory, and any of
>> these can fail at any point.  This poses a design and
>> error-handling problem: how do you best avoid obscuring the primary
>> function of code in a morass of error-handling?  Furthermore, when
>> an error is detected, how do you ensure that all temporarily
>> allocated resources (locks, buffers, etc.) are freed?  In a few
>> paragraphs, describe the strategy or strategies you adopted for
>> managing these issues.  Give an example.

---- SYNCHRONIZATION ----

>> B7: The "exec" system call returns -1 if loading the new executable
>> fails, so it cannot return before the new executable has completed
>> loading.  How does your code ensure this?  How is the load
>> success/failure status passed back to the thread that calls "exec"?

>> B8: Consider parent process P with child process C.  How do you
>> ensure proper synchronization and avoid race conditions when P
>> calls wait(C) before C exits?  After C exits?  How do you ensure
>> that all resources are freed in each case?  How about when P
>> terminates without waiting, before C exits?  After C exits?  Are
>> there any special cases?

---- RATIONALE ----

>> B9: Why did you choose to implement access to user memory from the
>> kernel in the way that you did?


>B10: What advantages or disadvantages can you see to your design
>for file descriptors?

Using just `struct file**` is simple to use and understand. However, it is not very efficient as the array has to be resized when the number of open files exceeds the size of the array. This can be improved by using a hash table instead of an array.

>> B11: The default tid_t to pid_t mapping is the identity mapping.
>> If you changed it, what advantages are there to your approach?


---