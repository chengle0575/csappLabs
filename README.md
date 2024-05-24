# TAKE AWAY







## **SHELL LAB:**

1. **Busy waiting waste CPU.** Engage sleep() system call to handle control to OS and let OS reschedule.

   Understand that OS will be activated only by external interruption or system call.

2. **After fork()**, parent and child process:
   ```
   share: open file table[shared by all process],
          v-node table[shared by all process]
   has seperate: memory space(stack,heap),
                 file discriptor table
   ```
   **Understanding of process memory space--->implies that global variable is not shared between parent and child process**
  
4. **Duplicate wait() cause problem.**

   Wait() waits for stage changes of a process by checking the 'change bit'.
  ```
  **Reap child process**
  process stage change--
  --->OS keeps the indo about the process arount its parent--
  ---->parent collect the infomation by wait()--
  ------>OS fully clean up the child process[means child process no longer exist]
  ```

4. Reap child process

   **zombie process: finished execution, not consume any resources anymore, but still remain entry in the process table.**

                   Because the parent process not having read its exit status via wait().

                   can lead to resouce leak if happen often.
      

7. Signal is not queued.
   
   **Each [type] of signal corresponds to one bit.** Once signal type K is pending, new coming signal type K is discarded.

   Block a signal means the sigal will be pending but not delivered until unblock.(delay the handle of signal)
   ```
   send a signal---> pending the signal(the bit of this sig type is changed)--->delivered/recieved the signal(the bit is cleared and the handler is called)
   ```

  
8. Each job is a process group. In this way, we can operate all of them at once.
   
   The shell is in a separate group.
  ```
  SHELL[group1]----JOB1[group2]:process A, process a1, process a2
               ----JOB2[group3]:process B, process b1, process b2...
  ```
7. Pay attention to race condition and concurrent.
   
   Concurrent! cannot assume which one happens first.
   
   After fork(), parent and child process are concurrent.
   
   The signal handler runs concurrent with the main program.

8. SIGNAL handler will be inheritaged from parent process.

   However, when use **exec()  system call** to execute a new program. **All memory of current child process will be replaced by the new program's memory.** This implies that the handler, data, variable.... are different now!
