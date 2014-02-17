/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios.c
 * PURPOSE:         VDM BIOS Support Library
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "emulator.h"
#include "bios.h"

/* PRIVATE VARIABLES **********************************************************/

static BOOLEAN Bios32Loaded = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

BOOLEAN BiosInitialize(IN LPCWSTR BiosFileName,
                       IN HANDLE  ConsoleInput,
                       IN HANDLE  ConsoleOutput)
{
    if (BiosFileName)
    {
        BOOL   Success;
        HANDLE hBiosFile;
        DWORD  BiosSize;

        /* Open the BIOS file */
        SetLastError(0); // For debugging purposes
        hBiosFile = CreateFileW(BiosFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
        DPRINT1("BIOS opening %s ; GetLastError() = %u\n", hBiosFile != INVALID_HANDLE_VALUE ? "succeeded" : "failed", GetLastError());

        /* If we failed, bail out */
        if (hBiosFile == INVALID_HANDLE_VALUE) return FALSE;

        /* OK, we have a handle to the BIOS file */

        /*
         * Retrieve the size of the file. Since the size of the BIOS file
         * should be at most 64kB, we just use GetFileSize.
         */
        BiosSize = GetFileSize(hBiosFile, NULL);
        if (BiosSize == INVALID_FILE_SIZE && GetLastError() != ERROR_SUCCESS)
        {
            /* We failed, return failure */

            /* Close the BIOS file */
            CloseHandle(hBiosFile);

            return FALSE;
        }

        DisplayMessage(L"First bytes at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
                       L"3 last bytes at 0x%p: 0x%02x 0x%02x 0x%02x",
            (PVOID)((ULONG_PTR)TO_LINEAR(0xF000, 0xFFFF) + 1 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 1 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 2 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 3 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 4 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 5 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 6 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 7 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 8 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 9 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 0xa - BiosSize),

            (PVOID)((ULONG_PTR)TO_LINEAR(0xF000, 0xFFFF) - 2),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) - 2),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) - 1),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) - 0));

        /* Attempt to load the BIOS file into memory */
        SetLastError(0); // For debugging purposes
        Success = ReadFile(hBiosFile,
                           (PVOID)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 1 - BiosSize),
                           /* (PVOID)((ULONG_PTR)BaseAddress + ROM_AREA_END + 1 - BiosSize) */
                           BiosSize,
                           &BiosSize,
                           NULL);
        DPRINT1("BIOS loading %s ; GetLastError() = %u\n", Success ? "succeeded" : "failed", GetLastError());

        /* Close the BIOS file */
        CloseHandle(hBiosFile);

        DisplayMessage(L"First bytes at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
                       L"3 last bytes at 0x%p: 0x%02x 0x%02x 0x%02x",
            (PVOID)((ULONG_PTR)TO_LINEAR(0xF000, 0xFFFF) + 1 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 1 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 2 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 3 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 4 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 5 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 6 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 7 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 8 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 9 - BiosSize),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) + 0xa - BiosSize),

            (PVOID)((ULONG_PTR)TO_LINEAR(0xF000, 0xFFFF) - 2),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) - 2),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) - 1),
           *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(0xF000, 0xFFFF) - 0));

        DisplayMessage(L"POST at 0x%p: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                       TO_LINEAR(getCS(), getIP()),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 0),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 1),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 2),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 3),
                       *(PCHAR)((ULONG_PTR)SEG_OFF_TO_PTR(getCS(), getIP()) + 4));

        /* Boot it up */

        /*
         * The CPU is already in reset-mode so that
         * CS:IP points to F000:FFF0 as required.
         */
        DisplayMessage(L"CS=0x%p ; IP=0x%p", getCS(), getIP());
        // setCS(0xF000);
        // setIP(0xFFF0);

        return TRUE;
    }
    else
    {
        Bios32Loaded = Bios32Initialize(ConsoleInput, ConsoleOutput);
        return Bios32Loaded;
    }
}

VOID BiosCleanup(VOID)
{
    if (Bios32Loaded) Bios32Cleanup();
}

/* EOF */
