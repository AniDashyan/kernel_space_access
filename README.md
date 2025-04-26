# Kernel Space Access

## Overview
This project is designed to demonstrate memory access violations by attempting to access memory beyond allocated heap and stack regions, as well as an invalid user-space address (0xDEADBEEF). The program illustrates why user-space programs cannot access kernel space by triggering and handling access violations, showing how the operating system enforces memory protection.

The project uses platform-specific exception handling mechanisms:

- **Linux/macOS**: Employs sigaction with SIGSEGV signal handling and jmp_buf/setjmp/longjmp from <csetjmp>.
- **Windows (MinGW-w64)**: Uses Vectored Exception Handling (VEH) to catch EXCEPTION_ACCESS_VIOLATION and EXCEPTION_STACK_OVERFLOW, integrated with jmp_buf/setjmp/longjmp.
- **Windows (MSVC)**: Utilizes Structured Exception Handling (SEH) with __try/__except.

## Build & Run

1. Clone the repository:
   ```
   git clone https://github.com/AniDashyan/kernel_space_access
   cd kernel-space-access
   ```

2. Build the project:
   ```
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build
   ```

3. Run the executable:
   ```
   build/kernel_access
   ```

## Example Output

### MinGW-w64 (Windows)
```
Starting memory access violation tests

=== Heap Out-of-Bounds Access ===
Base address: 0x000001c256de84b0
Attempting write to: 0x1c2571e84b0
Caught exception 0xc0000005 in VEH
Caught exception
Test completed

=== Stack Out-of-Bounds Access ===
Base address: 0x0000003396fff9f0
Attempting write to: 0x3397000990
Caught exception 0xc0000005 in VEH
Caught exception
Test completed

=== Invalid Address (0xDEADBEEF) ===
Base address: 0x00000000deadbeef
Attempting write to: 0xdeadbeef
Caught exception 0xc0000005 in VEH
Caught exception
Test completed

All test cases completed
```

## Explanation

### Output Explanation

- **Heap Test**: Attempts to write 1 MB beyond a 10-integer heap buffer, triggering an access violation (EXCEPTION_ACCESS_VIOLATION, 0xc0000005 in MinGW-w64), caught by VEH. The large offset ensures access to an unmapped or protected memory region.
- **Stack Test**: Attempts to write 4 KB beyond a 10-integer stack array, triggering an access violation, caught by VEH. A smaller offset (1000 indices) avoids stack overflow issues, ensuring the violation is handled as a standard access violation.
- **Invalid Address Test**: Attempts to write to 0xDEADBEEF, an unmapped user-space address, triggering an access violation, caught by VEH.

**Platform Variations**:
- **Linux/macOS**: Catches SIGSEGV with sigaction, showing the faulting address via siginfo_t.
- **MSVC**: Uses SEH, providing detailed output (e.g., access type, exception flags).
- **MinGW-w64**: VEH logs exception codes (0xc0000005 for access violation, 0xc00000fd for stack overflow if applicable).

## Why User-Space Programs Cannot Access Kernel Space

User-space programs, like this one, cannot access kernel space due to the operating system's memory protection mechanisms, enforced through hardware and software:

1. **Virtual Memory Separation**: Modern operating systems (e.g., Linux, macOS, Windows) use virtual memory to isolate user-space and kernel-space memory. Each process operates in its own virtual address space, where user-space addresses (e.g., heap, stack, 0xDEADBEEF) are mapped to physical memory allocated for the process. Kernel-space addresses (e.g., kernel code, data structures) are mapped to separate, privileged memory regions inaccessible to user-space processes.

2. **Privilege Levels**: CPUs enforce privilege levels (e.g., Ring 3 for user-space, Ring 0 for kernel-space on x86). User-space programs run in a restricted mode, preventing direct access to kernel memory or privileged instructions. Attempts to access kernel-space addresses trigger a hardware exception (e.g., SIGSEGV on Linux/macOS, EXCEPTION_ACCESS_VIOLATION on Windows), as seen in the program's output when accessing unmapped or protected regions.

3. **Page Table Protections**: The operating system configures page tables to mark kernel-space memory pages as privileged or unmapped in user-space contexts. Accessing such addresses results in a page fault, which the OS converts to an exception or signal, terminating the access attempt.

4. **System Call Interface**: User-space programs interact with the kernel only through controlled system calls (e.g., read, write), which execute in kernel mode under strict supervision. Direct memory access to kernel space is blocked to prevent security vulnerabilities and system instability.

In this program, attempts to access memory beyond allocated regions (heap, stack) or an invalid address (0xDEADBEEF) simulate scenarios where a user-space program might inadvertently try to access protected memory, including kernel space. The resulting exceptions demonstrate the OS's enforcement of memory isolation, ensuring user-space programs cannot breach kernel-space boundaries.
