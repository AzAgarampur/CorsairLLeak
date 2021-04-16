# CorsairLLeak
Map physical addresses into userspace (RW), read/write MSRs, send/recieve data on I/O ports, and query/set bus configuration data with the Corsair LL Access driver.
## How it works
The LL access driver has almost no security permissions. It calls `IoCreateDevice()` and `IoCreateSymbolicLink()` with all default parameters	, therefore granting everyone all access to the device object. However, `IRP_MJ_CREATE` is handled and the driver calls `SeQueryInformationToken` to verify that the caller's integrity level is at least High. Otherwise it returns `STATUS_ACCESS_DENIED`.

Once a valid handle has been opened to the device, usermode code can communicate with it via `DeviceIoControl()` using standard buffered I/O.

Many of the IOCTLs that can be sent to the device simply check the input and output buffer sizes then directly call the corresponding kernel function. This basically lets usermode code execute a list of kernelmode functions/operations and get their results. Some of these functions are:
- `MmMapIoSpace`
- `MmProbeAndLockPagesSpecifyCache`
- `HalSetBusDataByOffset`
- `__writemsr`
- `__out[byte][[d]word]`

So basically you can abuse the heck out of this. (Why is this WHCP signed?)
## The code
The one C file (`main.c`) is the source of a sample program that will attempt to open the device, map ten consecutive physical memory ranges starting from 0, each 4KB in size, and then read the kernel GS base MSR register.

Install the driver for your system (x86/x64) using the command `sc create CORSAIREXPLOIT binPath= ... type= kernel.`

The documented IOCTLs are the ones I've figured out how to use. I've not included ones for I/O `in/out` operations or bus data operations.
## Other notes
1. `__writemsr` seems to be surrounded by SEH. Attempting to change kernel GS base results in `DeviceIoControl()` to return `STATUS_PRIVILEGED_INSTRUCTION`.
2. The IOCTL to free mapped physical address is kinda buggy. It'll randomly double free, and cause a BSOD with code `PFN_LIST_CORRUPT`. It happens the most when trying to free physical addresses near `0x1A0000`.
3. I don't know what the output buffer and its size are when using the IOCTL to write to MSRs. Pretty sure there is no output buffer. Output size seems to be hardcoded to be set to `8`.
4. Additional comments/notes are in the sample code.
