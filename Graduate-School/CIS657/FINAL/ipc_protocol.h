/**
 * ipc_protocol.h - Interprocess Communication Protocol for XINU Simulation
 *
 * This file defines the communication protocol between the Windows host process
 * and the isolated XINU core process.
 */

// Define pipe names for communication
#define PIPE_HOST_TO_XINU "\\\\.\\pipe\\xinu_host_to_core"
#define PIPE_XINU_TO_HOST "\\\\.\\pipe\\xinu_core_to_host"

// Maximum message sizes
#define MAX_MESSAGE_LENGTH 1024
#define MAX_PARAM_VALUE 256
#define MAX_PARAMS 16

// Message types from host to XINU
typedef enum {
    CMD_INITIALIZE,          // Initialize XINU system
    CMD_CREATE_PROCESS,      // Create a new process
    CMD_START_SCHEDULER,     // Start the XINU scheduler
    CMD_RUN_STARVATION_TEST, // Run starvation prevention test
    CMD_GET_PROCESS_INFO,    // Get information about processes
    CMD_SHUTDOWN             // Shutdown XINU system
} HostCommandType;

// Message types from XINU to host
typedef enum {
    RESP_OK,                 // Success response
    RESP_ERROR,              // Error response
    RESP_PROCESS_CREATED,    // Process was created
    RESP_SCHEDULER_STATE,    // Current scheduler state
    RESP_PROCESS_INFO,       // Process information
    RESP_STARVATION_TEST     // Starvation test results
} XinuResponseType;

// Command structure sent from host to XINU
typedef struct {
    HostCommandType type;
    int paramCount;
    char paramNames[MAX_PARAMS][MAX_PARAM_VALUE];
    char paramValues[MAX_PARAMS][MAX_PARAM_VALUE];
} HostCommand;

// Response structure sent from XINU to host
typedef struct {
    XinuResponseType type;
    int paramCount;
    char paramNames[MAX_PARAMS][MAX_PARAM_VALUE];
    char paramValues[MAX_PARAMS][MAX_PARAM_VALUE];
} XinuResponse;

// Function prototypes for serializing/deserializing messages
// These are implemented in both host and XINU processes

// Serializes command to string format
int SerializeCommand(const HostCommand* cmd, char* buffer, int bufferSize);

// Deserializes string to command
int DeserializeCommand(const char* buffer, HostCommand* cmd);

// Serializes response to string format
int SerializeResponse(const XinuResponse* resp, char* buffer, int bufferSize);

// Deserializes string to response
int DeserializeResponse(const char* buffer, XinuResponse* resp);

// Helper functions
void AddCommandParam(HostCommand* cmd, const char* name, const char* value);
void AddResponseParam(XinuResponse* resp, const char* name, const char* value);
const char* GetCommandParam(const HostCommand* cmd, const char* name);
const char* GetResponseParam(const XinuResponse* resp, const char* name);

#endif /* IPC_PROTOCOL_H */
