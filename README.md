# paldOs

paldOs is an operating system based on Nachos.

## Nachos
**Not Another Completely Heuristic Operating System**, or **Nachos**, is instructional software for teaching undergraduate, and potentially graduate level operating systems courses. It was developed at the University of California, Berkeley, designed by Thomas Anderson, and is used by numerous schools around the world.

## What paldOs offer:
### Thread managment and concurrency
Starting from semaphores provided by Nachos we implemented other tools for thread managment and concurrency.
- Semaphores
- Locks
- Condition variables
- Channels

### Virtual memory
We implemented some virtual memory upgrades:
- TLB
- Demand Loading
- Swap
- Page Replacement Policy: LRU, FIFO

### Scheduler
- Multilevel priority queue

### Filesystem
Nachos provides a very limited file system and we build around that a lot of nice features:
- Larger and extensible files (Up to the disk space)
- Directory hierarchy

### Userland
Some of the programs that you can run as an user are:
- ls
- cd
- touch
- cp
- mkdir
- rm
- rmdir
- cat
- echo
- cat>
- exit
- help

> Note: the final version of the nachOS with all the mentioned features is in the filesys/ folder.

# Install

**Dependencies**

gcc-mipsel-linux-gnu

**Compiling**

Just write `make` in the repository path.

# Using paldOs

## Testing

For all this tests you can give a random seed to the clock for 'random' (but repeatable) interrupts to occur with the `rs` flag, for example:

`$ threads/nachos -rs 1000`

`$ filesys/nachos -rs 829`

### Threads

Run:

`threads/nachos -tt`

to see some threads tests available:

```terminal
$ threads/nachos -tt
Available tests:
(0) : Simple thread interleaving test.
(1) : Ornamental garden test.
(2) : Ornamental garden test (with semaphores).
(3) : Ornamental Garden test (with locks).
(4) : Channel test.
(5) : Multilevel priority queue test.
Choose a test to run:
```

### Filesys

Run:

`filesys/nachos -f` to format the disk.

`filesys/nachos -D` to view the disk content.

Filesystem test:

`filesys/nachos -bcp filesys/test/sobigthatdoesnotfit big` to copy a very large file in the linux fs to the 'big' file in the paldOs filesystem.

`filesys/nachos -tf` performance test for the filesystem:

- Create a file, open it, write a bunch of chunks of bytes, read a bunch of chunks of bytes, close the file and remove the file.

`filesys/nachos -tfs` concurrent performance test for the filesystem:
- Create, write, read, close and remove but do it concurrently.

`filesys/nachos -tfc` create a lot of files to fit all the possible sectors in the disk.

## Further reading
- https://en.wikipedia.org/wiki/Not_Another_Completely_Heuristic_Operating_System
- https://homes.cs.washington.edu/~tom/nachos/
