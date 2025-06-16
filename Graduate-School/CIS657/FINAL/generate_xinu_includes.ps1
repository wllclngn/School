$projectDir = $PSScriptRoot
$includesFile = Join-Path -Path $projectDir -ChildPath "xinu.h"
$xinu_init_file = Join-Path -Path $projectDir -ChildPath "xinu_init.c"
$makefilePath = Join-Path -Path $projectDir -ChildPath "compile" -ChildPath "Makefile"
$file_list_json = Join-Path -Path $projectDir -ChildPath "file_list.json"

Write-Host "Generating XINU includes and initialization code..." -ForegroundColor Cyan

if (-not (Test-Path $makefilePath)) {
    Write-Host "ERROR: Makefile not found at $makefilePath" -ForegroundColor Red
    exit 1
}

# Parse the Makefile to extract source files
$makefileContent = Get-Content -Path $makefilePath -Raw

# Extract components and source files from Makefile
$componentsMatch = [regex]::Match($makefileContent, 'COMPS\s*=\s*([^#]+)')
$components = @()
if ($componentsMatch.Success) {
    $componentsText = $componentsMatch.Groups[1].Value
    $components = $componentsText -split '\s+' | Where-Object { $_ -ne "" -and $_ -ne "\\" }
    Write-Host "Found components: $($components -join ', ')" -ForegroundColor Yellow
} else {
    Write-Host "No components found in Makefile" -ForegroundColor Red
    exit 1
}

# Initialize a collection for all source files
$sourceFiles = @()

# Process each component to extract its source files
foreach ($component in $components) {
    $componentUpper = $component.ToUpper() -replace '[\/]', '_'
    
    # Extract C files
    $cfilesMatch = [regex]::Match($makefileContent, "${componentUpper}_CFILES\s*=\s*([^#]+?)\s*(?=${componentUpper}_[CS]FILES|\s*SYSTEM_)")
    if ($cfilesMatch.Success) {
        $cfilesText = $cfilesMatch.Groups[1].Value
        $cfiles = $cfilesText -split '\s+\\?\s*\n\s*' | ForEach-Object {
            $_.Trim() -replace '\\$', '' -split '\s+' | Where-Object { $_ -ne "" }
        }
        $cfiles = $cfiles | Where-Object { $_ -match '\S' }
        
        # Add component path to each file
        foreach ($file in $cfiles) {
            $sourceFiles += "$component/$file"
        }
    }
    
    # Extract S files (assembly)
    $sfilesMatch = [regex]::Match($makefileContent, "${componentUpper}_SFILES\s*=\s*([^#]+?)\s*(?=${componentUpper}_[CS]FILES|\s*SYSTEM_)")
    if ($sfilesMatch.Success) {
        $sfilesText = $sfilesMatch.Groups[1].Value
        $sfiles = $sfilesText -split '\s+\\?\s*\n\s*' | ForEach-Object {
            $_.Trim() -replace '\\$', '' -split '\s+' | Where-Object { $_ -ne "" }
        }
        $sfiles = $sfiles | Where-Object { $_ -match '\S' }
        
        # Add component path to each file
        foreach ($file in $sfiles) {
            $sourceFiles += "$component/$file"
        }
    }
}

# Find all header files in the include directory
$includeDir = Join-Path -Path $projectDir -ChildPath "include"
$headerFiles = @()
if (Test-Path $includeDir) {
    $headerFiles = Get-ChildItem -Path $includeDir -Filter "*.h" | ForEach-Object { "include/$($_.Name)" }
    Write-Host "Found $($headerFiles.Count) header files in include directory" -ForegroundColor Yellow
}

# Generate file_list.json
$fileListData = @{
    "files" = @()
}

# Add source files
foreach ($sourceFile in $sourceFiles) {
    $fileListData.files += @{
        "path" = $sourceFile
        "type" = "source"
    }
}

# Add header files
foreach ($headerFile in $headerFiles) {
    $fileListData.files += @{
        "path" = $headerFile
        "type" = "header"
    }
}

# Add xinu_simulation.c
$fileListData.files += @{
    "path" = "xinu_simulation.c"
    "type" = "source"
}

# Add xinu_init.c
$fileListData.files += @{
    "path" = "xinu_init.c"
    "type" = "source"
}

# Write file_list.json
$fileListData | ConvertTo-Json -Depth 4 | Out-File -FilePath $file_list_json -Encoding UTF8
Write-Host "Generated $file_list_json with $($fileListData.files.Count) files" -ForegroundColor Green

# Create the xinu.h file that includes all headers
$xinuHeaderContent = @"
#ifndef _XINU_H_
#define _XINU_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

/* Type definitions */
typedef int             int32;
typedef short           int16;
typedef char            int8;
typedef unsigned int    uint32;
typedef unsigned short  uint16;
typedef unsigned char   uint8;
typedef char            bool8;
typedef int             pid32;
typedef int             sid32;
typedef int             qid16;
typedef int             pri16;
typedef int             umsg32;
typedef int             syscall;
typedef int             devcall;
typedef int             shellcmd;
typedef int             did32;
typedef int             ibid32;
typedef int             status;

/* Constants */
#define OK              1
#define SYSERR          (-1)
#define EOF             (-2)
#define TIMEOUT         (-3)
#define FALSE           0
#define TRUE            1
#define EMPTY           (-1)
#define NULL            0
#define NULLCH          '\0'
#define NULLSTR         ""
#define MAXPRIO         100
#define MINPRIO         1
#define NPROC           64
#define NQENT           500
#define BADPID          (-1)
#define SHELL_ERROR     1
#define SHELL_OK        0

/* Process states */
#define PR_FREE         0
#define PR_CURR         1
#define PR_READY        2
#define PR_RECV         3
#define PR_SLEEP        4
#define PR_SUSP         5
#define PR_WAIT         6
#define PR_RECTIM       7
"@

# Add includes for all header files
foreach ($headerFile in $headerFiles) {
    $fileName = [System.IO.Path]::GetFileName($headerFile)
    $xinuHeaderContent += "`n#include <$fileName>"
}

$xinuHeaderContent += @"

#endif
"@

# Write the xinu.h file
$xinuHeaderContent | Out-File -FilePath $includesFile -Encoding ASCII
Write-Host "Generated $includesFile with all header includes" -ForegroundColor Green

# Create the xinu_init.c file with key system initialization functions
$xinuInitContent = @"
#include <xinu.h>

/* System data structures */
struct procent proctab[NPROC];
qid16 readylist;
pid32 currpid;
uint32 clktime;
uint32 clkticks;

/* Process state strings for debugging */
const char *states[] = {
    "FREE", "CURR", "READY", "RECV",
    "SLEEP", "SUSP", "WAIT", "RECTIM"
};

/* Starvation prevention variables */
struct defer Defer;
bool8 enable_starvation_fix = FALSE;
pid32 pstarv_pid = BADPID;
uint32 pstarv_ready_time = 0;
uint32 last_boost_time = 0;

/* Queue entry structure */
struct qentry {
    pid32 qnext;
    pid32 qprev;
    pri16 qkey;
};

/* Queue table */
struct qentry queuetab[NQENT];

/* Function prototypes */
int32 firstid(qid16 q);
int32 firstkey(qid16 q);
pid32 getitem(pid32 pid);
pid32 enqueue(pid32 pid, qid16 q);
pid32 dequeue(qid16 q);
qid16 newqueue(void);

/* System initialization function */
void initialize_system(void) {
    int i;
    
    /* Initialize process table */
    for (i = 0; i < NPROC; i++) {
        proctab[i].prstate = PR_FREE;
        proctab[i].prprio = 0;
        strncpy(proctab[i].prname, "unused", 8);
        proctab[i].prpid = i;
    }
    
    /* Initialize process 0 as current */
    currpid = 0;
    proctab[0].prstate = PR_CURR;
    strncpy(proctab[0].prname, "prnull", 8);
    proctab[0].prprio = 0;
    
    /* Initialize ready list */
    readylist = newqueue();
    
    /* Initialize clocks */
    clktime = 0;
    clkticks = 0;
    
    /* Initialize deferrals */
    Defer.ndefers = 0;
    Defer.attempt = FALSE;
    
    /* Initialize starvation prevention variables */
    pstarv_pid = BADPID;
    enable_starvation_fix = FALSE;
    pstarv_ready_time = 0;
    last_boost_time = 0;
    
    kprintf("System initialized at time %d\n", clktime);
}

/* Create a new queue */
qid16 newqueue(void) {
    static qid16 nextqid = NPROC;
    qid16 q;
    
    q = nextqid++;
    queuetab[q].qnext = EMPTY;
    queuetab[q].qprev = EMPTY;
    return q;
}

/* Insert a process in a queue */
pid32 enqueue(pid32 pid, qid16 q) {
    int tail;
    
    if (pid < 0 || pid >= NPROC || q < 0 || q >= NQENT) {
        return SYSERR;
    }
    
    /* If queue is empty, add process as the only entry */
    if (queuetab[q].qnext == EMPTY) {
        queuetab[q].qnext = pid;
        queuetab[q].qprev = pid;
        queuetab[pid].qnext = EMPTY;
        queuetab[pid].qprev = EMPTY;
        return OK;
    }
    
    /* Add process at the tail of the queue */
    tail = queuetab[q].qprev;
    queuetab[pid].qprev = tail;
    queuetab[pid].qnext = EMPTY;
    queuetab[tail].qnext = pid;
    queuetab[q].qprev = pid;
    
    return OK;
}

/* Remove and return the first process from a queue */
pid32 dequeue(qid16 q) {
    pid32 pid;
    
    if (q < 0 || q >= NQENT || queuetab[q].qnext == EMPTY) {
        return SYSERR;
    }
    
    pid = queuetab[q].qnext;
    
    /* Only one process on the queue */
    if (queuetab[pid].qnext == EMPTY) {
        queuetab[q].qnext = EMPTY;
        queuetab[q].qprev = EMPTY;
    } else {
        queuetab[q].qnext = queuetab[pid].qnext;
        queuetab[queuetab[pid].qnext].qprev = EMPTY;
    }
    
    queuetab[pid].qnext = EMPTY;
    queuetab[pid].qprev = EMPTY;
    return pid;
}

/* Get ID of first process in a queue */
int32 firstid(qid16 q) {
    if (q < 0 || q >= NQENT || queuetab[q].qnext == EMPTY) {
        return EMPTY;
    }
    return queuetab[q].qnext;
}

/* Get key (priority) of first process in a queue */
int32 firstkey(qid16 q) {
    pid32 pid;
    
    if (q < 0 || q >= NQENT || queuetab[q].qnext == EMPTY) {
        return SYSERR;
    }
    
    pid = queuetab[q].qnext;
    return queuetab[pid].qkey;
}

/* Remove a specific item from a queue */
pid32 getitem(pid32 pid) {
    pid32 prev, next;
    
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    prev = queuetab[pid].qprev;
    next = queuetab[pid].qnext;
    
    if (prev == EMPTY) {
        return SYSERR;
    }
    
    if (next == EMPTY) {
        queuetab[prev].qnext = EMPTY;
    } else {
        queuetab[prev].qnext = next;
        queuetab[next].qprev = prev;
    }
    
    queuetab[pid].qnext = EMPTY;
    queuetab[pid].qprev = EMPTY;
    
    return pid;
}

/* Insert a process in order by key */
void insert(pid32 pid, int head, int key) {
    pid32 curr, prev;
    
    if (pid < 0 || pid >= NPROC) {
        return;
    }
    
    curr = queuetab[head].qnext;
    prev = head;
    
    queuetab[pid].qkey = key;
    
    /* Find insertion point */
    while (curr != EMPTY && queuetab[curr].qkey >= key) {
        prev = curr;
        curr = queuetab[curr].qnext;
    }
    
    /* Insert process between prev and curr */
    queuetab[pid].qnext = curr;
    queuetab[pid].qprev = prev;
    queuetab[prev].qnext = pid;
    
    if (curr != EMPTY) {
        queuetab[curr].qprev = pid;
    } else {
        /* New process is now the tail */
        queuetab[head].qprev = pid;
    }
}

/* Simulated context switch */
void ctxsw(void **old_sp, void **new_sp) {
    kprintf("Context switch: %p -> %p\n", old_sp, new_sp);
}

/* Print formatted output */
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

/* Get process ID */
pid32 getpid(void) {
    return currpid;
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
        insert(pid, readylist, newprio);
    }
    
    return oldprio;
}

/* Kill a process */
syscall kill(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    proctab[pid].prstate = PR_FREE;
    return OK;
}

/* Make a process ready */
syscall ready(pid32 pid) {
    if (pid < 0 || pid >= NPROC) {
        return SYSERR;
    }
    
    proctab[pid].prstate = PR_READY;
    insert(pid, readylist, proctab[pid].prprio);
    
    return OK;
}

/* Resume a suspended process */
syscall resume(pid32 pid) {
    if (pid < 0 || pid >= NPROC || proctab[pid].prstate != PR_SUSP) {
        return SYSERR;
    }
    
    return ready(pid);
}

/* Create a process */
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
    strncpy(proctab[pid].prname, name, 15);
    proctab[pid].prname[15] = '\0';
    proctab[pid].prpid = pid;
    proctab[pid].prprio = priority;
    proctab[pid].prstate = PR_SUSP;
    
    kprintf("Created process '%s' with PID %d and priority %d\n", name, pid, priority);
    
    return pid;
}

/* Yield processor to another process */
syscall yield(void) {
    pid32 next_pid;
    
    /* Check if there are any processes to run */
    if (queuetab[readylist].qnext == EMPTY) {
        return OK;
    }
    
    /* Remove highest priority process from ready list */
    next_pid = dequeue(readylist);
    if (next_pid == SYSERR) {
        return SYSERR;
    }
    
    /* Save current process state and switch to new process */
    proctab[currpid].prstate = PR_READY;
    insert(currpid, readylist, proctab[currpid].prprio);
    
    proctab[next_pid].prstate = PR_CURR;
    pid32 old_pid = currpid;
    currpid = next_pid;
    
    kprintf("*** CONTEXT SWITCH: From process %d (%s) to %d (%s) ***\n", 
        old_pid, proctab[old_pid].prname, currpid, proctab[currpid].prname);
        
    return OK;
}

/* Sleep for a specified time */
syscall sleep(uint32 delay) {
    Sleep(delay * 1000); // Windows Sleep function (milliseconds)
    return OK;
}
"@

# Write the xinu_init.c file
$xinuInitContent | Out-File -FilePath $xinu_init_file -Encoding ASCII
Write-Host "Generated $xinu_init_file with system initialization code" -ForegroundColor Green

Write-Host "XINU includes and initialization code generation completed successfully!" -ForegroundColor Green