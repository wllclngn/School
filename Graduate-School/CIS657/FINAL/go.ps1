# PowerShell Script for XINU Starvation Prevention Simulation
# Author: wllclngn
# Date: 2025-06-15

# Configuration Variables
$projectDir = $PSScriptRoot
$outputDir = Join-Path -Path $projectDir -ChildPath "sim_output"

# Function to execute a command and capture its output
function Invoke-CommandLine {
    param (
        [string]$command,
        [string]$workingDirectory = $PSScriptRoot,
        [switch]$noOutput
    )
    
    Write-Host "Executing: $command" -ForegroundColor Yellow
    
    $processInfo = New-Object System.Diagnostics.ProcessStartInfo
    $processInfo.FileName = "cmd.exe"
    $processInfo.Arguments = "/c $command"
    $processInfo.RedirectStandardError = $true
    $processInfo.RedirectStandardOutput = $true
    $processInfo.UseShellExecute = $false
    $processInfo.WorkingDirectory = $workingDirectory
    
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $processInfo
    $process.Start() | Out-Null
    
    if (-not $noOutput) {
        while (-not $process.StandardOutput.EndOfStream) {
            $line = $process.StandardOutput.ReadLine()
            Write-Host $line
        }
        
        while (-not $process.StandardError.EndOfStream) {
            $line = $process.StandardError.ReadLine()
            Write-Host $line -ForegroundColor Red
        }
    }
    
    $process.WaitForExit()
    return $process.ExitCode
}

# Create XINU simulation environment
function Create-XINUSimulation {
    Write-Host "`nPreparing XINU simulation environment..." -ForegroundColor Cyan
    
    # Create output directory if it doesn't exist
    if (-not (Test-Path $outputDir)) {
        New-Item -Path $outputDir -ItemType Directory | Out-Null
    } else {
        # Clean the output directory
        Remove-Item -Path "$outputDir\*" -Force
    }
    
    # Source files from your repository
    $p1File = Join-Path -Path $projectDir -ChildPath "shell/p1_process.c"
    $p2File = Join-Path -Path $projectDir -ChildPath "shell/p2_process.c"
    $pstarvFile = Join-Path -Path $projectDir -ChildPath "shell/pstarv_process.c"
    $starvationShellFile = Join-Path -Path $projectDir -ChildPath "shell/starvation_shell.c"
    $pstarvGlobalsFile = Join-Path -Path $projectDir -ChildPath "shell/pstarv_globals.c"
    
    # Check if source files exist
    $missingFiles = @()
    if (-not (Test-Path $p1File)) { $missingFiles += "shell/p1_process.c" }
    if (-not (Test-Path $p2File)) { $missingFiles += "shell/p2_process.c" }
    if (-not (Test-Path $pstarvFile)) { $missingFiles += "shell/pstarv_process.c" }
    if (-not (Test-Path $starvationShellFile)) { $missingFiles += "shell/starvation_shell.c" }
    if (-not (Test-Path $pstarvGlobalsFile)) { $missingFiles += "shell/pstarv_globals.c" }
    
    if ($missingFiles.Count -gt 0) {
        Write-Host "ERROR: The following required source files were not found:" -ForegroundColor Red
        foreach ($file in $missingFiles) {
            Write-Host "  - $file" -ForegroundColor Red
        }
        Write-Host "Please make sure all required source files are in the correct locations." -ForegroundColor Yellow
        return $false
    }
    
    # Create the header files needed for compilation
    
    # xinu.h - Main XINU Header
    $xinuHeaderPath = Join-Path -Path $outputDir -ChildPath "xinu.h"
    $xinuHeaderContent = @"
/* xinu.h - Simplified XINU header for Windows simulation */

#ifndef _XINU_H_
#define _XINU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Windows.h>

/* Constants */
#define NPROC           100     /* Number of user processes */
#define BADPID          -1      /* Invalid process ID */
#define OK              1       /* Normal system call return */
#define SYSERR          -1      /* System call failed return */
#define TRUE            1       /* Boolean true */
#define FALSE           0       /* Boolean false */
#define SHELL_OK        0       /* Shell command return value */
#define SHELL_ERROR     1       /* Shell error return value */
#define CONSOLE         0       /* Console device ID */
#define CLKTICKS_PER_SEC 100    /* Clock ticks per second */

/* Process state constants */
#define PR_FREE         0       /* Process table entry is unused */
#define PR_CURR         1       /* Process is currently running */
#define PR_READY        2       /* Process is on ready queue */
#define PR_RECV         3       /* Process waiting for message */
#define PR_SLEEP        4       /* Process is sleeping */
#define PR_SUSP         5       /* Process is suspended */
#define PR_WAIT         6       /* Process is on semaphore queue */
#define PR_RECTIM       7       /* Process is receiving with timeout */

/* Type definitions */
typedef int pid32;              /* Process ID type */
typedef int pri16;              /* Process priority type */
typedef unsigned int uint32;    /* Unsigned 32-bit integer */
typedef int syscall;            /* System call declaration */
typedef int bool8;              /* Boolean type */
typedef unsigned short uint16;  /* Unsigned 16-bit integer */
typedef int shellcmd;           /* Shell command return type */
typedef int did32;              /* Device ID type */
typedef int intmask;            /* Interrupt mask */

/* Process table entry */
struct procent {
    char    prname[16];         /* Process name */
    pid32   prpid;              /* Process ID */
    pri16   prprio;             /* Process priority */
    int     prstate;            /* Process state */
    void    *prstkptr;          /* Stack pointer */
    pid32   prparent;           /* Process parent */
    uint32  prstklen;           /* Stack length */
    char    *prsem;             /* Semaphore */
    bool8   prhasmsg;           /* Has message */
    uint16  prdesc[16];         /* Device descriptors */
};

/* Function declarations */
extern void kprintf(const char *fmt, ...);
extern syscall ready(pid32 pid);
extern syscall resume(pid32 pid);
extern syscall yield(void);
extern pri16 getprio(pid32 pid);
extern pri16 chprio(pid32 pid, pri16 newprio);
extern syscall kill(pid32 pid);
extern syscall sleep(uint32 delay);
extern pid32 create(void (*procaddr)(), uint32 stksize, pri16 priority, char *name, uint32 nargs, ...);
extern syscall receive(void);
extern syscall recvclr(void);

/* Global variables */
extern struct procent proctab[];
extern pid32 currpid;
extern uint32 clktime;
extern uint32 clkticks;

/* Macro definitions */
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* Special for Windows simulation */
void update_system_time(void);
void insert(pid32 pid, int head, int key);
pid32 getitem(pid32 pid);

#endif /* _XINU_H_ */
"@
    Set-Content -Path $xinuHeaderPath -Value $xinuHeaderContent -Encoding ASCII
    
    # pstarv.h - Starvation prevention header
    $pstarvHeaderPath = Join-Path -Path $outputDir -ChildPath "pstarv.h"
    $pstarvHeaderContent = @"
/* pstarv.h - Header for starvation prevention */

#ifndef _PSTARV_H_
#define _PSTARV_H_

#include "xinu.h"

/* Global variables for starvation prevention */
extern bool8 enable_starvation_fix;
extern pid32 pstarv_pid;
extern uint32 pstarv_ready_time;
extern uint32 last_boost_time;

/* Function prototypes */
void check_pstarv_time(void);
shellcmd starvation_test(int nargs, char *args[]);
shellcmd starvation_test2(int nargs, char *args[]);

/* Process function declarations */
void p1_func(void);
void p2_func(void);
void pstarv_func(void);

#endif
"@
    Set-Content -Path $pstarvHeaderPath -Value $pstarvHeaderContent -Encoding ASCII
    
    # Copy your source files to the simulation directory with modified extensions
    Copy-Item -Path $p1File -Destination (Join-Path -Path $outputDir -ChildPath "p1_process.c") -Force
    Copy-Item -Path $p2File -Destination (Join-Path -Path $outputDir -ChildPath "p2_process.c") -Force
    Copy-Item -Path $pstarvFile -Destination (Join-Path -Path $outputDir -ChildPath "pstarv_process.c") -Force
    Copy-Item -Path $starvationShellFile -Destination (Join-Path -Path $outputDir -ChildPath "starvation_shell.c") -Force
    Copy-Item -Path $pstarvGlobalsFile -Destination (Join-Path -Path $outputDir -ChildPath "pstarv_globals.c") -Force
    
    Write-Host "Copied XINU source files to simulation directory" -ForegroundColor Green
    
    # Create main wrapper file
    $mainPath = Join-Path -Path $outputDir -ChildPath "xinu_main.c"
    $mainContent = @"
/**
 * XINU Simulation Main File
 * Author: wllclngn
 * Date: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
 */

#include "xinu.h"
#include "pstarv.h"

/* Process table and globals */
struct procent proctab[NPROC];
pid32 currpid = 0;
uint32 clktime = 0;
uint32 clkticks = 0;

/* Ready list */
struct {
    pid32 proc_ids[NPROC];
    int count;
} readylist;

/* Update system time */
void update_system_time(void) {
    static DWORD last_tick = 0;
    DWORD current_tick = GetTickCount();
    
    if (last_tick == 0) {
        last_tick = current_tick;
        return;
    }
    
    DWORD elapsed = current_tick - last_tick;
    clkticks += elapsed;
    while (clkticks >= 1000) {
        clktime++;
        clkticks -= 1000;
    }
    
    last_tick = current_tick;
}

/* XINU API implementation for Windows simulation */
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* Insert process into ready list */
void insert(pid32 pid, int head, int key) {
    int i;
    
    /* Find insertion point based on priority */
    for (i = 0; i < readylist.count; i++) {
        if (proctab[readylist.proc_ids[i]].prprio < key) {
            break;
        }
    }
    
    /* Shift elements */
    for (int j = readylist.count; j > i; j--) {
        readylist.proc_ids[j] = readylist.proc_ids[j-1];
    }
    
    /* Insert process */
    readylist.proc_ids[i] = pid;
    readylist.count++;
}

/* Remove process from ready list */
pid32 getitem(pid32 pid) {
    int i;
    
    /* Find process in ready list */
    for (i = 0; i < readylist.count; i++) {
        if (readylist.proc_ids[i] == pid) {
            break;
        }
    }
    
    /* Process not found */
    if (i >= readylist.count) {
        return SYSERR;
    }
    
    /* Shift elements */
    for (; i < readylist.count - 1; i++) {
        readylist.proc_ids[i] = readylist.proc_ids[i+1];
    }
    
    readylist.count--;
    return pid;
}

/* Create a new process */
pid32 create(void (*procaddr)(), uint32 stksize, pri16 priority, char *name, uint32 nargs, ...) {
    pid32 pid;
    
    /* Find free slot in process table */
    for (pid = 0; pid < NPROC; pid++) {
        if (proctab[pid].prstate == PR_FREE) {
            break;
        }
    }
    
    if (pid >= NPROC) {
        kprintf("ERROR: No free process slots\n");
        return SYSERR;
    }
    
    /* Initialize process table entry */
    strncpy_s(proctab[pid].prname, sizeof(proctab[pid].prname), name, _TRUNCATE);
    proctab[pid].prpid = pid;
    proctab[pid].prprio = priority;
    proctab[pid].prstate = PR_SUSP;
    
    kprintf("Created process '%s' with PID %d and priority %d\n", name, pid, priority);
    
    return pid;
}

/* Kill a process */
syscall kill(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    proctab[pid].prstate = PR_FREE;
    return OK;
}

/* Sleep for a specified time */
syscall sleep(uint32 delay) {
    Sleep(delay * 1000);
    return OK;
}

/* Make a process ready */
syscall ready(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    proctab[pid].prstate = PR_READY;
    insert(pid, 0, proctab[pid].prprio);
    
    /* If this is the starving process, record its ready time */
    if (pid == pstarv_pid) {
        pstarv_ready_time = clktime;
    }
    
    return OK;
}

/* Resume a suspended process */
syscall resume(pid32 pid) {
    if (pid < 0 || pid >= NPROC || proctab[pid].prstate != PR_SUSP) {
        return SYSERR;
    }
    
    return ready(pid);
}

/* Get process priority */
pri16 getprio(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    return proctab[pid].prprio;
}

/* Change process priority */
pri16 chprio(pid32 pid, pri16 newprio) {
    pri16 oldprio;
    
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    oldprio = proctab[pid].prprio;
    proctab[pid].prprio = newprio;
    
    /* Update position in ready list if process is ready */
    if (proctab[pid].prstate == PR_READY) {
        getitem(pid);
        insert(pid, 0, newprio);
    }
    
    return oldprio;
}

/* Yield processor to another process */
syscall yield(void) {
    /* Select highest priority process from ready list */
    if (readylist.count > 0) {
        pid32 next_pid = readylist.proc_ids[0];
        getitem(next_pid);
        
        /* Put current process back in ready list */
        proctab[currpid].prstate = PR_READY;
        insert(currpid, 0, proctab[currpid].prprio);
        
        /* Switch to new process */
        proctab[next_pid].prstate = PR_CURR;
        pid32 old_pid = currpid;
        currpid = next_pid;
        
        kprintf("*** CONTEXT SWITCH: From process %d (%s) to %d (%s) ***\n", 
            old_pid, proctab[old_pid].prname, currpid, proctab[currpid].prname);
    }
    
    return OK;
}

/* Stub functions for completeness */
syscall receive(void) {
    return 0;
}

syscall recvclr(void) {
    return OK;
}

/* Initialize the system */
void initialize_system(void) {
    int i;
    
    /* Initialize process table */
    for (i = 0; i < NPROC; i++) {
        proctab[i].prstate = PR_FREE;
    }
    
    /* Initialize ready list */
    readylist.count = 0;
    
    /* Initialize time */
    clktime = 0;
    clkticks = 0;
    
    /* Initialize process 0 as current */
    proctab[0].prstate = PR_CURR;
    strncpy_s(proctab[0].prname, sizeof(proctab[0].prname), "prnull", _TRUNCATE);
    proctab[0].prprio = 0;
    currpid = 0;
}

/* Execute a process function */
void execute_process(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return;
    }
    
    /* Set as current process */
    currpid = pid;
    proctab[pid].prstate = PR_CURR;
    
    /* Execute the function based on process name */
    if (strcmp(proctab[pid].prname, "P1") == 0 || 
        strcmp(proctab[pid].prname, "P1_Process") == 0) {
        p1_func();
    }
    else if (strcmp(proctab[pid].prname, "P2") == 0 || 
             strcmp(proctab[pid].prname, "P2_Process") == 0) {
        p2_func();
    }
    else if (strcmp(proctab[pid].prname, "Pstarv") == 0 || 
             strcmp(proctab[pid].prname, "Pstarv_Process") == 0) {
        pstarv_func();
    }
}

/* Main function */
int main(void) {
    /* Initialize the simulation */
    initialize_system();
    
    /* Initialize time */
    update_system_time();
    
    kprintf("\n======================================================\n");
    kprintf("XINU Starvation Prevention Simulation\n");
    kprintf("Using your actual source files from repository\n");
    kprintf("Date: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")\n");
    kprintf("======================================================\n\n");
    
    /* Run the starvation test using your implementation */
    char *args[] = {"starvation_test2"};
    starvation_test2(1, args);
    
    /* Main simulation loop */
    int iterations = 0;
    int max_iterations = 100;
    
    while (iterations < max_iterations) {
        /* Update system time */
        update_system_time();
        
        /* Check for time-based starvation prevention */
        check_pstarv_time();
        
        /* If we have ready processes, run the highest priority one */
        if (readylist.count > 0) {
            pid32 next_pid = readylist.proc_ids[0];
            getitem(next_pid);
            
            /* Switch to new process */
            pid32 old_pid = currpid;
            
            if (proctab[old_pid].prstate == PR_CURR) {
                proctab[old_pid].prstate = PR_READY;
                insert(old_pid, 0, proctab[old_pid].prprio);
            }
            
            currpid = next_pid;
            proctab[currpid].prstate = PR_CURR;
            
            kprintf("*** CONTEXT SWITCH: From process %d to %d (%s) ***\n", 
                old_pid, currpid, proctab[currpid].prname);
            
            /* Execute the process */
            execute_process(currpid);
        }
        
        /* Check if all processes are done */
        int active_count = 0;
        for (int i = 1; i < NPROC; i++) { /* Skip process 0 */
            if (proctab[i].prstate != PR_FREE) {
                active_count++;
            }
        }
        
        if (active_count == 0) {
            break;
        }
        
        /* Small delay to prevent CPU hogging */
        Sleep(100);
        iterations++;
    }
    
    kprintf("\n======================================================\n");
    kprintf("Simulation completed after %d iterations\n", iterations);
    kprintf("======================================================\n");
    
    return 0;
}
"@
    Set-Content -Path $mainPath -Value $mainContent -Encoding ASCII
    
    Write-Host "Created simulation main file: $mainPath" -ForegroundColor Green
    
    return @{
        MainPath = $mainPath
        OutputDir = $outputDir
    }
}

# Build the simulation
function Build-XINUSimulation {
    param (
        [string]$mainPath,
        [string]$outputDir
    )
    
    Write-Host "`nBuilding XINU simulation with your files..." -ForegroundColor Cyan
    
    # Find Visual Studio
    $vsWherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $vsDevCmdPath = $null
    
    if (Test-Path $vsWherePath) {
        $vsPath = & $vsWherePath -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            $vsDevCmdPath = Join-Path $vsPath "Common7\Tools\VsDevCmd.bat"
        }
    }
    
    if (-not $vsDevCmdPath -or -not (Test-Path $vsDevCmdPath)) {
        Write-Host "ERROR: Visual Studio Developer Command Prompt not found." -ForegroundColor Red
        Write-Host "Please install Visual Studio with C++ development tools." -ForegroundColor Yellow
        return $false
    }
    
    # Create compiled files directory if needed
    $compiledDir = Join-Path -Path $outputDir -ChildPath "compiled"
    if (-not (Test-Path $compiledDir)) {
        New-Item -Path $compiledDir -ItemType Directory -Force | Out-Null
    } else {
        # Clean the directory
        Remove-Item -Path "$compiledDir\*" -Force -ErrorAction SilentlyContinue
    }
    
    # Get all C files in the output directory
    $cFiles = Get-ChildItem -Path $outputDir -Filter "*.c" | ForEach-Object { $_.FullName }
    
    $outputExe = Join-Path -Path $outputDir -ChildPath "xinu_simulation.exe"
    $compileBatch = Join-Path $env:TEMP "xinu_compile.bat"
    
    $batchContent = @"
@echo off
echo Setting up Visual Studio environment...
call "$vsDevCmdPath"
echo Compiling XINU simulation files...
cd "$outputDir"
cl.exe /nologo /W4 /EHsc /c "$mainPath" /I"$outputDir"
cl.exe /nologo /W4 /EHsc /c p1_process.c /I"$outputDir"
cl.exe /nologo /W4 /EHsc /c p2_process.c /I"$outputDir"
cl.exe /nologo /W4 /EHsc /c pstarv_process.c /I"$outputDir"
cl.exe /nologo /W4 /EHsc /c pstarv_globals.c /I"$outputDir"
cl.exe /nologo /W4 /EHsc /c starvation_shell.c /I"$outputDir"
echo Linking XINU simulation...
link.exe /OUT:"$outputExe" *.obj
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)
echo Compilation successful!
"@
    
    # Write the batch file and execute it
    $batchContent | Out-File -FilePath $compileBatch -Encoding ASCII
    
    Write-Host "Executing compiler with your files..." -ForegroundColor Yellow
    $result = Invoke-CommandLine $compileBatch
    Remove-Item $compileBatch
    
    # Check if compilation was successful
    if (Test-Path $outputExe) {
        Write-Host "Successfully built XINU simulation at $outputExe" -ForegroundColor Green
        return $outputExe
    } else {
        Write-Host "ERROR: Failed to build XINU simulation" -ForegroundColor Red
        return $false
    }
}

# Run the simulation
function Run-XINUSimulation {
    param (
        [string]$executable
    )
    
    Write-Host "`nRunning XINU simulation..." -ForegroundColor Cyan
    
    if (-not (Test-Path $executable)) {
        Write-Host "ERROR: Executable not found at $executable" -ForegroundColor Red
        return $false
    }
    
    # Run the simulation
    Write-Host "Starting XINU starvation prevention demonstration..." -ForegroundColor Yellow
    Invoke-CommandLine $executable
    
    Write-Host "`nSimulation completed!" -ForegroundColor Green
    return $true
}

# Main script execution
try {
    $currentDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $currentUser = $env:USERNAME
    
    # Welcome message
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "=== XINU CIS657 Final Project Simulation ===" -ForegroundColor Cyan
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "Current Date and Time (UTC): $currentDate" -ForegroundColor White
    Write-Host "Current User's Login: $currentUser" -ForegroundColor White
    Write-Host "====================================================" -ForegroundColor Cyan
    Write-Host "This script uses your actual XINU source files" -ForegroundColor Green
    
    # Step 1: Create XINU simulation
    $simFiles = Create-XINUSimulation
    if ($simFiles -eq $false) {
        exit 1
    }
    
    # Step 2: Build the simulation
    $executable = Build-XINUSimulation -mainPath $simFiles.MainPath -outputDir $simFiles.OutputDir
    if ($executable -eq $false) {
        exit 1
    }
    
    # Step 3: Run the simulation
    $success = Run-XINUSimulation -executable $executable
    if (-not $success) {
        exit 1
    }
    
    # Final message
    Write-Host "`nXINU starvation prevention simulation completed successfully!" -ForegroundColor Green
    Write-Host "This simulation used your actual source files:" -ForegroundColor White
    Write-Host "  - p1_process.c" -ForegroundColor White
    Write-Host "  - p2_process.c" -ForegroundColor White
    Write-Host "  - pstarv_process.c" -ForegroundColor White
    Write-Host "  - starvation_shell.c" -ForegroundColor White
    Write-Host "  - pstarv_globals.c" -ForegroundColor White
    Write-Host "`nYou can modify these files and run the simulation again to" -ForegroundColor White
    Write-Host "test your changes before implementing them in XINU." -ForegroundColor White
    Write-Host "`nTo run the simulation again:" -ForegroundColor Cyan
    Write-Host "$executable" -ForegroundColor White
}
catch {
    Write-Host "An error occurred:" -ForegroundColor Red
    Write-Host $_ -ForegroundColor Red
    exit 1
}