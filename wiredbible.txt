WIREDOS: SYSTEM DESIGN BIBLE

A technical vision for a modern hobby operating system project.

WIREDOS is not just another hobby kernel. It is a deliberatly engineered, carefully architectured operating system designed around 4 pillars:

1. Object centric system design inspired by Windows NT
2. Strict subsystem isolation
3. Predictable, inspectable, tiered performance
4. Portability to other architectures made easy

This document outlines the philosophy and long term goals of the project.

1. Architectural philosophy

1.1 Hybrid kernel architecture

WIRED uses a micro/hybrid kernel, with the following principles:

* Core kernel is minimal: scheduler, memory management, IPC, object manager, networking, filesystems (VFS), and core hardware abstraction

* Everything else, like subsystems, runtimes, high-level drivers, and GUI applications, are isolated in their own subsystems.

* Userspace is made to act like a collection of trusted servers, with programs running as userspace clients

* Every subsystem (filesystem drivers, GUI programs, java programs) communicates through defined C interfaces, shared objects, object handles, and kernel IPC channels.


1.2 Multics-like layering

The OS is structured in predictable layers or rings, similar to Multics:

1. Hardware abstraction layer
2. Kernel core (memory, ACPI functions, core system drivers)
3. Kernel objects + handles
4. Userspace servers
5. Environment subsystems (WIRED API, POSIX, JDK)
6. Userspace and GUI

Every component will be separated by 
* layer
* responsibilites
* dependencies

1.3 Portability

WIRED is conceptually hardware-agnostic. Currently, the only architecture supported is AMD64, but future targets inclue

* PowerPC
* Sparc64
* DEC Alpha
* RISC-V

Portability is included as a fun challenge to me to see how many targets I can hit with as little code possible. A HAL will be written to abstract the upper-level code. The HAL will expose:

* Interrupt controller abstraction (LAPIC, OpenPIC, XICS)
* Timer abstraction
* Page table helpers (x86 PAE, PPC hash tables, RISC-V paging)
* Bootloader handoff
* Device enumeration
* Basic system information

1.4 Memory management

WIRED uses a four-tiered system memory managment architecture:

Kernel heap / slab alloc
|
Virtual memory management (kernel and usermode address space, .bss, .text)
|
4 Level PAE Paging
|
Physical memory management (E820 map, malloc() free() functions, frame allocator)

Memory is tracked as kernel objects, not raw pointers.

1.4 Scheduling and multitasking

WIRED uses a preemptive, priority based round robin scheduler, modeled after 

* UNIX system V
* FreeBSD and Solaris' early schedulers
* Windows NT kernel dispatcher

Core features include

* Time quantum slices
* Kernel threads and user threads
* Thread pools
* Process groups
* Per-CPU run queues
* Load balancing between cores
* System clock ticks
* Mutex / spinlocking

1.5 Object manager (Ob), the heart of WIREDOS

Heavily inspired by the Windows NT Object manager.

1.5.1 Every resource is an object

Examples:

* Files
* Mutexes
* Threads
* Processes
* Timers
* Sockets
* Shared memory segments
* Symbolic links
* Registry keys and nodes

All objects share:

* A type
* A vtable
* A reference count
* A namespace path

The namespaces provided are

\System\
\Devices\
\Processes\
\Handles\
\Registry\
\POSIX\
\WIREDAPI\

1.6 Filesystems

Immediate targets for filesystems include

* FAT32 (for EFI)
* Simple VFS layer
* UFS (short term)

Eventually, I will implement the WIRED filesystem (WFS), which will include

* Master file table
* Journaling 
* Disk Access Control Lists
* Checksumming
* Every file is an object

1.7 IPC and inter-kernel messaging

WIRED will include a flexible ICP model

* Message queues
* Ports
* Channels
* Shared memory with synchronization

1.8 Drivers and Device model

Drivers must:

* Live in userspace (when possible)
* Use a stable ABI exposed by the Ob
* Be hot-reloadable
* Use event driven models (interrupt -> message -> driver thread)

Driver categories include:

* Bus drivers (PCI, ACPI, OpenFirmware [for different architectures], USB)
* Storage drivers
* Network drivers
* Simple VGA drivers for video cards
* Input drivers

1.9 Environment subsystems

WIRED supports multiple "faces" to the OS:

- POSIX Subsystem
  * ELF loader
  * Execve, fork, signals, pipes
  * BSD-style libc

- JDK Subsystem
  * Running java for system apps
  * Class loader integrated with the Ob

- WIRED Native API
The low-level C API with full access to:

  * Objects
  * Handles
  * Kernel Services
  * IPC
  * Virtual memory

Files containing these libraries will reside in the filesystem (posix.dll, java.dll, wiredos.dll)

1.10 Graphical system

I will include the WIRED window system, a custom usermode windowing server

* Looks like Windows 3.1
* Object based GUI elements
* Bitmap fonts
* 16 colors (all you need)
  
1.11 Networking

Minimal TCP/IP and NIC drivers will be included, as well as ARP, DHCP, and SSH (future)

2. Boot process

AMD64 boot flow:

1. UEFI->Limine
2. Kernel loaded with UEFI memory map + limine modules
3. Early console (framebuf)
4. HAL init (future)
5. Core kernel init (GDT/IDT/ACPI/APIC/HPET)
6. PMM + VMM
7. Object manager
8. Scheduler
9. System services
10. Start env subsystems and GUI

PowerPC / Sparc64 boot flow:

1. OpenFirmware
2. Retrieve device tree
3. Parse MMU map
4. HAL init
5. Everything else

3. Userland vision

Initially:

* Shell
* File utilities
* Text editor
* System diagnostic tools (dmesg)

Long term:

* WIRED window system
* Control panel
* Regedit
* Hardware monitor
* Process monitoring
* Posix compat suite
* Dev tools

4. Testing and tooling

WIRED has a custom build system, expanded to fit the entire project.

* QEMU for testing builds
* Bochs for CPU-level debugging
* Serial console logging
* Kernel tracing hooks

5. Coding standards

WIRED enforces:

* BSD/Allman style C formatting
* static everyhting
* Minimal globals
* Header separation
* Architecture specific code isolated under arch/
* Kernel code in kernel/
* Subsystem code in subsystem/ (or kernel/subsystem/)
* Userspace in usermode/

6. Long term dreams

These are not immediate goals, but the finishing touches

* Real driver ecosystem
* Portable layered kernel for multiple architectures and not just x86
* Object-centric server framework
* Porting DOOM and Quake
* A lisp and BASIC interpreter
* Audio subsystem
* Filesystem snapshots
* Multi-user workstation support

The OS should feel like a combination of NT 3.1, Solaris 7, FreeBSD, and Multics - designed for enthusiasts and kernel devs, and those who love meticulously architected systems.

7. Developer roadmap

PHASE 1 - BOOTSTRAP AND HARDWARE DISCOVERY

1.1 Bootloader integration

* Use Limine (done)
* Implement entry stub and kernel handoff (done)
* Parse memory map (in progress)
* Initialize early terminal (GOP + serial) (done)

1.2 CPU and architecture

* Set up GDT and IDT (done)
* Implement ISR/IRQ stubs (done)
* Initialize LAPIC/IOAPIC (in progress)

1.3 ACPI bringup

* Parse XSDT (done)
* Parse FADT, MADT, HPET (in progress)
* Enumerate CPUS
* Enumerate APIC
* Initialize HPET timer

PHASE 2 - MEMORY MANAGEMENT

2.1 Physical memory manager

* Parse E820 memmap
* Frame bitmap/buddy allocator
* Frame allocation/free
* Reserved region tracking

2.2 VMM

* Implement PAE paging
* Set up kernel address space
* Implement map_page, unmap_page, switch_address_space
* Kernel heap

2.3 Slab allocator

* Slabs for small / medium kernel objects
* Cache constructor / destructor callbacks
* Debugging: dump slab stats

PHASE 3 - SCHEDULING AND MULTITASKING

3.1 Threading

* Thread creation
* Stack setup
* Context switch routine
* Per-CPU run queues

3.2 Scheduler

* Priority-based round robin
* Timer-driven preemption
* Idle threads per core
* System clock ticks, uptime counter

3.3 Process model

* Address spaces
* Process groups
* Process flags
* Basic signals for POSIX runtime

PHASE 4 - OBJECT MANAGER

4.1 Object types

* Object header
* Reference counting
* Type table
* Vtable for object ops

4.2 Namespaces

* Directory objects
* Path parsing
* Symbolic links
* Handle tables per process

4.3 Kernel objects

* Threads
* Processes
* Mutexes
* Timers
* Events
* Pipes
* Files

PHASE 5 - IPC SYSTEM

5.1 Messaging primitives

* Message queues
* Ports
* Channels
* Shared memory regions

5.2 System server infrastructure

* Kernel launches userspace servers
* Channels for server <--> kernel communication
* Event loops for servers

PHASE 6 - FILESYSTEM

6.1 VFS architecture

* VFS nodes
* Inodes, objects
* path resolution
* File handle integration with Ob
* Block device abstraction

6.2 FAT32 Driver

* Read / write
* Directory listing
* Long filename support
* VFS glue

6.3 WFS (Wired Filesystem) (FUTURE GOAL)

* Metadata model
* Extent storage
* MFT-style object tbale
* Checksumming
* ACLs

PHASE 7 - DRIVERS AND DEVICE MODEL

7.1 Driver framework

* Driver object type
* Userspace or kernel mode driver sandboxing
* Bus enumeration (PCI, ACPI, OpenFirmware device tree)

7.2 Essential drivers

* Keyboard
* Mouse
* AHCI or NVMe storage
* Framebuffer driver
* Network card

PHASE 8 - ENVIRONMENT SUBSYSTEMS

8.1 POSIX 

* Elf loader
* Execve
* Fork
* Pipes
* Signals
* Minimal, BSD-Like libc
* /proc like utilities

8.2 WIRED API

* Syscalls for objects
* Syscalls for IPC
* Syscalls for memory
* Syscalls for scheduling
* Userspace wrappers
* Documentation

FUTURE GOALS (6 - 12 months)

* Networking
* Graphics and windowing
* Security infrastructure
* Filesystem snapshots
* Filesystem journaling
* SMP tuning
* Audio subsystem
* Package manager
* Portable kernel
* BASIC / lisp interpreter
* Networking services
* Plugin based driver ecosystem
* Multi user support 

8. Conclusion

WIRED is ambitious because it needs to be. It is the ultimate passion project, made by people who love computers. It is not a commercial product that needs to be held to standards. Its sole purpose is to explore ideas that commercial systems abandoned

* Object management
* Portable architeture
* Replacable subsystems
* Human-readable implementation

The development goal is not speed - its elegant, comprehensive, creative design. This document will evolve as we refine the design, but everything written in this bible will appear in the final release.

Welcome to WIREDOS.