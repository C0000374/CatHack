//++
//
// Copyright (C) C0000374
// 
// Module:
//     MAIN.C
// 
// Abstract:
//     This module implements a build system for Java projects.
// 
// Revision History:
//     27/08/26 C0000374 Copy it from the old CHC source code,
//                       remove ARGSCL.BLD and ARGSJAVAC.BLD files.
//
//--

#include <Windows.h>
#include <ShLWAPI.h>
#include <INTRIN.H>

#pragma warning(push)
#pragma warning(disable:4005)
#define CDECL	__cdecl
#define STDCALL __stdcall
#pragma warning(pop)

typedef struct $$BLD$STRLIST_ENTRY BLD$STRLIST_ENTRY, * PBLD$STRLIST_ENTRY;
struct $$BLD$STRLIST_ENTRY {
    PBLD$STRLIST_ENTRY Next;
    PBLD$STRLIST_ENTRY Previous;
    PWSTR String;
};

typedef struct $$BLD$STRLIST BLD$STRLIST, * PBLD$STRLIST;
struct $$BLD$STRLIST {
    PBLD$STRLIST_ENTRY ListHead;
};

HANDLE BLD$ConsoleOutputHandle;

//++
//
// Helper functions.
//
//--

FORCEINLINE
VOID
BLD$WriteConsole(
    IN PCWSTR String
    )
{
    UINT32 NumberOfCharsWritten;

    WriteConsoleW(
        BLD$ConsoleOutputHandle,
        String,
        lstrlenW(String),
        &NumberOfCharsWritten,
        NULL
    );
    return;
}

PVOID
STDCALL
BLD$AllocateMemory(
    IN UINT32 Size
    )
{
    PVOID Block;

    Block = LocalAlloc(LPTR, Size);
    if (Block == NULL) {

        BLD$WriteConsole(L"%BUILD-F-NOMEM, Can't allocate a memory!");
        ExitProcess(1);
    }

    return Block;
}

PWSTR
STDCALL
BLD$DuplicateString(
    IN PCWSTR SourceString
    )
{
    PWSTR Result;

    Result = StrDupW(SourceString);
    if (Result == NULL) {

        BLD$WriteConsole(L"%BUILD-F-NOMEM, Can't allocate a memory!");
        ExitProcess(1);
    }

    return Result;
}

VOID
STDCALL
BLD$CreateList(
    IN PBLD$STRLIST StrList
    )
{
    StrList->ListHead = NULL;
    return;
}

VOID
STDCALL
BLD$DestroyList(
    IN PBLD$STRLIST StringList
    )
{
    PBLD$STRLIST_ENTRY CurrentEntry;
    PBLD$STRLIST_ENTRY NextEntry;

    if (StringList->ListHead != NULL) {

        CurrentEntry = StringList->ListHead;
        do {
            NextEntry = CurrentEntry->Next;
            LocalFree(CurrentEntry->String);
            LocalFree(CurrentEntry);
            CurrentEntry = NextEntry;
        } while (CurrentEntry != StringList->ListHead);
    }
    return;
}

VOID
STDCALL
BLD$AddStringToList(
    IN PBLD$STRLIST StrList,
    IN PCWSTR String
    )
{
    PBLD$STRLIST_ENTRY StringListEntry;

    if (StrList->ListHead == NULL) {

        StrList->ListHead = BLD$AllocateMemory(sizeof(BLD$STRLIST_ENTRY));
        StrList->ListHead->Next = StrList->ListHead;
        StrList->ListHead->Previous = StrList->ListHead;
        StrList->ListHead->String = BLD$DuplicateString(String);
    }
    else {
        StringListEntry = BLD$AllocateMemory(sizeof(BLD$STRLIST_ENTRY));
        StrList->ListHead->Previous->Next = StringListEntry;
        StringListEntry->Previous = StrList->ListHead->Previous;
        StringListEntry->Next = StrList->ListHead;
        StrList->ListHead->Previous = StringListEntry;
        StringListEntry->String = BLD$DuplicateString(String);
    }
    return;
}

BOOLEAN
STDCALL
BLD$ExecuteCommand(
    IN PCWSTR Command,
    IN PUINT32 ExitCode
    )
#define ec$BUFFER_SIZE  2048
{
    STARTUPINFOW StartupInformation;
    PROCESS_INFORMATION ProcessInformation;
    BOOL Status_B32;
    PWSTR Buffer;

    Buffer = BLD$AllocateMemory(ec$BUFFER_SIZE * sizeof(WCHAR));

    __stosb((PUINT8)&StartupInformation, 0, sizeof(STARTUPINFOW));
    StartupInformation.cb = sizeof(STARTUPINFOW);

    ExpandEnvironmentStringsW(
        Command,
        Buffer,
        ec$BUFFER_SIZE
    );

    Status_B32 = CreateProcessW(
        NULL,
        Buffer,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &StartupInformation,
        &ProcessInformation
    );
    if (Status_B32 == FALSE) goto Exit;

    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);

    if (ExitCode != NULL) GetExitCodeProcess(ProcessInformation.hProcess, ExitCode);

    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);
Exit:
    LocalFree(Buffer);
    return (BOOLEAN)Status_B32;
}
#undef ec$BUFFER_SIZE

//++
//
// Main functions.
//
//--

VOID
STDCALL
BLD$FindFiles(
    IN PCWSTR Directory,
    IN PCWSTR Mask,
    IN PBLD$STRLIST Files
    )
//++
//
// Routine Description:
//     This function finds files specified by the file mask.
//
// Parameter:
//     Directory - Specifies a directory for search.
//     Mask - Specifies the file mask.
//     Files - A pointer to a list to receive file names.
//
// Return Value:
//     None.
//
//--
{
    PWSTR FullSearchPath;
    PWSTR FullPath;
    HANDLE hFind;
    WIN32_FIND_DATAW FindData;

    FullSearchPath = BLD$AllocateMemory(MAX_PATH * 2 * sizeof(WCHAR));
    FullPath = FullSearchPath + MAX_PATH;

    wnsprintfW(
        FullSearchPath,
        MAX_PATH,
        L"%s\\%s",
        Directory,
        Mask
    );
    FullSearchPath[MAX_PATH - 1] = 0;

    hFind = FindFirstFileW(FullSearchPath, &FindData);
    if (hFind != INVALID_HANDLE_VALUE) {

        do {
            if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                wnsprintfW(
                    FullPath,
                    MAX_PATH,
                    L"%s\\%s",
                    Directory,
                    FindData.cFileName
                );
                FullPath[MAX_PATH - 1] = 0;

                BLD$AddStringToList(Files, FullPath);
            }
        } while (FindNextFileW(hFind, &FindData));
        FindClose(hFind);
    }

    wnsprintfW(
        FullSearchPath,
        MAX_PATH,
        L"%s\\*",
        Directory
    );
    FullSearchPath[MAX_PATH - 1] = 0;

    hFind = FindFirstFileW(FullSearchPath, &FindData);
    if (hFind != INVALID_HANDLE_VALUE) {

        do {
            if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && lstrcmpW(FindData.cFileName, L".") != 0 && lstrcmpW(FindData.cFileName, L"..") != 0) {

                wnsprintfW(
                    FullPath,
                    MAX_PATH,
                    L"%s\\%s",
                    Directory,
                    FindData.cFileName
                );
                FullPath[MAX_PATH - 1] = 0;

                BLD$FindFiles(FullPath, Mask, Files);
            }
        } while (FindNextFileW(hFind, &FindData));
        FindClose(hFind);
    }

    LocalFree(FullSearchPath);
    return;
}

VOID
STDCALL
BLD$PreprocessFile(
    IN PCWSTR DirectoryName,
    IN PCWSTR FileName,
    IN PCWSTR ClArguments,
    IN PBLD$STRLIST FilesToRemove
    )
//++
//
// Routine Description:
//     This function preprocesses a single .JXX file.
//
// Parameters:
//     DirectoryName - Specifies the file directory.
//     FileName - Specifies the file name.
//     ClArguments - Specifies arguments for the CL.EXE
//     FilesToRemove - A pointer to a list to receive names of files to delete.
// 
// Return Value:
//     None.
//
//--
{
    BOOLEAN Status_B8;
    BOOLEAN Failed;
    UINT32 ExitCode;
    UINT32 OldLength;
    UINT32 ClCommandLineLength;
    PWSTR ClCommandLine;
    PWSTR OldCurrentDirectory;
    PWSTR JavaFileName;
    PWSTR NameForList;

    Failed = FALSE;
    OldCurrentDirectory = BLD$AllocateMemory(MAX_PATH * 3 * sizeof(WCHAR));
    JavaFileName = OldCurrentDirectory + MAX_PATH;
    NameForList = OldCurrentDirectory + MAX_PATH * 2;

    GetCurrentDirectoryW(MAX_PATH, OldCurrentDirectory);
    SetCurrentDirectoryW(DirectoryName);

    lstrcpyW(JavaFileName, FileName);
    OldLength = lstrlenW(JavaFileName);

    //+
    // .jxx
    // .java
    //     ^
    //     |
    //     OldLength
    //-
    JavaFileName[OldLength - 3] = 'j';
    JavaFileName[OldLength - 2] = 'a';
    JavaFileName[OldLength - 1] = 'v';
    JavaFileName[OldLength] = 'a';
    JavaFileName[OldLength + 1] = 0;

    ClCommandLineLength = lstrlenW(FileName) + lstrlenW(JavaFileName) + lstrlenW(ClArguments) + 256;
    ClCommandLine = BLD$AllocateMemory(ClCommandLineLength * sizeof(WCHAR));

    wnsprintfW(
        ClCommandLine,
        ClCommandLineLength,
        L"CL.EXE /EP /P /Fi%s /TC /nologo %s %s",
        JavaFileName,
        ClArguments,
        FileName
    );
    ClCommandLine[ClCommandLineLength - 1] = 0;

    Status_B8 = BLD$ExecuteCommand(ClCommandLine, &ExitCode);
    if (!Status_B8 || ExitCode != 0) {

        BLD$WriteConsole(L"%BUILD-F-PREPROC, Preprocessor error.");
        Failed = TRUE;
        goto Cleanup;
    }

    SetCurrentDirectoryW(OldCurrentDirectory);

    wnsprintfW(
        NameForList,
        MAX_PATH,
        L"%s\\%s",
        DirectoryName,
        JavaFileName
    );
    NameForList[MAX_PATH - 1] = 0;

    BLD$AddStringToList(FilesToRemove, NameForList);

Cleanup:
    LocalFree(OldCurrentDirectory);
    LocalFree(ClCommandLine);
    if (Failed) ExitProcess(1);
    return;
}

VOID
STDCALL
BLD$PreprocessFiles(
    PCWSTR Directory,
    PCWSTR ArgsCl,
    PBLD$STRLIST FilesToRemove
    )
//++
//
// Routine Description:
//     This function finds and preprocesses all .JXX files.
// 
// Parameters:
//     Directory - Specifies a directory for search.
//     ArgsCl - Specifies arguments for the CL.EXE.
//     FilesToRemove - A pointer to a list to receive the names of files to delete.
// 
// Return Value:
//     None.
// 
//--
{
    PWSTR FullSearchPath;
    PWSTR FullPath;
    HANDLE hFind;
    WIN32_FIND_DATAW FindData;

    FullSearchPath = BLD$AllocateMemory(MAX_PATH * 2 * sizeof(WCHAR));
    FullPath = FullSearchPath + MAX_PATH;

    wnsprintfW(
        FullSearchPath,
        MAX_PATH,
        L"%s\\*.jxx",
        Directory
    );
    FullSearchPath[MAX_PATH - 1] = 0;

    hFind = FindFirstFileW(FullSearchPath, &FindData);
    if (hFind != INVALID_HANDLE_VALUE) {

        do {
            if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {

                BLD$PreprocessFile(
                    Directory,
                    FindData.cFileName,
                    ArgsCl,
                    FilesToRemove
                );
            }
        } while (FindNextFileW(hFind, &FindData));
        FindClose(hFind);
    }

    wnsprintfW(
        FullSearchPath,
        MAX_PATH,
        L"%s\\*",
        Directory
    );
    FullSearchPath[MAX_PATH - 1] = 0;

    hFind = FindFirstFileW(FullSearchPath, &FindData);
    if (hFind != INVALID_HANDLE_VALUE) {

        do {
            if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && lstrcmpW(FindData.cFileName, L".") != 0 && lstrcmpW(FindData.cFileName, L"..") != 0) {

                wnsprintfW(
                    FullPath,
                    MAX_PATH,
                    L"%s\\%s",
                    Directory,
                    FindData.cFileName
                );
                FullPath[MAX_PATH - 1] = 0;

                BLD$PreprocessFiles(FullPath, ArgsCl, FilesToRemove);
            }
        } while (FindNextFileW(hFind, &FindData));
        FindClose(hFind);
    }

    LocalFree(FullSearchPath);
    return;
}

VOID
CDECL
mainCRTStartup(
    VOID
    )
//++
//
// Routine Description:
//     This is a main BUILD.EXE subroutine.
//     It invokes CL.EXE to preprocess .JXX files,
//     invokes JAVAC.EXE to compile .JAVA files and
//     deletes preprocessed .JXX files.
//
// Parameters:
//     None.
//
// Return Value:
//     None.
//
//--
{
    BLD$STRLIST FilesToCompile;
    BLD$STRLIST FilesToRemove;
    PWSTR ArgsCl;
    PWSTR ArgsJavaC;
    HANDLE hRecompList;
    UINT32 BufferSize;
    PWSTR Command;
    BOOLEAN Status;
    UINT32 ExitCode;
    PBLD$STRLIST_ENTRY CurrentEntry;
    PWSTR* CommandLine;
    UINT32 ArgumentsCount;
    UINT32 Length;
    UINT32 NumberOfBytesWritten;
    PSTR Buffer;

    ExitCode = 0;

    BLD$ConsoleOutputHandle = CreateFileW(
        L"ConOut$",
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL
    );

    //+
    // Parse a command line.
    //-

    CommandLine = CommandLineToArgvW(GetCommandLineW(), &ArgumentsCount);
    if (ArgumentsCount < 3) {

        BLD$WriteConsole(L"Usage:\r\n\tBUILD.EXE <CL.EXE-Arguments> <JAVAC.EXE-Arguments>");
        ExitProcess(1);
    }

    ArgsCl = CommandLine[1];
    ArgsJavaC = CommandLine[2];

    //+
    // Preprocess all .JXX files.
    //-

    BLD$CreateList(&FilesToRemove);
    BLD$PreprocessFiles(L".", ArgsCl, &FilesToRemove);

    //+
    // Compile all .JAVA files.
    //-

    BLD$CreateList(&FilesToCompile);
    BLD$FindFiles(L".", L"*.java", &FilesToCompile);

    hRecompList = CreateFileW(
        L"BldTmp00.tmp",
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        0,
        NULL
    );
    if (hRecompList == INVALID_HANDLE_VALUE) {

        BLD$WriteConsole(L"%BUILD-F-CANTCREATFIL, Can't create BLDTMP00.TMP.");
        ExitProcess(1);
    }

    CurrentEntry = FilesToCompile.ListHead;
    do {
        //+
        // Make Java compiler happy.
        //-
        Length = lstrlenW(CurrentEntry->String);
        CurrentEntry->String[Length - 4] = 'j';
        CurrentEntry->String[Length - 3] = 'a';
        CurrentEntry->String[Length - 2] = 'v';
        CurrentEntry->String[Length - 1] = 'a';

        //+
        // Write the file name to the file.
        //-
        BufferSize = WideCharToMultiByte(
            CP_UTF8,
            0,
            CurrentEntry->String,
            -1,
            NULL,
            0,
            NULL,
            NULL
        );

        Buffer = BLD$AllocateMemory(BufferSize + 1);

        WideCharToMultiByte(
            CP_UTF8,
            0,
            CurrentEntry->String,
            -1,
            Buffer,
            BufferSize,
            NULL,
            NULL
        );
        Buffer[BufferSize - 1] = '\r';
        Buffer[BufferSize] = '\n';

        WriteFile(
            hRecompList,
            Buffer,
            BufferSize + 1,
            &NumberOfBytesWritten,
            NULL
        );

        LocalFree(Buffer);
        CurrentEntry = CurrentEntry->Next;
    } while (CurrentEntry != FilesToCompile.ListHead);

    CloseHandle(hRecompList);

    BufferSize = lstrlenW(ArgsJavaC) + 256;
    Command = BLD$AllocateMemory(BufferSize * sizeof(WCHAR));
    wnsprintfW(
        Command,
        BufferSize,
        L"%%JavaHome%%\\Bin\\JavaC.exe -encoding UTF-8 -g %s @BldTmp00.tmp",
        ArgsJavaC
    );

    Status = BLD$ExecuteCommand(Command, &ExitCode);
    if (Status == FALSE || ExitCode != 0) {

        BLD$WriteConsole(L"%BUILD-F-JAVAC, Java compiler error.");
        ExitCode = 1;
    }

    //+
    // Remove all preprocessed files and remove a BLDTMP00.TMP file.
    //-

    DeleteFileW(L"BldTmp00.tmp");
    CurrentEntry = FilesToRemove.ListHead;
    do {
        DeleteFileW(CurrentEntry->String);
        CurrentEntry = CurrentEntry->Next;
    } while (CurrentEntry != FilesToRemove.ListHead);

    //+
    // Free resources and exit.
    //-
    BLD$DestroyList(&FilesToCompile);
    BLD$DestroyList(&FilesToRemove);
    ExitProcess(ExitCode);
}