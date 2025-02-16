# Simple Thread Scheduler (SUT Library)

## Overview
The Simple Thread Scheduler (SUT) library provides a many-to-many user-level threading system with a simple First Come First Serve (FCFS) scheduler. It includes compute and I/O executors to handle tasks efficiently without blocking. The SUT library allows tasks (C functions) to run cooperatively, supporting asynchronous I/O operations.

This project demonstrates a lightweight threading library with features to manage compute and I/O-bound tasks, showcasing cooperative multitasking, context switching, and efficient task management.

## Features and Specifications

### Core Features
- **User-Level Threading**: Tasks execute within the same process, sharing memory, and are managed using a task control block (TCB).
- **Compute Executor (C-EXEC)**: Dedicated to compute tasks, handling task creation, scheduling, and execution.
- **I/O Executor (I-EXEC)**: Manages asynchronous I/O operations, preventing blocking of compute executors.
- **First Come First Serve Scheduling**: Tasks are scheduled based on their order in the ready queue.
- **Asynchronous I/O**: Tasks performing I/O operations are queued and resumed upon completion.

### SUT Library API

#### 1. `void sut_init()`
Initializes the SUT library, creating kernel-level threads for the compute and I/O executors.

#### 2. `bool sut_create(sut_task_f fn)`
Creates a new task. Takes a C function `fn` as the task body and adds it to the task ready queue.
- **Return Value**: Returns `true` on success, `false` otherwise.

#### 3. `void sut_yield()`
Pauses the currently running task and places it at the back of the task ready queue.

#### 4. `void sut_exit()`
Terminates the current task without placing it back in the ready queue.

#### 5. `int sut_open(char *fname)`
Requests the I/O executor to open the specified file.
- **Return Value**: Returns a file descriptor on success, or a negative value on failure.

#### 6. `void sut_write(int fd, char *buf, int size)`
Writes data from the buffer `buf` to the file specified by `fd`.

#### 7. `char *sut_read(int fd, char *buf, int size)`
Reads data from the file specified by `fd` into a pre-allocated buffer `buf`.
- **Return Value**: Returns the buffer on success, or `NULL` on failure.

#### 8. `void sut_close(int fd)`
Closes the file specified by `fd` using the I/O executor.

#### 9. `void sut_shutdown()`
Cleans up resources, ensures all executors terminate gracefully, and shuts down the threading library.

## Architecture
The SUT library is structured into two key executors:
- **Compute Executor (C-EXEC)**: Handles task creation, scheduling, and execution. It manages the task ready queue and ensures tasks run until completion, yielding, or exiting.
- **I/O Executor (I-EXEC)**: Manages asynchronous I/O requests, utilizing a request and wait queue to handle operations without blocking.

### Task Life Cycle
1. **Task Creation**: A task is created using `sut_create()`, which initializes a TCB and places the task in the ready queue.
2. **Task Execution**: The C-EXEC selects tasks from the ready queue and executes them until they yield or exit.
3. **I/O Operations**: I/O tasks are queued for the I-EXEC, which processes them asynchronously and returns results.
4. **Task Termination**: Tasks terminate either voluntarily (via `sut_exit()`) or upon completing their operation.

### Key Data Structures
- **Task Control Block (TCB)**: Contains task metadata, including stack, function pointer, and state.
- **Ready Queue**: Stores tasks waiting for execution.
- **Wait Queue**: Holds tasks awaiting I/O operations.
- **Request Queue**: Buffers I/O requests for the I-EXEC.

## Optimization Details

### 1. Context Switching
- Utilized `makecontext()` and `swapcontext()` for efficient context management.
- Optimized the storage and restoration of task states using lightweight TCBs.

### 2. Asynchronous I/O
- I/O operations are offloaded to the I-EXEC, ensuring compute tasks remain unaffected by slow I/O operations.
- Non-blocking operations improve throughput and responsiveness.

### 3. Task Scheduling
- Implemented FCFS scheduling for simplicity and fairness.
- Prevented CPU overutilization by introducing nanosleep intervals when the ready queue is empty.

### 4. Resource Management
- Graceful shutdown using `sut_shutdown()` ensures all threads terminate cleanly and resources are deallocated.

## Edge Cases and Considerations

### 1. Task Starvation
- Ensured fairness in task scheduling by adhering strictly to FCFS order.

### 2. I/O Buffer Overflow
- Validated buffer sizes during `sut_read()` and `sut_write()` operations to prevent memory corruption.

### 3. Deadlocks
- Implemented safeguards to prevent deadlocks when multiple tasks access shared resources.

### 4. Missing Files
- Handled file not found errors gracefully during `sut_open()` by returning negative values.

### 5. Executor Synchronization
- Coordinated between the C-EXEC and I-EXEC to prevent race conditions during queue access.

### 6. Task Termination During I/O
- Ensured tasks exiting during an I/O operation clean up their resources without disrupting the I-EXEC.

## Testing and Debugging

### Test Cases
- **Basic Compute Tasks**: Verified task creation, yielding, and termination.
- **Nested Task Creation**: Tested tasks creating additional tasks dynamically.
- **Asynchronous I/O**: Validated non-blocking reads, writes, and file operations.
- **Stress Testing**: Simulated scenarios with 30+ tasks performing compute and I/O operations.

### Debugging Tools
- **Logging**: Incorporated detailed logs for task state transitions and I/O operations.
- **GDB**: Used to debug context switching and thread synchronization issues.

## How to Run

1. **Initialize the Library**:
   ```c
   sut_init();
   ```

2. **Create Tasks**:
   ```c
   sut_create(task_function);
   ```

3. **Perform I/O Operations**:
   ```c
   int fd = sut_open("file.txt");
   sut_write(fd, buffer, size);
   sut_close(fd);
   ```

4. **Shutdown**:
   ```c
   sut_shutdown();
   ```

## Conclusion
The Simple Thread Scheduler provides a robust platform for user-level threading with efficient task management and asynchronous I/O handling. Its modular design and adherence to cooperative multitasking principles make it a valuable tool for understanding threading concepts and building lightweight threading solutions.
