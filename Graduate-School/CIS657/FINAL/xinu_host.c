/**
 * xinu_host.c - Windows host process for XINU simulation
 * 
 * This program creates and communicates with the XINU core process
 * via named pipes. It provides a user interface for interacting with
 * the XINU scheduler.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <process.h>
#include "ipc_protocol.h"

// Global variables
HANDLE pipeToXinu = INVALID_HANDLE_VALUE;   // Pipe for sending commands to XINU
HANDLE pipeFromXinu = INVALID_HANDLE_VALUE; // Pipe for receiving responses from XINU
PROCESS_INFORMATION xinuProcessInfo;        // XINU process information
char currentUser[256] = "user";             // Current Windows username
char xinuExecutablePath[512] = "";          // Path to XINU executable
BOOL shutdownRequested = FALSE;             // Flag to indicate shutdown

// Function prototypes
BOOL StartXinuProcess(void);
BOOL InitializePipes(void);
BOOL SendCommandToXinu(HostCommand* cmd);
BOOL ReceiveResponseFromXinu(XinuResponse* resp);
void HandleUserCommands(void);
void CleanupAndExit(int exitCode);
void DisplayWelcome(void);
void DisplayHelp(void);

// Host-side implementation of IPC protocol functions
int SerializeCommand(const HostCommand* cmd, char* buffer, int bufferSize) {
    // Format: type|paramCount|name1=value1|name2=value2|...
    int offset = _snprintf_s(buffer, bufferSize, bufferSize, "%d|%d", cmd->type, cmd->paramCount);
    
    for (int i = 0; i < cmd->paramCount && offset < bufferSize; i++) {
        offset += _snprintf_s(buffer + offset, bufferSize - offset, bufferSize - offset, 
                             "|%s=%s", cmd->paramNames[i], cmd->paramValues[i]);
    }
    
    return offset;
}

int DeserializeResponse(const char* buffer, XinuResponse* resp) {
    char* context = NULL;
    char* token = NULL;
    char tempBuffer[MAX_MESSAGE_LENGTH];
    
    // Copy buffer to avoid modifying original
    strncpy_s(tempBuffer, sizeof(tempBuffer), buffer, _TRUNCATE);
    
    // Extract type
    token = strtok_s(tempBuffer, "|", &context);
    if (!token) return 0;
    resp->type = (XinuResponseType)atoi(token);
    
    // Extract parameter count
    token = strtok_s(NULL, "|", &context);
    if (!token) return 0;
    resp->paramCount = atoi(token);
    
    // Extract parameters
    for (int i = 0; i < resp->paramCount; i++) {
        // Get name=value pair
        token = strtok_s(NULL, "|", &context);
        if (!token) break;
        
        // Split into name and value
        char* equals = strchr(token, '=');
        if (!equals) continue;
        
        // Extract name
        int nameLen = (int)(equals - token);
        strncpy_s(resp->paramNames[i], MAX_PARAM_VALUE, token, nameLen);
        resp->paramNames[i][nameLen] = '\0';
        
        // Extract value
        strncpy_s(resp->paramValues[i], MAX_PARAM_VALUE, equals + 1, _TRUNCATE);
    }
    
    return 1;
}

void AddCommandParam(HostCommand* cmd, const char* name, const char* value) {
    if (cmd->paramCount < MAX_PARAMS) {
        strncpy_s(cmd->paramNames[cmd->paramCount], MAX_PARAM_VALUE, name, _TRUNCATE);
        strncpy_s(cmd->paramValues[cmd->paramCount], MAX_PARAM_VALUE, value, _TRUNCATE);
        cmd->paramCount++;
    }
}

const char* GetResponseParam(const XinuResponse* resp, const char* name) {
    for (int i = 0; i < resp->paramCount; i++) {
        if (_stricmp(resp->paramNames[i], name) == 0) {
            return resp->paramValues[i];
        }
    }
    return NULL;
}

/**
 * StartXinuProcess - Start the XINU core process
 */
BOOL StartXinuProcess(void) {
    printf("Starting XINU core process: %s\n", xinuExecutablePath);
    
    STARTUPINFO startupInfo;
    ZeroMemory(&startupInfo, sizeof(STARTUPINFO));
    startupInfo.cb = sizeof(STARTUPINFO);
    
    ZeroMemory(&xinuProcessInfo, sizeof(PROCESS_INFORMATION));
    
    // Start the XINU process with pipe names as arguments
    char commandLine[1024];
    _snprintf_s(commandLine, sizeof(commandLine), _TRUNCATE, 
                "\"%s\" %s %s", xinuExecutablePath, PIPE_HOST_TO_XINU, PIPE_XINU_TO_HOST);
    
    if (!CreateProcess(
        NULL,               // Application name
        commandLine,        // Command line
        NULL,               // Process security attributes
        NULL,               // Thread security attributes
        FALSE,              // Inherit handles
        0,                  // Creation flags
        NULL,               // Environment
        NULL,               // Current directory
        &startupInfo,       // Startup info
        &xinuProcessInfo    // Process information
    )) {
        printf("Failed to start XINU process: %d\n", GetLastError());
        return FALSE;
    }
    
    printf("XINU core process started successfully (PID: %d)\n", xinuProcessInfo.dwProcessId);
    return TRUE;
}

/**
 * InitializePipes - Create and connect named pipes for IPC
 */
BOOL InitializePipes(void) {
    // Create pipes first - XINU process will connect to these
    pipeToXinu = CreateNamedPipe(
        TEXT(PIPE_HOST_TO_XINU),           // Pipe name
        PIPE_ACCESS_OUTBOUND,              // Outbound pipe
        PIPE_TYPE_MESSAGE | PIPE_WAIT,     // Message-type and blocking mode
        1,                                 // Max instances
        MAX_MESSAGE_LENGTH,                // Output buffer size
        MAX_MESSAGE_LENGTH,                // Input buffer size
        0,                                 // Default timeout
        NULL                               // Security attributes
    );
    
    if (pipeToXinu == INVALID_HANDLE_VALUE) {
        printf("Failed to create outbound pipe: %d\n", GetLastError());
        return FALSE;
    }
    
    pipeFromXinu = CreateNamedPipe(
        TEXT(PIPE_XINU_TO_HOST),           // Pipe name
        PIPE_ACCESS_INBOUND,               // Inbound pipe
        PIPE_TYPE_MESSAGE | PIPE_WAIT,     // Message-type and blocking mode
        1,                                 // Max instances
        MAX_MESSAGE_LENGTH,                // Output buffer size
        MAX_MESSAGE_LENGTH,                // Input buffer size
        0,                                 // Default timeout
        NULL                               // Security attributes
    );
    
    if (pipeFromXinu == INVALID_HANDLE_VALUE) {
        printf("Failed to create inbound pipe: %d\n", GetLastError());
        CloseHandle(pipeToXinu);
        return FALSE;
    }
    
    printf("Pipes created, waiting for XINU process to connect...\n");
    
    // Connect outbound pipe
    if (!ConnectNamedPipe(pipeToXinu, NULL) && GetLastError() != ERROR_PIPE_CONNECTED) {
        printf("Failed to connect outbound pipe: %d\n", GetLastError());
        CloseHandle(pipeToXinu);
        CloseHandle(pipeFromXinu);
        return FALSE;
    }
    
    // Connect inbound pipe
    if (!ConnectNamedPipe(pipeFromXinu, NULL) && GetLastError() != ERROR_PIPE_CONNECTED) {
        printf("Failed to connect inbound pipe: %d\n", GetLastError());
        CloseHandle(pipeToXinu);
        CloseHandle(pipeFromXinu);
        return FALSE;
    }
    
    printf("Pipes connected successfully\n");
    return TRUE;
}

/**
 * SendCommandToXinu - Send a command to the XINU process
 */
BOOL SendCommandToXinu(HostCommand* cmd) {
    char buffer[MAX_MESSAGE_LENGTH];
    DWORD bytesWritten;
    
    int length = SerializeCommand(cmd, buffer, sizeof(buffer));
    if (length <= 0) {
        printf("Failed to serialize command\n");
        return FALSE;
    }
    
    if (!WriteFile(pipeToXinu, buffer, length, &bytesWritten, NULL)) {
        printf("Failed to send command to XINU: %d\n", GetLastError());
        return FALSE;
    }
    
    return TRUE;
}

/**
 * ReceiveResponseFromXinu - Receive a response from the XINU process
 */
BOOL ReceiveResponseFromXinu(XinuResponse* resp) {
    char buffer[MAX_MESSAGE_LENGTH];
    DWORD bytesRead;
    
    if (!ReadFile(pipeFromXinu, buffer, sizeof(buffer), &bytesRead, NULL)) {
        printf("Failed to receive response from XINU: %d\n", GetLastError());
        return FALSE;
    }
    
    buffer[bytesRead] = '\0';
    
    if (!DeserializeResponse(buffer, resp)) {
        printf("Failed to deserialize response\n");
        return FALSE;
    }
    
    return TRUE;
}

/**
 * InitializeXinuSystem - Initialize the XINU system
 */
BOOL InitializeXinuSystem(void) {
    HostCommand cmd;
    XinuResponse resp;
    
    printf("Initializing XINU system...\n");
    
    cmd.type = CMD_INITIALIZE;
    cmd.paramCount = 0;
    AddCommandParam(&cmd, "username", currentUser);
    
    if (!SendCommandToXinu(&cmd)) {
        printf("Failed to send initialization command\n");
        return FALSE;
    }
    
    if (!ReceiveResponseFromXinu(&resp)) {
        printf("Failed to receive initialization response\n");
        return FALSE;
    }
    
    if (resp.type == RESP_OK) {
        const char* message = GetResponseParam(&resp, "message");
        if (message) {
            printf("%s\n", message);
        }
        return TRUE;
    } else {
        const char* error = GetResponseParam(&resp, "error");
        if (error) {
            printf("Error: %s\n", error);
        } else {
            printf("Unknown initialization error\n");
        }
        return FALSE;
    }
}

/**
 * RunStarvationTest - Run starvation prevention test
 */
void RunStarvationTest(const char* testType) {
    HostCommand cmd;
    XinuResponse resp;
    
    printf("Running starvation test '%s'...\n", testType);
    
    cmd.type = CMD_RUN_STARVATION_TEST;
    cmd.paramCount = 0;
    AddCommandParam(&cmd, "type", testType);
    
    if (!SendCommandToXinu(&cmd)) {
        printf("Failed to send starvation test command\n");
        return;
    }
    
    // Handle the streaming output from the test
    while (1) {
        if (!ReceiveResponseFromXinu(&resp)) {
            printf("Failed to receive starvation test response\n");
            return;
        }
        
        if (resp.type == RESP_OK) {
            const char* message = GetResponseParam(&resp, "message");
            if (message) {
                printf("%s\n", message);
            }
            break;
        } else if (resp.type == RESP_STARVATION_TEST) {
            const char* output = GetResponseParam(&resp, "output");
            if (output) {
                printf("%s", output);
            }
        } else {
            const char* error = GetResponseParam(&resp, "error");
            if (error) {
                printf("Error: %s\n", error);
            }
            break;
        }
    }
}

/**
 * GetProcessInfo - Get information about processes
 */
void GetProcessInfo(void) {
    HostCommand cmd;
    XinuResponse resp;
    
    printf("Getting process information...\n");
    
    cmd.type = CMD_GET_PROCESS_INFO;
    cmd.paramCount = 0;
    
    if (!SendCommandToXinu(&cmd)) {
        printf("Failed to send process info command\n");
        return;
    }
    
    if (!ReceiveResponseFromXinu(&resp)) {
        printf("Failed to receive process info response\n");
        return;
    }
    
    if (resp.type == RESP_PROCESS_INFO) {
        const char* processCount = GetResponseParam(&resp, "count");
        if (processCount) {
            printf("Active processes: %s\n", processCount);
        }
        
        // Display each process
        for (int i = 0; i < resp.paramCount; i++) {
            if (strncmp(resp.paramNames[i], "process", 7) == 0) {
                printf("%s\n", resp.paramValues[i]);
            }
        }
    } else {
        const char* error = GetResponseParam(&resp, "error");
        if (error) {
            printf("Error: %s\n", error);
        } else {
            printf("Unknown error retrieving process information\n");
        }
    }
}

/**
 * ShutdownXinuSystem - Send shutdown command to XINU
 */
void ShutdownXinuSystem(void) {
    HostCommand cmd;
    XinuResponse resp;
    
    printf("Shutting down XINU system...\n");
    
    cmd.type = CMD_SHUTDOWN;
    cmd.paramCount = 0;
    
    if (!SendCommandToXinu(&cmd)) {
        printf("Failed to send shutdown command\n");
        return;
    }
    
    if (!ReceiveResponseFromXinu(&resp)) {
        printf("Failed to receive shutdown response\n");
        return;
    }
    
    if (resp.type == RESP_OK) {
        const char* message = GetResponseParam(&resp, "message");
        if (message) {
            printf("%s\n", message);
        }
    } else {
        const char* error = GetResponseParam(&resp, "error");
        if (error) {
            printf("Error: %s\n", error);
        } else {
            printf("Unknown error during shutdown\n");
        }
    }
}

/**
 * HandleUserCommands - Process user commands from the console
 */
void HandleUserCommands(void) {
    char input[256];
    
    DisplayWelcome();
    
    while (!shutdownRequested) {
        printf("\nxinu> ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nEnd of input, exiting...\n");
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = '\0';
        
        // Process commands
        if (strcmp(input, "help") == 0) {
            DisplayHelp();
        } else if (strcmp(input, "starvation_test_Q1") == 0) {
            RunStarvationTest("Q1");
        } else if (strcmp(input, "starvation_test_Q2") == 0) {
            RunStarvationTest("Q2");
        } else if (strcmp(input, "ps") == 0) {
            GetProcessInfo();
        } else if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            shutdownRequested = TRUE;
            ShutdownXinuSystem();
            break;
        } else if (strlen(input) > 0) {
            printf("Unknown command: %s\n", input);
            printf("Type 'help' for a list of commands\n");
        }
    }
}

/**
 * CleanupAndExit - Clean up resources and exit
 */
void CleanupAndExit(int exitCode) {
    // Close pipe handles
    if (pipeToXinu != INVALID_HANDLE_VALUE) {
        CloseHandle(pipeToXinu);
    }
    
    if (pipeFromXinu != INVALID_HANDLE_VALUE) {
        CloseHandle(pipeFromXinu);
    }
    
    // Terminate XINU process if still running
    if (xinuProcessInfo.hProcess != NULL) {
        // Try graceful shutdown first, then force if needed
        if (!shutdownRequested) {
            ShutdownXinuSystem();
        }
        
        // Wait a bit for graceful shutdown
        WaitForSingleObject(xinuProcessInfo.hProcess, 1000);
        
        // Check if still running
        DWORD exitCode;
        if (GetExitCodeProcess(xinuProcessInfo.hProcess, &exitCode) && exitCode == STILL_ACTIVE) {
            // Force termination
            TerminateProcess(xinuProcessInfo.hProcess, 0);
        }
        
        CloseHandle(xinuProcessInfo.hProcess);
        CloseHandle(xinuProcessInfo.hThread);
    }
    
    exit(exitCode);
}

/**
 * DisplayWelcome - Show welcome message
 */
void DisplayWelcome(void) {
    time_t now;
    struct tm timeinfo;
    char timestr[64];
    
    time(&now);
    gmtime_s(&timeinfo, &now);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &timeinfo);
    
    printf("\n===================================================================\n");
    printf("XINU Starvation Prevention Simulation\n");
    printf("User: %s\n", currentUser);
    printf("Date: %s UTC\n", timestr);
    printf("===================================================================\n\n");
    
    printf("Type 'help' for a list of commands\n");
}

/**
 * DisplayHelp - Show help information
 */
void DisplayHelp(void) {
    printf("\nAvailable commands:\n");
    printf("  help                - Display this help message\n");
    printf("  starvation_test_Q1  - Run Q1 demonstration (context switch based priority boosting)\n");
    printf("  starvation_test_Q2  - Run Q2 demonstration (time based priority boosting)\n");
    printf("  ps                  - Show process information\n");
    printf("  exit, quit          - Exit the simulation\n");
}

/**
 * main - Entry point for host process
 */
int main(int argc, char *argv[]) {
    // Process command-line arguments
    if (argc < 2) {
        printf("Usage: %s <xinu_executable_path> [username]\n", argv[0]);
        return 1;
    }
    
    // Store XINU executable path
    strncpy_s(xinuExecutablePath, sizeof(xinuExecutablePath), argv[1], _TRUNCATE);
    
    // Store username if provided
    if (argc >= 3) {
        strncpy_s(currentUser, sizeof(currentUser), argv[2], _TRUNCATE);
    }
    
    // Set up console control handler for graceful termination
    if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CleanupAndExit, TRUE)) {
        printf("Warning: Could not set console control handler\n");
    }
    
    // Start XINU process
    if (!StartXinuProcess()) {
        return 1;
    }
    
    // Initialize pipes
    if (!InitializePipes()) {
        // Error message already displayed
        CleanupAndExit(1);
    }
    
    // Initialize XINU system
    if (!InitializeXinuSystem()) {
        CleanupAndExit(1);
    }
    
    // Process user commands
    HandleUserCommands();
    
    // Clean up and exit
    CleanupAndExit(0);
    return 0;  // Never reached
}