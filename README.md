# LoadLibrary DLL Injector

A straightforward C++ DLL injector that utilizes the `LoadLibrary` API technique. This solution includes the injector itself, along with multiple test DLLs and dummy target applications to demonstrate and verify the injection process.

## Features
* **LoadLibrary Injection:** Classic and reliable DLL injection method using `VirtualAllocEx`, `WriteProcessMemory`, and `CreateRemoteThread`.
* **Test DLLs Included:** Sample DLLs to verify successful injection.
* **Multiple Dummy Apps:** Lightweight target applications to test the injector safely without affecting system processes.