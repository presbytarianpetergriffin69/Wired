
# Table of Contents

1.  [Architectural philosophy](#orgf38312c)
    1.  [Hybrid kernel architecture](#org409a73d)
    2.  [Multics-like layering](#orgd21fba3)
    3.  [Portability](#orgf0bc901)
    4.  [Memory management](#orgcf5e475)
    5.  [Scheduling and multitasking](#org3a37d24)
    6.  [Object manager (Ob), the heart of WIREDOS](#orgbaeb4ef)
    7.  [Filesystems](#orgdad1998)
    8.  [IPC and inter-kernel messaging](#org2cde9c3)
    9.  [Drivers and Device model](#orgc8c2aa4)
    10. [Environment subsystems](#orgf0e6b2a)
    11. [Graphical system](#org109add8)
    12. [Networking](#org59930c1)
2.  [Boot process](#orga47c6fe)
3.  [Testing and tooling](#orgf3ca357)
4.  [Coding standards](#org09d948a)
5.  [Long term dreams](#orge67b458)
6.  [Developer roadmap](#orgc8b3d2a)
    1.  [PHASE 1 - BOOTSTRAP AND HARDWARE DISCOVERY](#org8073d40)
        1.  [Bootloader integration](#org03b00a4)
        2.  [CPU and architecture](#org5e102de)
        3.  [ACPI bringup](#org82e086a)
    2.  [PHASE 2 - MEMORY MANAGEMENT](#org98e816b)
        1.  [Physical memory manager](#orge7ae80f)
        2.  [VMM](#org62d696a)
        3.  [Slab allocator](#org1a795f1)
    3.  [PHASE 3 - SCHEDULING AND MULTITASKING](#org8425998)
        1.  [Threading](#org2f3de19)
        2.  [Scheduler](#orgd5471c7)
        3.  [Process model](#org7934965)
    4.  [PHASE 4 - OBJECT MANAGER](#orgfb9aafb)
        1.  [Object types](#org6cb4d0d)
        2.  [Namespaces](#orgc06e390)
        3.  [Kernel objects](#orge58357b)
    5.  [PHASE 5 - IPC SYSTEM](#org243d256)
        1.  [Messaging primitives](#org459e8b0)
        2.  [System server infrastructure](#org57621c3)
    6.  [PHASE 6 - FILESYSTEM](#org8dfece0)
        1.  [VFS architecture](#org463cb14)
        2.  [FAT32 Driver](#org8773797)
        3.  [WFS (Wired Filesystem) (FUTURE GOAL)](#orgdfed874)
    7.  [PHASE 7 - DRIVERS AND DEVICE MODEL](#org42eae6a)
        1.  [Driver framework](#orgf308fa4)
        2.  [Essential drivers](#org8afa34b)
    8.  [PHASE 8 - ENVIRONMENT SUBSYSTEMS](#org7a0d1c0)
        1.  [POSIX](#orgfacd036)
        2.  [WIRED API](#org809c391)
        3.  [FUTURE GOALS (6 - 12 months)](#orgd1fc2a8)
7.  [Conclusion](#org5fd34bb)

WIREDOS is not just another hobby kernel. It is a deliberatly engineered, carefully architectured operating system designed around 4 pillars:

1.  Object centric system design inspired by Windows NT
2.  Strict subsystem isolation
3.  Predictable, inspectable, tiered performance
4.  Portability to other architectures made easy

This document outlines the philosophy and long term goals of the project.


<a id="orgf38312c"></a>

# Architectural philosophy


<a id="org409a73d"></a>

## Hybrid kernel architecture

WIRED uses a micro/hybrid kernel, with the following principles:

-   Core kernel is minimal: scheduler, memory management, IPC, object manager, networking, filesystems (VFS), and core hardware abstraction

-   Everything else, like subsystems, runtimes, high-level drivers, and GUI applications, are isolated in their own subsystems.

-   Userspace is made to act like a collection of trusted servers, with programs running as userspace clients

-   Every subsystem (filesystem drivers, GUI programs, java programs) communicates through defined C interfaces, shared objects, object handles, and kernel IPC channels.


<a id="orgd21fba3"></a>

## Multics-like layering

The OS is structured in predictable layers or rings, similar to Multics:

1.  Hardware abstraction layer
2.  Kernel core (memory, ACPI functions, core system drivers)
3.  Kernel objects + handles
4.  Userspace servers
5.  Environment subsystems (WIRED API, POSIX, JDK)
6.  Userspace and GUI

Every component will be separated by 

-   layer
-   responsibilites
-   dependencies


<a id="orgf0bc901"></a>

## Portability

WIRED is conceptually hardware-agnostic. Currently, the only architecture supported is AMD64, but future targets inclue

-   PowerPC
-   Sparc64
-   DEC Alpha
-   RISC-V

Portability is included as a fun challenge to me to see how many targets I can hit with as little code possible. A HAL will be written to abstract the upper-level code. The HAL will expose:

-   Interrupt controller abstraction (LAPIC, OpenPIC, XICS)
-   Timer abstraction
-   Page table helpers (x86 PAE, PPC hash tables, RISC-V paging)
-   Bootloader handoff
-   Device enumeration
-   Basic system information


<a id="orgcf5e475"></a>

## Memory management

WIRED uses a four-tiered system memory managment architecture:

Kernel heap / slab alloc

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<tbody>
<tr>
</tr>
</tbody>
</table>

Virtual memory management (kernel and usermode address space, .bss, .text)

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<tbody>
<tr>
</tr>
</tbody>
</table>

4 Level PAE Paging

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<tbody>
<tr>
</tr>
</tbody>
</table>

Physical memory management (E820 map, malloc() free() functions, frame allocator)

Memory is tracked as kernel objects, not raw pointers.


<a id="org3a37d24"></a>

## Scheduling and multitasking

WIRED uses a preemptive, priority based round robin scheduler, modeled after 

-   UNIX system V
-   FreeBSD and Solaris' early schedulers
-   Windows NT kernel dispatcher

Core features include

-   Time quantum slices
-   Kernel threads and user threads
-   Thread pools
-   Process groups
-   Per-CPU run queues
-   Load balancing between cores
-   System clock ticks
-   Mutex / spinlocking


<a id="orgbaeb4ef"></a>

## Object manager (Ob), the heart of WIREDOS

Heavily inspired by the Windows NT Object manager.

**Every resource is an object**

Examples:

-   Files
-   Mutexes
-   Threads
-   Processes
-   Timers
-   Sockets
-   Shared memory segments
-   Symbolic links
-   Registry keys and nodes

All objects share:

-   A type
-   A vtable
-   A reference count
-   A namespace path

The namespaces provided are

\System\\
\Devices\\
\Processes\\
\Handles\\
\Registry\\
\POSIX\\
\WIREDAPI\\


<a id="orgdad1998"></a>

## Filesystems

Immediate targets for filesystems include

-   FAT32 (for EFI)
-   Simple VFS layer
-   UFS (short term)

Eventually, I will implement the WIRED filesystem (WFS), which will include

-   Master file table
-   Journaling
-   Disk Access Control Lists
-   Checksumming
-   Every file is an object


<a id="org2cde9c3"></a>

## IPC and inter-kernel messaging

WIRED will include a flexible ICP model

-   Message queues
-   Ports
-   Channels
-   Shared memory with synchronization


<a id="orgc8c2aa4"></a>

## Drivers and Device model

Drivers must:

-   Live in userspace (when possible)
-   Use a stable ABI exposed by the Ob
-   Be hot-reloadable
-   Use event driven models (interrupt -> message -> driver thread)

Driver categories include:

-   Bus drivers (PCI, ACPI, OpenFirmware [for different architectures], USB)
-   Storage drivers
-   Network drivers
-   Simple VGA drivers for video cards
-   Input drivers


<a id="orgf0e6b2a"></a>

## Environment subsystems

WIRED supports multiple "faces" to the OS:

-   POSIX Subsystem
    -   ELF loader
    -   Execve, fork, signals, pipes
    -   BSD-style libc

-   JDK Subsystem
    -   Running java for system apps
    -   Class loader integrated with the Ob

-   WIRED Native API

The low-level C API with full access to:

-   Objects
-   Handles
-   Kernel Services
-   IPC
-   Virtual memory

Files containing these libraries will reside in the filesystem (posix.dll, java.dll, wiredos.dll)


<a id="org109add8"></a>

## Graphical system

I will include the WIRED window system, a custom usermode windowing server

-   Looks like Windows 3.1
-   Object based GUI elements
-   Bitmap fonts
-   16 colors (all you need)


<a id="org59930c1"></a>

## Networking

Minimal TCP/IP and NIC drivers will be included, as well as ARP, DHCP, and SSH (future)


<a id="orga47c6fe"></a>

# Boot process

AMD64 boot flow:

1.  UEFI->Limine
2.  Kernel loaded with UEFI memory map + limine modules
3.  Early console (framebuf)
4.  HAL init (future)
5.  Core kernel init (GDT/IDT/ACPI/APIC/HPET)
6.  PMM + VMM
7.  Object manager
8.  Scheduler
9.  System services
10. Start env subsystems and GUI

PowerPC / Sparc64 boot flow:

1.  OpenFirmware
2.  Retrieve device tree
3.  Parse MMU map
4.  HAL init
5.  Everything else

6.  Userland vision

Initially:

-   Shell
-   File utilities
-   Text editor
-   System diagnostic tools (dmesg)

Long term:

-   WIRED window system
-   Control panel
-   Regedit
-   Hardware monitor
-   Process monitoring
-   Posix compat suite
-   Dev tools


<a id="orgf3ca357"></a>

# Testing and tooling

WIRED has a custom build system, expanded to fit the entire project.

-   QEMU for testing builds
-   Bochs for CPU-level debugging
-   Serial console logging
-   Kernel tracing hooks


<a id="org09d948a"></a>

# Coding standards

WIRED enforces:

-   BSD/Allman style C formatting
-   static everyhting
-   Minimal globals
-   Header separation
-   Architecture specific code isolated under arch/
-   Kernel code in kernel/
-   Subsystem code in subsystem/ (or kernel/subsystem/)
-   Userspace in usermode/


<a id="orge67b458"></a>

# Long term dreams

These are not immediate goals, but the finishing touches

-   Real driver ecosystem
-   Portable layered kernel for multiple architectures and not just x86
-   Object-centric server framework
-   Porting DOOM and Quake
-   A lisp and BASIC interpreter
-   Audio subsystem
-   Filesystem snapshots
-   Multi-user workstation support

The OS should feel like a combination of NT 3.1, Solaris 7, FreeBSD, and Multics - designed for enthusiasts and kernel devs, and those who love meticulously architected systems.


<a id="orgc8b3d2a"></a>

# Developer roadmap


<a id="org8073d40"></a>

## PHASE 1 - BOOTSTRAP AND HARDWARE DISCOVERY


<a id="org03b00a4"></a>

### Bootloader integration

-   Use Limine (done)
-   Implement entry stub and kernel handoff (done)
-   Parse memory map (in progress)
-   Initialize early terminal (GOP + serial) (done)


<a id="org5e102de"></a>

### CPU and architecture

-   Set up GDT and IDT (done)
-   Implement ISR/IRQ stubs (done)
-   Initialize LAPIC/IOAPIC (in progress)


<a id="org82e086a"></a>

### ACPI bringup

-   Parse XSDT (done)
-   Parse FADT, MADT, HPET (in progress)
-   Enumerate CPUS
-   Enumerate APIC
-   Initialize HPET timer


<a id="org98e816b"></a>

## PHASE 2 - MEMORY MANAGEMENT


<a id="orge7ae80f"></a>

### Physical memory manager

-   Parse E820 memmap
-   Frame bitmap/buddy allocator
-   Frame allocation/free
-   Reserved region tracking


<a id="org62d696a"></a>

### VMM

-   Implement PAE paging
-   Set up kernel address space
-   Implement map<sub>page</sub>, unmap<sub>page</sub>, switch<sub>address</sub><sub>space</sub>
-   Kernel heap


<a id="org1a795f1"></a>

### Slab allocator

-   Slabs for small / medium kernel objects
-   Cache constructor / destructor callbacks
-   Debugging: dump slab stats


<a id="org8425998"></a>

## PHASE 3 - SCHEDULING AND MULTITASKING


<a id="org2f3de19"></a>

### Threading

-   Thread creation
-   Stack setup
-   Context switch routine
-   Per-CPU run queues


<a id="orgd5471c7"></a>

### Scheduler

-   Priority-based round robin
-   Timer-driven preemption
-   Idle threads per core
-   System clock ticks, uptime counter


<a id="org7934965"></a>

### Process model

-   Address spaces
-   Process groups
-   Process flags
-   Basic signals for POSIX runtime


<a id="orgfb9aafb"></a>

## PHASE 4 - OBJECT MANAGER


<a id="org6cb4d0d"></a>

### Object types

-   Object header
-   Reference counting
-   Type table
-   Vtable for object ops


<a id="orgc06e390"></a>

### Namespaces

-   Directory objects
-   Path parsing
-   Symbolic links
-   Handle tables per process


<a id="orge58357b"></a>

### Kernel objects

-   Threads
-   Processes
-   Mutexes
-   Timers
-   Events
-   Pipes
-   Files


<a id="org243d256"></a>

## PHASE 5 - IPC SYSTEM


<a id="org459e8b0"></a>

### Messaging primitives

-   Message queues
-   Ports
-   Channels
-   Shared memory regions


<a id="org57621c3"></a>

### System server infrastructure

-   Kernel launches userspace servers
-   Channels for server <&#x2013;> kernel communication
-   Event loops for servers


<a id="org8dfece0"></a>

## PHASE 6 - FILESYSTEM


<a id="org463cb14"></a>

### VFS architecture

-   VFS nodes
-   Inodes, objects
-   path resolution
-   File handle integration with Ob
-   Block device abstraction


<a id="org8773797"></a>

### FAT32 Driver

-   Read / write
-   Directory listing
-   Long filename support
-   VFS glue


<a id="orgdfed874"></a>

### WFS (Wired Filesystem) (FUTURE GOAL)

-   Metadata model
-   Extent storage
-   MFT-style object tbale
-   Checksumming
-   ACLs


<a id="org42eae6a"></a>

## PHASE 7 - DRIVERS AND DEVICE MODEL


<a id="orgf308fa4"></a>

### Driver framework

-   Driver object type
-   Userspace or kernel mode driver sandboxing
-   Bus enumeration (PCI, ACPI, OpenFirmware device tree)


<a id="org8afa34b"></a>

### Essential drivers

-   Keyboard
-   Mouse
-   AHCI or NVMe storage
-   Framebuffer driver
-   Network card


<a id="org7a0d1c0"></a>

## PHASE 8 - ENVIRONMENT SUBSYSTEMS


<a id="orgfacd036"></a>

### POSIX

-   Elf loader
-   Execve
-   Fork
-   Pipes
-   Signals
-   Minimal, BSD-Like libc
-   /proc like utilities


<a id="org809c391"></a>

### WIRED API

-   Syscalls for objects
-   Syscalls for IPC
-   Syscalls for memory
-   Syscalls for scheduling
-   Userspace wrappers
-   Documentation


<a id="orgd1fc2a8"></a>

### FUTURE GOALS (6 - 12 months)

-   Networking
-   Graphics and windowing
-   Security infrastructure
-   Filesystem snapshots
-   Filesystem journaling
-   SMP tuning
-   Audio subsystem
-   Package manager
-   Portable kernel
-   BASIC / lisp interpreter
-   Networking services
-   Plugin based driver ecosystem
-   Multi user support


<a id="org5fd34bb"></a>

# Conclusion

WIRED is ambitious because it needs to be. It is the ultimate passion project, made by people who love computers. It is not a commercial product that needs to be held to standards. Its sole purpose is to explore ideas that commercial systems abandoned

-   Object management
-   Portable architeture
-   Replacable subsystems
-   Human-readable implementation

The development goal is not speed - its elegant, comprehensive, creative design. This document will evolve as we refine the design, but everything written in this bible will appear in the final release.

Welcome to WIREDOS.

