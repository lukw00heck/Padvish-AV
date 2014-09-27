### Killing Padvish AV 
  
  
##### 1 : Race-condition flaw in hooked ZwOpenProcess/ZwOpenThread


*UPDATE : The race-condition flaw is patched in the new version of Padvish EPS. I found and tested it
         on Padvish EPS ver 1.2.38.1083, but seems somehow they found it (just one week after I found it,
		 and the only change is in this functions! are they stealing something from my VM ? ;) 
		 upgrade to 1.4.31.1143 version and you are safe from race-condition flaw!  
		 - Shahriyar, 5/29/2014*

Padvish's driver apsp.sys hooks ZwOpenProcess and ZwOpenThread to protects its process, there is 
race-condition flaw with both of this hook functions. for the sake of simplicity I only describe ZwOpenProcess.
so lets examine the HookedZwOpenProcess :

        .text:00012EF9                 mov     [ebp+ms_exc.disabled], edi
        .text:00012EFC                 push    1               ; Alignment
        .text:00012EFE                 push    4               ; Length
        .text:00012F00                 mov     esi, [ebp+ProcessHandle]
        .text:00012F03                 push    esi             ; Address
        .text:00012F04                 call    ds:ProbeForWrite
        .text:00012F0A                 mov     ebx, [esi]
        .text:00012F0C                 push    [ebp+ClientId]  ; _DWORD
        .text:00012F0F                 push    [ebp+ObjectAttributes] ; _DWORD
        .text:00012F12                 push    [ebp+DesiredAccess] ; _DWORD
        .text:00012F15                 push    esi             ; _DWORD
        .text:00012F16                 call    OriginalZwOpenProcess

It starts by calling the original ZwOpenProcess and passing all the user-provided arguments unmodified.
this call is done for obtaining a handle to the target process. then we have :

        .text:00012F1C                 cmp     eax, edi
        .text:00012F1E                 jl      short loc_12F91
        .text:00012F20                 push    edi             ; HandleInformation
        .text:00012F21                 lea     eax, [ebp+Object]
        .text:00012F24                 push    eax             ; Object
        .text:00012F25                 push    edi             ; AccessMode
        .text:00012F26                 mov     eax, ds:PsProcessType
        .text:00012F2B                 push    dword ptr [eax] ; ObjectType
        .text:00012F2D                 push    edi             ; DesiredAccess
        .text:00012F2E                 push    dword ptr [esi] ; Handle
        .text:00012F30                 call    ds:ObReferenceObjectByHandle
        .text:00012F36                 cmp     eax, edi
        .text:00012F38                 jl      short loc_12F89
        .text:00012F3A                 push    [ebp+Object]
        .text:00012F3D                 call    ds:PsGetProcessId
        .text:00012F43                 mov     edi, eax
        .text:00012F45                 call    ds:PsGetCurrentProcessId
        .text:00012F4B                 test    edi, edi
        .text:00012F4D                 jz      short loc_12F80
        .text:00012F4F                 push    eax             +---+
        .text:00012F50                 call    sub_11E82           |
        .text:00012F55                 mov     [ebp+>ar_20], eax   |
        .text:00012F58                 push    edi                 |
        .text:00012F59                 call    sub_11E82           |
        .text:00012F5E                 mov     edi, eax            |
        .text:00012F60                 push    edi                 |     Validation process
        .text:00012F61                 call    sub_12936           |
        .text:00012F66                 cmp     eax, 0FFFFFFFFh     |
        .text:00012F69                 jz      short loc_12F80     |
        .text:00012F6B                 push    [ebp+>ar_20]        |
        .text:00012F6E                 push    edi                 |
        .text:00012F6F                 call    sub_12986       +---+
        .text:00012F74                 cmp     eax, 0FFFFFFFFh
        .text:00012F77                 jnz     short loc_12F80

The above code gets a handle to the target process's EPROCCESS structure and then passes it to the PsGetProcessId
for getting the target process's ID, then it compares and validate it against some pre-filed list by calling 
the sub_11E82 , sub_12936 and sub_12986. if the validation process fails, function will changes the DesiredAccess
argument to PROCESS_QUERY_INFORMATION (0x400) :

        .text:00012F79                 mov     [ebp+DesiredAccess], 400h

then it close the handle obtained by calling the OriginalZwOpenProcess and then return by calling the 
OriginalZwOpenProcess again but with modified DesiredAccess (in case of failed validation ):

        .text:00012F80                 mov     ecx, [ebp+Object] ; Object
        .text:00012F83                 call    ds:ObfDereferenceObject
        .text:00012F89 loc_12F89:                              ; CODE XREF: HookedZwOpenProcess+5Aj
        .text:00012F89                 push    dword ptr [esi] ; Handle
        .text:00012F8B                 call    ds:ZwClose
        .text:00012F91 loc_12F91:                              ; CODE XREF: HookedZwOpenProcess+40j
        .text:00012F91                 mov     [esi], ebx
        .text:00012F93                 mov     [ebp+ms_exc.disabled], 0FFFFFFFEh
        .text:00012F9A                 jmp     short loc_12FAD
        .text:00012F9C loc_12F9C:                              ; DATA XREF: .rdata:stru_15258o
        .text:00012F9C                 xor     eax, eax        ; Exception filter 0 for function 12EDE
        .text:00012F9E                 inc     eax
        .text:00012F9F                 retn
        .text:00012FA0 loc_12FA0:                              ; DATA XREF: .rdata:stru_15258o
        .text:00012FA0                 mov     esp, [ebp+ms_exc.old_esp] ; Exception handler 0 for function 12EDE
        .text:00012FA3                 mov     [ebp+ms_exc.disabled], 0FFFFFFFEh
        .text:00012FAA loc_12FAA:                              ; CODE XREF: HookedZwOpenProcess+13j
        .text:00012FAA                 mov     esi, [ebp+ProcessHandle]
        .text:00012FAD                 push    [ebp+ClientId]  ; _DWORD
        .text:00012FB0                 push    [ebp+ObjectAttributes] ; _DWORD
        .text:00012FB3                 push    [ebp+DesiredAccess] ; _DWORD
        .text:00012FB6                 push    esi             ; _DWORD
        .text:00012FB7                 call    OrginalZwOpenProcess
        .text:00012FBD                 call    __SEH_epilog4
        .text:00012FC2                 retn    10h

    
The race-condition occur because apsp.sys first calls the original ZwOpenPrcess by passing the user-mode 
ProcessHandle address with unmodified DesiredAccess, then its starts to validate this action. I a normal
situation we can obtain a limited handle to any process ( even protected one ) by calling the ZwOpenProcess, 
but we don't have the handle value before ZwOpenProcess finished its works and returned.
But in this situation the HookedZwOpenProcess calls original ZwOpenProcess two times, first time with unmodified 
arguments for getting EPROCESS Object and respectfully target's process ID and second time after the validation
process has finished (with modified DesiredAccess in case of validation failure).

So we can call HookedZwOpenProcess (by calling OpenProcess) and passing the PROCESS_ALL_ACCESS as DesiredAccess
argument, this will cause the HookedZwOpenProcess call the original ZwOpenProcess for the first time without
modifing any argument (for getting a valid handle to the target process). now we have a valid
handle with PROCESS_ALL_ACCESS privilege, but for using it we most intrupt the HookedZwOpenProcess thread 
execution and force the kernel to schedule another thread which uses this handle for terminating the process.

For doing this I created two separated threads, one which infinitely loops for opening a PROCESS_ALL_ACCESS 
handle to the target process and one which infinitely loops for terminating the process by returned handle.
for making the scenario more possible we can set priority class of the OpenProcess threads to the 
BELOW_NORMAL_PRIORITY_CLASS and TerminateProcess threads to normal or above normal.

*Shahriyar, 5/10/2014* 

##### 2 : NtRenameKey flaw
There is another flaw in dealing with "Image File Execution Options" registry key. Padvish protects this key 
and wont let us set any value inside a key named with it's process names ( its service's process name particularly ).
so its possible to create a key like "Image File Execution Options\APCcSvc.exe", but its not possible to set any value 
inside this key. we can overcome this limitation by creating a random key in "Image File Execution Options"
and setting desired values inside ( e.g "debugger" ) and then renaming it with NtRenameKey to "APCcSvc.exe". 
this wont let Podvish service starts again after termination or reboot.

*Shahriyar, 9/27/2014*
