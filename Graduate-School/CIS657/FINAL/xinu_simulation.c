#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "xinu.h"

// Define simulation-specific function prototype
int xinu_main();

// Entry point for the simulation
int main(int argc, char *argv[]) {
    // Process command-line arguments
    char username[100] = "user";
    
    if (argc >= 2) {
        strncpy(username, argv[1], sizeof(username) - 1);
        username[sizeof(username) - 1] = '\0';
    }
    
    // Call the XINU main entry point
    return xinu_main();
}