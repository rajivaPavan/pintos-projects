		     +--------------------------+
       	     |		    CS 140		    |
		     | PROJECT 2: USER PROGRAMS	|
		     | 	   DESIGN DOCUMENT     	|
		     +--------------------------+

R. P. Pitiwaduge 210479L rajiva.21@cse.mrt.ac.lk

---- PRELIMINARIES ----

>> If you have any preliminary comments on your submission, notes for the
>> TAs, or extra credit, please give them here.

>> Please cite any offline or online sources you consulted while
>> preparing your submission, other than the Pintos documentation, course
>> text, lecture notes, and course staff.

			   ARGUMENT PASSING
			   ================

---- DATA STRUCTURES ----

>> A1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

---- ALGORITHMS ----

> A2: Briefly describe how you implemented argument parsing.  

When a new process is created using the `process_execute` function, the name of the thread to be created is extracted from the filename parameter. This is done by using the `strtok_r` function to tokenize the filename parameter and extract the first token, which represents the name of the executable file. The extracted name is then used as the name of the new thread that will be created to run the executable.

Moving on to the `start_process` function; Technically, the parameter `file_name_` in `static void start_process (void *file_name_)` consists of the file name of the executable and arguements passed along with it. Therefore, it was renamed to `command_` which was a more suitable variable name.

> How do you arrange for the elements of argv[] to be in the right order?

The `stack_arguments` function is used to set up the user stack for a new process. It takes a string of arguments (`args`) and a pointer to the stack pointer (`esp`) as input.

The function first tokenizes the `args` string using `strtok_r` and stores the tokens in an array called `argv`. It also counts the number of arguments in the `argc` variable.

Next, the function pushes the arguments to the stack in reverse order. It does this by iterating over the `argv` array in reverse order and pushing each argument to the stack using `memcpy` function. It also word-aligns the stack pointer by adding padding bytes if necessary.

After pushing the arguments to the stack, the function pushes a null sentinel to mark the end of the argument list. It then pushes the addresses of the arguments to the stack in reverse order.

Finally, the function pushes the address of the `argv` array to the stack, followed by the value of `argc`. It also pushes a fake return address to the stack.

>> How do you avoid overflowing the stack page?



---- RATIONALE ----

> A3: Why does Pintos implement strtok_r() but not strtok()?

Unlike `strtok`, `strtok_r` is thread safe.

> A4: In Pintos, the kernel separates commands into a executable name
and arguments.  In Unix-like systems, the shell does this
separation.  Identify at least two advantages of the Unix approach.

Memory used by the kernel to seperate the commands into executable name and arguments is saved, when this is handled by ther shell.

			     SYSTEM CALLS
			     ============

---- DATA STRUCTURES ----

>> B1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

>> B2: Describe how file descriptors are associated with open files.
>> Are file descriptors unique within the entire OS or just within a
>> single process?

---- ALGORITHMS ----

>> B3: Describe your code for reading and writing user data from the
>> kernel.

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

>> B10: What advantages or disadvantages can you see to your design
>> for file descriptors?

>> B11: The default tid_t to pid_t mapping is the identity mapping.
>> If you changed it, what advantages are there to your approach?

			   SURVEY QUESTIONS
			   ================

Answering these questions is optional, but it will help us improve the
course in future quarters.  Feel free to tell us anything you
want--these questions are just to spur your thoughts.  You may also
choose to respond anonymously in the course evaluations at the end of
the quarter.

>> In your opinion, was this assignment, or any one of the three problems
>> in it, too easy or too hard?  Did it take too long or too little time?

>> Did you find that working on a particular part of the assignment gave
>> you greater insight into some aspect of OS design?

>> Is there some particular fact or hint we should give students in
>> future quarters to help them solve the problems?  Conversely, did you
>> find any of our guidance to be misleading?

>> Do you have any suggestions for the TAs to more effectively assist
>> students, either for future quarters or the remaining projects?

>> Any other comments?

---