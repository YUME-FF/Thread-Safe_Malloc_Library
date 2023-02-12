# Thread-Safe Malloc
This is Zhe Fan's ECE650 Project Repository.

# Table of Contents

- [Thread-Safe Malloc](#Thread-Safe-Malloc)
  - [Implementation Description](#Implementation-Description)
    - [Lock-Based](#Lock-Based)
    - [Non-Lock-Based](#Non-Lock-Based)
  - [Performance Result Presentation and Analysis](#Performance-Result-Presentation-and-Analysis)

## Thread-Safe Malloc

### Implementation Description

For this assignment, I implemented my own version of two different thread-safe versions (i.e. safe for concurrent access by different threads of a process) of the malloc() and free() functions. This is to solve problems like race conditions, which means that incorrect parallelcode may (sometimes often) result in correct execution due to the absence of certain timingconditions in which the bugs can manifest.

#### Lock-Based
For Lock-Based version, we should put lock before and after malloc() and free(), function pthread\_mutex\_lock() will prevent running simultaneously, which allows concurrency for malloc() and free().

#### Non-Lock-Based
For Non-Lock-Based version, we only put lock before and after sbrk(). And each time we use a new list to represent the free chunk, which means every free chunk is independent on each other, without overlapping memory region.

### Performance Result Presentation and Analysis

| *Average for 50 times tests|    Lock     | Non-Lock |
| --| ----------- | ----- |
| Execution Time(s) | 0.109     | 0.178  |
| *Data segment size(bytes)| 43251424 | 44433824 |


For Execution Time, Non-Lock is faster than Lock. According to Section 1, Non-Lock only lock sbrk(), but all other operations will happen simultaneously. However, for Lock-version, it locks the malloc() and free() which makes less code run simultaneously. Therefore, Lock-Based is faster.
	
For Data segment size, Lock-Based and Non-Lock-Based have close size, which means they behavior similarly in find free chunk.. Properly handle everything received based on game rules.
