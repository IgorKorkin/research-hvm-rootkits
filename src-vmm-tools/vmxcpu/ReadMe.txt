VMX
 CPU0	- Driver to put processor 0 into VMX mode.
 CPU1	- Driver to put processor 1 into VMX mode.

CPUID 	- Utility to execute CPUID on both processors.
	- You can run this anytime as it is normal instruction.

VMXOFF	- Utility to execute VMXOFF on both processors.
	- Do not run this unless both CPU(s) are in VMX mode.

   ***	The 2 utilities CPUID & VMXOFF set the affinity to one processor and then the other.
	If you do not have at least 2 processors, do not run them.