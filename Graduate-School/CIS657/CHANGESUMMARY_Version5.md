# CIS657 Xinu Midterm Project – Change Summary

## Overview

This document summarizes all significant changes and improvements made to the Xinu project in the `Graduate-School/CIS657` folder to meet the CIS657 Midterm exam requirements.

---

## 1. Shell Commands Added

- **Created new shell commands** in `shell/`:
  - `xsh_create.c` – Create a new process at a user-specified priority; runs infinite loop after announcing PID.
  - `xsh_createsleep.c` – Like `create`, but new process sleeps for 10 seconds before announcing PID.
  - `xsh_psready.c` – Lists only ready processes (filtered version of `ps`).
  - `xsh_wait.c` – Creates a process that waits on a global semaphore after announcing PID.
  - `xsh_signaln.c` – Signals a semaphore N times, allowing multiple waiting processes to proceed.
  - `xsh_resumen.c` – Resumes multiple processes by PID, with highest priority process running after resumption.

- **Added corresponding entries to `cmdtab`** in `shell/shell.c` for each new shell command.

- **Declared prototypes** for each new shell command in `include/shprototypes.h`.

---

## 2. Helper and System Processes

- Defined all helper process functions (`waiter`, `signaller`, `runforever`, `runafterwait`) in `system/main.c`.
- Optionally, left legacy test code (auto-starting `waiter` and `signaller`) in `main.c` for demonstration, but commented for clarity.
- Improved documentation and structure of `main.c` to:
    - Add detailed comments and error handling.
    - Start the shell interactively.
    - Ensure `main` never returns (OS keeps running).

---

## 3. Kernel and Shell Infrastructure

- **Added global semaphore:**
    - Declared in `system/main.c`: `sid32 globalsemaphore;`
    - Externed in `include/semaphore.h`: `extern sid32 globalsemaphore;`

- **Ensured helper function prototypes** exist in `include/prototypes.h` as needed.

---

## 4. Build System

- **Updated `compile/makefile.c`:**
    - Added all new shell command files and `main.c` to the build.
    - Cleaned up references to legacy or unused files.
    - Ensured robust object file generation and correct include paths.

---

## 5. Documentation and References

- Added this summary file (`CHANGESUMMARY.md`).
- Confirmed assignment PDF (`CIS657 Midterm exam.pdf`) is present for reference.

---

## 6. General Clean-Up

- Checked for and ignored unnecessary files (e.g., swap files `.swp`, object files `.o`) in version control.
- Provided comments and error checking for clarity and grading.

---

## 7. Folder/Repo Structure

- All new and modified files reside in their appropriate subfolders:
    - `shell/` – Shell command implementations and prototypes.
    - `system/` – Helper process implementations and `main.c`.
    - `include/` – All necessary headers (including prototypes and semaphore).
    - `compile/` – Makefile.
    - Root of `CIS657/` – Assignment PDF and this summary.

---

## 8. Result

The project now fully reflects the requirements of the CIS657 Midterm prompt, provides robust shell command functionality, and is ready for grading or further extension.

---