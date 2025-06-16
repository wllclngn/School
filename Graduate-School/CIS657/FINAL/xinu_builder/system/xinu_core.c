/**
 * xinu_core.c - XINU core process with actual scheduler implementation
 * 
 * This program runs as an isolated process that communicates with the
 * Windows host process via named pipes. It uses the real XINU scheduler
 * code to demonstrate starvation prevention.
 */

// Standard includes
#include <stddef.h>  // Include at the very top to define size_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ipc_protocol.h"
#include "xinu_includes.h"
#include "process.h"
#include "pstarv.h"

// Function prototypes
bool InitializePipes(const char* inPipeName, const char* outPipeName);
bool ProcessHostCommand(void);
void CleanupAndExit(int exitCode);
pid32 XinuCreateProcess(const char* name, int priority); // Renamed from CreateProcess
void SimulateReadyQueue(void);
void DisplaySystemInfo(void);

// External function declarations from our test modules
extern process run_starvation_test_q1(void);
extern process run_starvation_test_q2(void);

// Add this function to display system info
void DisplaySystemInfo(void) {
    // Get current time in UTC
    time_t now = time(NULL);
    struct tm *tm_info = gmtime(&now);
    char time_str[30];
    strftime(time_str, 30, "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Get username
    char username[256] = "user"; // Default if not available
    
    // Display system information
    printf("Current Date and Time (UTC): %s\n", time_str);
    printf("Current User's Login: %s\n", username);
}

/**
 * HandleRunStarvationTestCommand - Handle starvation test command
 */
void HandleRunStarvationTestCommand(const HostCommand* cmd) {
    XinuResponse resp;
    
    const char* testType = GetCommandParam(cmd, "type");
    if (!testType) {
        resp.type = RESP_ERROR;
        resp.paramCount = 0;
        AddResponseParam(&resp, "error", "Missing test type parameter");
        SendResponseToHost(&resp);
        return;
    }
    
    // Send a response to acknowledge command
    resp.type = RESP_STARVATION_TEST;
    resp.paramCount = 0;
    AddResponseParam(&resp, "output", "\nStarting starvation test...\n");
    SendResponseToHost(&resp);
    
    // Run the appropriate test
    if (strcmp(testType, "Q1") == 0) {
        run_starvation_test_q1();
    } else if (strcmp(testType, "Q2") == 0) {
        run_starvation_test_q2();
    } else {
        // Unknown test type
        resp.type = RESP_ERROR;
        resp.paramCount = 0;
        AddResponseParam(&resp, "error", "Unknown test type");
        SendResponseToHost(&resp);
        return;
    }
    
    // Send completion response
    resp.type = RESP_OK;
    resp.paramCount = 0;
    AddResponseParam(&resp, "message", "Starvation test completed");
    SendResponseToHost(&resp);
}

/**
 * XinuCreateProcess - Create a new process in the XINU system
 * Renamed from CreateProcess to avoid Windows API conflict
 */
pid32 XinuCreateProcess(const char* name, int priority) {
    // Find free slot in process table
    pid32 pid = -1;
    for (int i = 0; i < NPROC; i++) {
        if (proctab[i].prstate == PR_FREE) {
            pid = i;
            break;
        }
    }
    
    if (pid == -1) {
        return BADPID;  // No free slots
    }
    
    // Initialize process entry
    proctab[pid].prstate = PR_SUSP;  // Start suspended
    proctab[pid].prprio = priority;
    strncpy(proctab[pid].prname, name, PNMLEN-1);
    proctab[pid].prname[PNMLEN-1] = '\0';  // Ensure null termination
    proctab[pid].prstklen = 1024;   // Simulated stack size
    proctab[pid].prstkptr = malloc(proctab[pid].prstklen);  // Allocate actual stack
    proctab[pid].prparent = currpid;
    proctab[pid].prtime = 0;
    proctab[pid].prcpuused = 0;
    proctab[pid].prstarvation = FALSE;
    
    return pid;
}

/**
 * ProcessHostCommand - Process a command from the host
 */
bool ProcessHostCommand(void) {
    HostCommand cmd;
    char buffer[MAX_MESSAGE_LENGTH];
    DWORD bytesRead;
    
    // Read command from pipe
    if (!ReadFile(pipeFromHost, buffer, sizeof(buffer), &bytesRead, NULL)) {
        printf("Error reading from pipe: %d\n", GetLastError());
        return FALSE;
    }
    
    buffer[bytesRead] = '\0';
    
    // Deserialize command
    if (!DeserializeCommand(buffer, &cmd)) {
        printf("Error deserializing command\n");
        return FALSE;
    }
    
    // Process command based on type
    switch (cmd.type) {
        case CMD_INITIALIZE:
            // Handle initialization
            break;
        case CMD_RUN_STARVATION_TEST:
            HandleRunStarvationTestCommand(&cmd);
            break;
        case CMD_GET_PROCESS_INFO:
            // Handle get process info
            break;
        case CMD_SHUTDOWN:
            // Handle shutdown
            return FALSE;
        default:
            // Unknown command
            SendErrorResponse("Unknown command");
            break;
    }
    
    return TRUE;
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
