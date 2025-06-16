/**
 * xinu_core.c - XINU core process with actual scheduler implementation
 * 
 * This program runs as an isolated process that communicates with the
 * Windows host process via named pipes. It uses the real XINU scheduler
 * code to demonstrate starvation prevention.
 */

// Fix conflicts between Windows API and XINU definitions
#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "ipc_protocol.h"

// Rest of includes remain the same...

// Function prototypes
bool InitializePipes(const char* inPipeName, const char* outPipeName);
bool ProcessHostCommand(void);
void CleanupAndExit(int exitCode);
pid32 XinuCreateProcess(const char* name, int priority); // Renamed from CreateProcess
void SimulateReadyQueue(void);
void DisplaySystemInfo(void);

// Add this function to display system info
void DisplaySystemInfo(void) {
    // Get current time in UTC
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    char time_str[30];
    strftime(time_str, 30, "%Y-%m-%d %H:%M:%SS", tm_info);
    
    // Get Windows username
    char username[256];
    DWORD username_len = 256;
    GetUserNameA(username, &username_len);
    
    // Display system information
    printf("Current Date and Time (UTC): %s\n", time_str);
    printf("Current User's Login: %s\n", username);
}

// Rest of the code remains the same...

/**
 * XinuCreateProcess - Create a new process in the XINU system
 * Renamed from CreateProcess to avoid Windows API conflict
 */
pid32 XinuCreateProcess(const char* name, int priority) {
    // Implementation remains the same as CreateProcess
    // Find free slot in process table
    pid32 pid = -1;
    for (int i = 0; i < NPROC; i++) {
        if (proctab[i].prstate == PRFREE) {
            pid = i;
            break;
        }
    }
    
    if (pid == -1) {
        return BADPID;  // No free slots
    }
    
    // Initialize process entry
    proctab[pid].prstate = PRSUSP;  // Start suspended
    proctab[pid].prprio = priority;
    strncpy_s(proctab[pid].prname, sizeof(proctab[pid].prname), name, _TRUNCATE);
    proctab[pid].prstklen = 1024;   // Simulated stack size
    proctab[pid].prstkptr = 0;      // No real stack in simulation
    proctab[pid].prparent = currpid;
    proctab[pid].prtime = 0;
    proctab[pid].prcpuused = 0;
    proctab[pid].prstarvation = FALSE;
    
    return pid;
}

// Update all function calls that used CreateProcess to use XinuCreateProcess instead

/**
 * HandleCreateProcessCommand - Handle the process creation command
 */
void HandleCreateProcessCommand(const HostCommand* cmd) {
    // Same implementation but use XinuCreateProcess instead
    XinuResponse resp;
    
    const char* name = GetCommandParam(cmd, "name");
    const char* priorityStr = GetCommandParam(cmd, "priority");
    
    if (!name || !priorityStr) {
        resp.type = RESP_ERROR;
        resp.paramCount = 0;
        AddResponseParam(&resp, "error", "Missing name or priority parameter");
        SendResponseToHost(&resp);
        return;
    }
    
    int priority = atoi(priorityStr);
    pid32 pid = XinuCreateProcess(name, priority); // Changed from CreateProcess
    
    // Rest of the function remains the same...
}

/**
 * main - Entry point for XINU core process
 */
int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <inPipeName> <outPipeName>\n", argv[0]);
        return 1;
    }
    
    // Display system information
    DisplaySystemInfo();
    
    // Get pipe names from command-line arguments
    const char *inPipeName = argv[1];
    const char *outPipeName = argv[2];
    
    // Connect to pipes
    if (!InitializePipes(inPipeName, outPipeName)) {
        return 1;
    }
    
    // Process commands from host until shutdown
    while (ProcessHostCommand()) {
        // Continue processing commands until shutdown flag is set
    }
    
    // Clean up and exit
    CleanupAndExit(0);
    return 0;
}