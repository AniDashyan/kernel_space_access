#include <iostream>
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <iomanip>
#include <csignal>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <signal.h>
#endif
#include <csetjmp>

// Use jmp_buf for Windows, sigjmp_buf for Linux
#ifdef _WIN32
static jmp_buf jmp_env;
#else
static sigjmp_buf jmp_env;
#endif
static volatile sig_atomic_t jmp_set = 0;
static volatile sig_atomic_t caught_signal = 0;
static volatile uintptr_t fault_address = 0;

#ifdef _WIN32
LONG WINAPI vectored_handler(EXCEPTION_POINTERS* ExceptionInfo) {
    DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    if ((code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_STACK_OVERFLOW) && jmp_set) {
        fault_address = ExceptionInfo->ExceptionRecord->ExceptionInformation[1];
        caught_signal = code;
        longjmp(jmp_env, 1);
    }
    std::cerr << "Unhandled exception 0x" << std::hex << code
              << " at address: 0x" << std::setw(16) << std::setfill('0')
              << ExceptionInfo->ExceptionRecord->ExceptionInformation[1] << "\n";
    _exit(EXIT_FAILURE);
}
#else
static void signal_handler(int sig, siginfo_t* info, void*) {
    if (jmp_set) {
        fault_address = reinterpret_cast<uintptr_t>(info->si_addr);
        caught_signal = sig;
        siglongjmp(jmp_env, 1);
    }
    std::cerr << "Unhandled signal " << sig << "\n";
    _exit(EXIT_FAILURE);
}
#endif

void setup_handler() {
#ifdef _WIN32
    #ifndef USE_SEH // MinGW-w64 uses VEH
    AddVectoredExceptionHandler(1, vectored_handler);
    #endif
#else // Linux only
    struct sigaction sa = {};
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO; 
    sigemptyset(&sa.sa_mask);
    
    // Add SIGSEGV to sa_mask
    sigaddset(&sa.sa_mask, SIGSEGV);
    
    // Install handler for SIGSEGV only
    if (sigaction(SIGSEGV, &sa, nullptr) == -1) {
        std::cerr << "Failed to set SIGSEGV handler: " << std::strerror(errno) << "\n";
        std::exit(EXIT_FAILURE);
    }
#endif
}

#ifdef USE_SEH
int filter_exception(EXCEPTION_POINTERS* ep) {
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void attempt_access(const char* test_name, int* base_ptr, uintptr_t offset, bool is_heap = false) {
    std::cout << "\n=== " << test_name << " ===\n";
    uintptr_t base_addr = reinterpret_cast<uintptr_t>(base_ptr);
    int* target_ptr = base_ptr + offset;
    uintptr_t target_addr = reinterpret_cast<uintptr_t>(target_ptr);
    std::cout << "Base address: 0x" << std::hex << std::setw(16) << std::setfill('0') << base_addr
              << "\nAttempting write to: 0x" << target_addr << "\n";

#ifdef USE_SEH // SEH for MSVC
    EXCEPTION_POINTERS* exception_info = nullptr;
    __try {
        *target_ptr = 42;
        std::cout << "Unexpected success: Wrote 42\n";
    } __except (exception_info = GetExceptionInformation(), filter_exception(exception_info)) {
        EXCEPTION_RECORD* record = exception_info->ExceptionRecord;
        std::cerr << "Access Violation at: 0x" << std::hex << std::setw(16) << std::setfill('0')
                  << record->ExceptionAddress << "\n";
        std::cerr << "Attempted access: 0x" << record->ExceptionInformation[1]
                  << "\nType: " << (record->ExceptionInformation[0] == 0 ? "Read" :
                                    record->ExceptionInformation[0] == 1 ? "Write" : "Execute")
                  << "\nFlags: 0x" << record->ExceptionFlags << "\n";
    }
#else
    caught_signal = 0;
    fault_address = 0;
    
    // Set jmp_set before calling setjmp/sigsetjmp to avoid race conditions
    #ifdef _WIN32
    jmp_set = 1;
    if (setjmp(jmp_env) == 0) {
    #else
    jmp_set = 1;
    if (sigsetjmp(jmp_env, 1) == 0) {
    #endif
        *target_ptr = 42;
        jmp_set = 0;
        std::cout << "Unexpected success: Wrote 42\n";
    } else {
        jmp_set = 0;
        #ifdef _WIN32
        std::cerr << "Caught exception 0x" << std::hex << caught_signal
                  << " at address: 0x" << std::setw(16) << std::setfill('0') << fault_address << "\n";
        #else
        std::cerr << "Caught SIGSEGV (signal: " << caught_signal << ") at address: 0x"
                  << std::hex << std::setw(16) << std::setfill('0') << fault_address << "\n";
        #endif
        std::cout << "Caught exception\n";
    }
#endif

    if (is_heap) delete[] base_ptr;
    std::cout << "Test completed\n";
}

int main() {
    std::cout << "Starting memory access violation tests\n";

#ifndef USE_SEH
    setup_handler();
#endif

    int* heap_buffer = new int[10];
    attempt_access("Heap Out-of-Bounds Access", heap_buffer, 0x100000, true);

    int stack_array[10];
    attempt_access("Stack Out-of-Bounds Access", stack_array, 1000);

    int* invalid_ptr = reinterpret_cast<int*>(static_cast<uintptr_t>(0xDEADBEEF));
    attempt_access("Invalid Address (0xDEADBEEF)", invalid_ptr, 0);

    std::cout << "\nAll test cases completed\n";
    return 0;
}