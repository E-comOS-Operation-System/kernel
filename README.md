E-comOS Kernel 
===============================================
This is the core kernel of E-comOS, built for "no Unix legacy, clean microkernel" design.

License: AGPLv3  
Key rule (no need to read the whole license): If you modify this kernel or make derivative works (like your own OS based on it), you MUST open source your code under AGPLv3 (or a later version).  
For more details: Check the [Copyright Page in our Wiki](https://github.com/e-comos/wiki/blob/main/Copyright.md) (direct link to save your time).

===============================================
How to Make a Runable Image / Build Your Own Distro  
-----------------------------------------------
We made two commands to cover your needs — pick the one that fits:

1. For testing the kernel quickly: `make image`  
   - Output: A basic image file named `canuse.img`  
   - What it does: Just the kernel + minimal bootloader (enough to run in QEMU/old laptops like ThinkPad X260 for testing).

2. For making your own E-comOS distro: `make fuckimage`  
   - Output: A complete folder (named `distro-base/`) with extra system components:  
     User-space services (VGA display, IPC helper, basic shell)  
     Bootable config (supports USB/CD burning)  
     Empty template for adding your own tools (e.g., your QR code scanner)  
   - How to use it: Modify the files in `distro-base/` (e.g., add your app, tweak the shell), then package it into an ISO/USB — this is exactly how we build official E-comOS releases!
