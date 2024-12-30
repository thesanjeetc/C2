## A C2 (Command & Control) Project

This is an exploration of malware development with C2 (Command & Control) infra I developed from scratch as part of my internship project at F-Secure. The implant targets Windows systems and is controlled by a NodeJS control server over websockets.

#### Remote Commands
Remote commands include extract system information, upload files, execute arbitrary shell commands and enable real-time activity monitoring.
\
\
<img src="https://github.com/user-attachments/assets/a93eca3f-0cad-43e9-be6d-c1884b24cbe4" width="600" >


#### Activity Monitoring
This covers keylogging, active window title and clipboard hooking to capture user activity and related context.
\
\
<img src="https://github.com/user-attachments/assets/2dff8ffe-ab7c-49f1-8218-a761c6b357d1" width="600" >


#### Obfuscation Techniques
Obfuscation techniques are used to evade basic detection and involve stack strings, XOR encryption and dynamic DLL loading at runtime. This makes the implant more resilient to static analysis and signature-based detection. Modifications are made pre-compilation.
\
\
<img src="https://github.com/user-attachments/assets/d5644841-4036-4e10-b696-e5ff3649b621" width="600" >


#### Persistence
A simple persistence mechanism involves adding a registry key to run the implant on startup.
