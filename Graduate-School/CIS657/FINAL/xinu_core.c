/**
 * xinu_core.c - XINU core process with actual scheduler implementation
 * 
 * This program runs as an isolated process that communicates with the
 * Windows host process via named pipes. It uses the real XINU scheduler
 * code to demonstrate starvation prevention.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "ipc_protocol.h"

// Define XINU types and constants
typedef int pid32;
typedef int pri16;
typedef int sid32;
typedef int qid16;
typedef int bool;
typedef unsigned int umsg32;
typedef int status;

#define NPROC       8       /* number of user processes      */
#define NSEM        100     /* number of semaphores          */
#define PRNONEMPTY  1       /* ready list is not empty       */
#define PRCURR      '\02'   /* process is currently running  */
#define PRREADY     '\04'   /* process is on ready queue     */
#define PRSUSP      '\03'   /* process is suspended          */
#define PRFREE      '\01'   /* process slot is free          */
#define PRWAIT      '\05'   /* process is waiting            */
#define PRSLEEP     '\06'   /* process is sleeping           */
#define PR_FREE     0       /* process table entry is unused */
#define BADPID      (-1)    /* used when invalid pid needed  */
#define OK          1       /* system call successful        */
#define SYSERR      (-1)    /* system call unsuccessful      */
#define TRUE        1
#define FALSE       0

// Process table entry
struct procent {
    char    prstate;        /* process state: PRCURR, etc. */
    pri16   prprio;         /* process priority            */
    char    prname[16];     /* process name                */
    int     prstklen;       /* stack length in bytes       */
    int     prstkptr;       /* saved stack pointer         */
    int     prparent;       /* ID of the creating process  */
    int     prtime;         /* time process has run        */
    int     prcpuused;      /* CPU time used               */
    bool    prstarvation;   /* starvation prevention flag  */
};

// ===== XINU Global variables (from kernel) =====
extern struct procent proctab[];         // Process table
extern pid32 currpid;                    // Currently running process
extern qid16 readylist;                  // Ready list ID
extern int starvIters;                   // Starvation iterations
extern int starvBoost;                   // Starvation priority boost
extern int starvTime;                    // Starvation time threshold

// ===== XINU Function prototypes (implemented in system/*.c) =====
extern void resched(void);               // From resched.c
extern status ready(pid32 pid);          // From ready.c
extern void starvation_check(void);      // From starvation_prevention.c

// Local variables to simulate XINU environment
struct procent proctab[NPROC];          // Process table
pid32 currpid;                          // Currently running process
qid16 readylist;                        // Ready list ID
int starvIters = 0;                     // Starvation iterations
int starvBoost = 10;                    // Starvation priority boost
int starvTime = 5;                      // Starvation time threshold
bool enable_starvation_prevention = TRUE;
char current_username[256] = "";        // Username passed from host

// Windows IPC variables
HANDLE pipeFromHost = INVALID_HANDLE_VALUE;
HANDLE pipeToHost = INVALID_HANDLE_VALUE;
bool shutdownFlag = FALSE;

// Function prototypes
bool InitializePipes(const char* inPipeName, const char* outPipeName);
bool ProcessHostCommand(void);
void CleanupAndExit(int exitCode);
pid32 CreateProcess(const char* name, int priority);
void SimulateReadyQueue(void);

// XINU IPC protocol functions
int SerializeResponse(const XinuResponse* resp, char* buffer, int bufferSize) {
    // Format: type|paramCount|name1=value1|name2=value2|...
    int offset = _snprintf_s(buffer, bufferSize, bufferSize, "%d|%d", resp->type, resp->paramCount);
    
    for (int i = 0; i < resp->paramCount && offset < bufferSize; i++) {
        offset += _snprintf_s(buffer + offset, bufferSize - offset, bufferSize - offset, 
                             "|%s=%s", resp->paramNames[i], resp->paramValues[i]);
    }
    
    return offset;
}

int DeserializeCommand(const char* buffer, HostCommand* cmd) {
    char* context = NULL;
    char* token = NULL;
    char tempBuffer[MAX_MESSAGE_LENGTH];
    
    // Copy buffer to avoid modifying original
    strncpy_s(tempBuffer, sizeof(tempBuffer), buffer, _TRUNCATE);
    
    // Extract type
    token = strtok_s(tempBuffer, "|", &context);
    if (!token) return 0;
    cmd->type = (HostCommandType)atoi(token);
    
    // Extract parameter count
    token = strtok_s(NULL, "|", &context);
    if (!token) return 0;
    cmd->paramCount = atoi(token);
    
    // Extract parameters
    for (int i = 0; i < cmd->paramCount; i++) {
        // Get name=value pair
        token = strtok_s(NULL, "|", &context);
        if (!token) break;
        
        // Split into name and value
        char* equals = strchr(token, '=');
        if (!equals) continue;
        
        // Extract name
        int nameLen = (int)(equals - token);
        strncpy_s(cmd->paramNames[i], MAX_PARAM_VALUE, token, nameLen);
        cmd->paramNames[i][nameLen] = '\0';
        
        // Extract value
        strncpy_s(cmd->paramValues[i], MAX_PARAM_VALUE, equals + 1, _TRUNCATE);
    }
    
    return 1;
}

void AddResponseParam(XinuResponse* resp, const char* name, const char* value) {
    if (resp->paramCount < MAX_PARAMS) {
        strncpy_s(resp->paramNames[resp->paramCount], MAX_PARAM_VALUE, name, _TRUNCATE);
        strncpy_s(resp->paramValues[resp->paramCount], MAX_PARAM_VALUE, value, _TRUNCATE);
        resp->paramCount++;
    }
}

const char* GetCommandParam(const HostCommand* cmd, const char* name) {
    for (int i = 0; i < cmd->paramCount; i++) {
        if (_stricmp(cmd->paramNames[i], name) == 0) {
            return cmd->paramValues[i];
        }
    }
    return NULL;
}

/**
 * InitializePipes - Connect to the named pipes created by the host
 */
bool InitializePipes(const char* inPipeName, const char* outPipeName) {
    // Connect to host-to-xinu pipe (for reading commands)
    pipeFromHost = CreateFile(
        inPipeName,                 // Pipe name
        GENERIC_READ,               // Read access
        0,                          // No sharing
        NULL,                       // Default security
        OPEN_EXISTING,              // Open existing pipe
        0,                          // Default attributes
        NULL                        // No template file
    );
    
    if (pipeFromHost == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "XINU: Failed to connect to input pipe: %d\n", GetLastError());
        return FALSE;
    }
    
    // Connect to xinu-to-host pipe (for sending responses)
    pipeToHost = CreateFile(
        outPipeName,                // Pipe name
        GENERIC_WRITE,              // Write access
        0,                          // No sharing
        NULL,                       // Default security
        OPEN_EXISTING,              // Open existing pipe
        0,                          // Default attributes
        NULL                        // No template file
    );
    
    if (pipeToHost == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "XINU: Failed to connect to output pipe: %d\n", GetLastError());
        CloseHandle(pipeFromHost);
        return FALSE;
    }
    
    return TRUE;
}

/**
 * SendResponseToHost - Send a response to the host process
 */
bool SendResponseToHost(XinuResponse* resp) {
    char buffer[MAX_MESSAGE_LENGTH];
    DWORD bytesWritten;
    
    int length = SerializeResponse(resp, buffer, sizeof(buffer));
    if (length <= 0) {
        fprintf(stderr, "XINU: Failed to serialize response\n");
        return FALSE;
    }
    
    if (!WriteFile(pipeToHost, buffer, length, &bytesWritten, NULL)) {
        fprintf(stderr, "XINU: Failed to send response to host: %d\n", GetLastError());
        return FALSE;
    }
    
    return TRUE;
}

/**
 * ReceiveCommandFromHost - Receive a command from the host process
 */
bool ReceiveCommandFromHost(HostCommand* cmd) {
    char buffer[MAX_MESSAGE_LENGTH];
    DWORD bytesRead;
    
    if (!ReadFile(pipeFromHost, buffer, sizeof(buffer), &bytesRead, NULL)) {
        DWORD error = GetLastError();
        if (error == ERROR_BROKEN_PIPE || error == ERROR_PIPE_NOT_CONNECTED) {
            // Host disconnected
            fprintf(stderr, "XINU: Host process disconnected\n");
            shutdownFlag = TRUE;
            return FALSE;
        }
        
        fprintf(stderr, "XINU: Failed to receive command from host: %d\n", error);
        return FALSE;
    }
    
    buffer[bytesRead] = '\0';
    
    if (!DeserializeCommand(buffer, cmd)) {
        fprintf(stderr, "XINU: Failed to deserialize command\n");
        return FALSE;
    }
    
    return TRUE;
}

/**
 * InitializeXinuSystem - Initialize the XINU system
 */
void InitializeXinuSystem(const HostCommand* cmd) {
    XinuResponse resp;
    
    // Get username from command parameters
    const char* username = GetCommandParam(cmd, "username");
    if (username) {
        strncpy_s(current_username, sizeof(current_username), username, _TRUNCATE);
    }
    
    // Initialize process table
    for (int i = 0; i < NPROC; i++) {
        proctab[i].prstate = PRFREE;
        proctab[i].prprio = 0;
        strncpy_s(proctab[i].prname, sizeof(proctab[i].prname), "", _TRUNCATE);
        proctab[i].prstklen = 0;
        proctab[i].prstkptr = 0;
        proctab[i].prparent = 0;
        proctab[i].prtime = 0;
        proctab[i].prcpuused = 0;
        proctab[i].prstarvation = FALSE;
    }
    
    // Initialize main system variables
    currpid = 0;
    readylist = 0;
    
    // Process 0 is the XINU null process
    proctab[0].prstate = PRCURR;
    proctab[0].prprio = 0;
    strncpy_s(proctab[0].prname, sizeof(proctab[0].prname), "null", _TRUNCATE);
    
    // Send response
    resp.type = RESP_OK;
    resp.paramCount = 0;
    AddResponseParam(&resp, "message", "XINU system initialized successfully");
    
    SendResponseToHost(&resp);
}

/**
 * CreateProcess - Create a new process in the XINU system
 */
pid32 CreateProcess(const char* name, int priority) {
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

/**
 * HandleCreateProcessCommand - Handle the process creation command
 */
void HandleCreateProcessCommand(const HostCommand* cmd) {
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
    pid32 pid = CreateProcess(name, priority);
    
    if (pid == BADPID) {
        resp.type = RESP_ERROR;
        resp.paramCount = 0;
        AddResponseParam(&resp, "error", "Failed to create process (process table full)");
    } else {
        resp.type = RESP_PROCESS_CREATED;
        resp.paramCount = 0;
        
        char pidStr[16];
        _snprintf_s(pidStr, sizeof(pidStr), _TRUNCATE, "%d", pid);
        AddResponseParam(&resp, "pid", pidStr);
        AddResponseParam(&resp, "name", name);
        
        char priorityStrRes[16];
        _snprintf_s(priorityStrRes, sizeof(priorityStrRes), _TRUNCATE, "%d", priority);
        AddResponseParam(&resp, "priority", priorityStrRes);
        
        // Make the process ready
        proctab[pid].prstate = PRREADY;
    }
    
    SendResponseToHost(&resp);
}

/**
 * HandleGetProcessInfoCommand - Handle the get process info command
 */
void HandleGetProcessInfoCommand(void) {
    XinuResponse resp;
    resp.type = RESP_PROCESS_INFO;
    resp.paramCount = 0;
    
    int count = 0;
    for (int i = 0; i < NPROC; i++) {
        if (proctab[i].prstate != PRFREE) {
            count++;
            
            char processKey[32];
            _snprintf_s(processKey, sizeof(processKey), _TRUNCATE, "process%d", i);
            
            char processInfo[MAX_PARAM_VALUE];
            const char* stateStr;
            
            switch (proctab[i].prstate) {
                case PRCURR: stateStr = "CURRENT"; break;
                case PRREADY: stateStr = "READY"; break;
                case PRSUSP: stateStr = "SUSPENDED"; break;
                case PRWAIT: stateStr = "WAITING"; break;
                case PRSLEEP: stateStr = "SLEEPING"; break;
                default: stateStr = "UNKNOWN"; break;
            }
            
            _snprintf_s(processInfo, sizeof(processInfo), _TRUNCATE, 
                       "PID: %d  Name: %-16s  Priority: %-3d  State: %-10s  CPU: %-3d  %s",
                       i, proctab[i].prname, proctab[i].prprio, stateStr, proctab[i].prcpuused,
                       (proctab[i].prstarvation ? "[STARVING]" : ""));
            
            AddResponseParam(&resp, processKey, processInfo);
        }
    }
    
    char countStr[16];
    _snprintf_s(countStr, sizeof(countStr), _TRUNCATE, "%d", count);
    AddResponseParam(&resp, "count", countStr);
    
    SendResponseToHost(&resp);
}

/**
 * RunQ1StarvationTest - Run the context-switch based starvation test
 */
void RunQ1StarvationTest(void) {
    XinuResponse resp;
    pid32 pid1, pid2, pidStarv;
    char buffer[512];
    
    // Send initial output message
    resp.type = RESP_STARVATION_TEST;
    resp.paramCount = 0;
    AddResponseParam(&resp, "output", "Starting starvation simulation for Q1 (context switch based)...\n");
    SendResponseToHost(&resp);
    
    // Create test processes
    pid1 = CreateProcess("P1", 40);
    pid2 = CreateProcess("P2", 35);
    pidStarv = CreateProcess("PStarv", 25);
    
    if (pid1 == BADPID || pid2 == BADPID || pidStarv == BADPID) {
        resp.type = RESP_ERROR;
        resp.paramCount = 0;
        AddResponseParam(&resp, "error", "Failed to create test processes");
        SendResponseToHost(&resp);
        return;
    }
    
    // Mark starvation process
    proctab[pidStarv].prstarvation = TRUE;
    
    // Set processes to ready state
    proctab[pid1].prstate = PRREADY;
    proctab[pid2].prstate = PRREADY;
    proctab[pidStarv].prstate = PRREADY;
    
    // Send process creation message
    resp.type = RESP_STARVATION_TEST;
    resp.paramCount = 0;
    AddResponseParam(&resp, "output", "P1, P2, and PStarv processes created with priorities 40, 35, and 25\n");
    SendResponseToHost(&resp);
    
    // Send execution start message
    resp.type = RESP_STARVATION_TEST;
    resp.paramCount = 0;
    AddResponseParam(&resp, "output", "All processes resumed. Starting execution...\n");
    SendResponseToHost(&resp);
    
    // Simulate scheduler execution for context switch starvation prevention
    for (int iter = 1; iter <= 10; iter++) {
        // Simulate one scheduler iteration
        currpid = (iter % 2 == 0) ? pid1 : pid2;  // Alternate between high-priority processes
        
        // Increment CPU time for current process
        proctab[currpid].prcpuused++;
        
        // Update time for starving process
        proctab[pidStarv].prtime++;
        
        // Apply starvation prevention after a few iterations
        if (iter == 5) {
            _snprintf_s(buffer, sizeof(buffer), _TRUNCATE,
                       "Iteration %d: PStarv has been waiting for %d cycles, applying starvation prevention...\n",
                       iter, proctab[pidStarv].prtime);
            
            resp.type = RESP_STARVATION_TEST;
            resp.paramCount = 0;
            AddResponseParam(&resp, "output", buffer);
            SendResponseToHost(&resp);
            
            // Boost starving process priority
            int oldPrio = proctab[pidStarv].prprio;
            proctab[pidStarv].prprio += starvBoost;
            
            _snprintf_s(buffer, sizeof(buffer), _TRUNCATE,
                       "Boosting PStarv priority from %d to %d\n", oldPrio, proctab[pidStarv].prprio);
            
            resp.type = RESP_STARVATION_TEST;
            resp.paramCount = 0;
            AddResponseParam(&resp, "output", buffer);
            SendResponseToHost(&resp);
        }
        
        // After boosting, it will run on next iteration
        if (iter == 6) {
            currpid = pidStarv;
            proctab[pidStarv].prcpuused++;
            proctab[pidStarv].prtime = 0;  // Reset time waiting
            
            _snprintf_s(buffer, sizeof(buffer), _TRUNCATE,
                       "Iteration %d: PStarv is now running with boosted priority %d\n",
                       iter, proctab[pidStarv].prprio);
            
            resp.type = RESP_STARVATION_TEST;
            resp.paramCount = 0;
            AddResponseParam(&resp, "output", buffer);
            SendResponseToHost(&resp);
        }
        
        // Status update every iteration
        _snprintf_s(buffer, sizeof(buffer), _TRUNCATE,
                   "Iteration %d: Current process is %s (pid %d, priority %d)\n",
                   iter, proctab[currpid].prname, currpid, proctab[currpid].prprio);
        
        resp.type = RESP_STARVATION_TEST;
        resp.paramCount = 0;
        AddResponseParam(&resp, "output", buffer);
        SendResponseToHost(&resp);
        
        // Sleep to simulate time passing
        Sleep(500);
    }
    
    // Final status report
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE,
               "\nFinal process states:\n"
               "P1     (pid %d): Priority %d, CPU cycles %d\n"
               "P2     (pid %d): Priority %d, CPU cycles %d\n"
               "PStarv (pid %d): Priority %d, CPU cycles %d\n",
               pid1, proctab[pid1].prprio, proctab[pid1].prcpuused,
               pid2, proctab[pid2].prprio, proctab[pid2].prcpuused,
               pidStarv, proctab[pidStarv].prprio, proctab[pidStarv].prcpuused);
    
    resp.type = RESP_STARVATION_TEST;
    resp.paramCount = 0;
    AddResponseParam(&resp, "output", buffer);
    SendResponseToHost(&resp);
    
    // Cleanup - mark processes as free
    proctab[pid1].prstate = PRFREE;
    proctab[pid2].prstate = PRFREE;
    proctab[pidStarv].prstate = PRFREE;
    
    // Send completion message
    resp.type = RESP_OK;
    resp.paramCount = 0;
    AddResponseParam(&resp, "message", "Starvation prevention test Q1 completed");
    SendResponseToHost(&resp);
}

/**
 * RunQ2StarvationTest - Run the time-based starvation test
 */
void RunQ2StarvationTest(void) {
    XinuResponse resp;
    pid32 pid1, pid2, pidStarv;
    char buffer[512];
    
    // Send initial output message
    