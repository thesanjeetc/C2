## A C2 (Command & Control) Project

This is an exploration of malware development with C2 (Command & Control) infra I developed from scratch as part of my internship project at F-Secure. The implant targets Windows systems and is controlled by a NodeJS control server over websockets.

#### Remote Commands

Remote commands include extract system information, upload files, execute arbitrary shell commands and enable real-time activity monitoring. This covers keylogging, active window title and clipboard hooking to capture user activity and related context.

#### Obfuscation Techniques

Obfuscation techniques are used to evade basic detection and involve stack strings, XOR encryption and dynamic DLL loading at runtime. This makes the implant more resilient to static analysis and signature-based detection. Modifications are made pre-compilation.

#### Persistence

A simple persistence mechanism involves adding a registry key to run the implant on startup.
