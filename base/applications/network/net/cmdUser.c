/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS net command
 * FILE:            base/applications/network/net/cmdUser.c
 * PURPOSE:
 *
 * PROGRAMMERS:     Eric Kohl
 *                  Curtis Wilson
 */

#include "net.h"

static WCHAR szPasswordChars[] = L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ@#$%_-+:";

static
int
CompareInfo(const void *a, const void *b)
{
    return _wcsicmp(((PUSER_INFO_0)a)->usri0_name,
                    ((PUSER_INFO_0)b)->usri0_name);
}


static
NET_API_STATUS
EnumerateUsers(VOID)
{
    PUSER_INFO_0 pBuffer = NULL;
    PSERVER_INFO_100 pServer = NULL;
    DWORD dwRead = 0, dwTotal = 0;
    DWORD i;
    DWORD ResumeHandle = 0;
    NET_API_STATUS Status;

    Status = NetServerGetInfo(NULL,
                              100,
                              (LPBYTE*)&pServer);
    if (Status != NERR_Success)
        return Status;

    ConPuts(StdOut, L"\n");
    ConResPrintf(StdOut, IDS_USER_ACCOUNTS, pServer->sv100_name);
    ConPuts(StdOut, L"\n\n");
    PrintPadding(L'-', 79);
    ConPuts(StdOut, L"\n");

    NetApiBufferFree(pServer);

    do
    {
        Status = NetUserEnum(NULL,
                             0,
                             0,
                             (LPBYTE*)&pBuffer,
                             MAX_PREFERRED_LENGTH,
                             &dwRead,
                             &dwTotal,
                             &ResumeHandle);
        if ((Status != NERR_Success) && (Status != ERROR_MORE_DATA))
            return Status;

        qsort(pBuffer,
              dwRead,
              sizeof(PUSER_INFO_0),
              CompareInfo);

        for (i = 0; i < dwRead; i++)
        {
            if (pBuffer[i].usri0_name)
                ConPrintf(StdOut, L"%s\n", pBuffer[i].usri0_name);
        }

        NetApiBufferFree(pBuffer);
        pBuffer = NULL;
    }
    while (Status == ERROR_MORE_DATA);

    return NERR_Success;
}


static
VOID
PrintDateTime(DWORD dwSeconds)
{
    LARGE_INTEGER Time;
    FILETIME FileTime;
    SYSTEMTIME SystemTime;
    WCHAR DateBuffer[80];
    WCHAR TimeBuffer[80];

    RtlSecondsSince1970ToTime(dwSeconds, &Time);
    FileTime.dwLowDateTime = Time.u.LowPart;
    FileTime.dwHighDateTime = Time.u.HighPart;
    FileTimeToLocalFileTime(&FileTime, &FileTime);
    FileTimeToSystemTime(&FileTime, &SystemTime);

    GetDateFormatW(LOCALE_USER_DEFAULT,
                   DATE_SHORTDATE,
                   &SystemTime,
                   NULL,
                   DateBuffer,
                   80);

    GetTimeFormatW(LOCALE_USER_DEFAULT,
                   TIME_NOSECONDS,
                   &SystemTime,
                   NULL,
                   TimeBuffer,
                   80);

    ConPrintf(StdOut, L"%s %s", DateBuffer, TimeBuffer);
}


static
DWORD
GetTimeInSeconds(VOID)
{
    LARGE_INTEGER Time;
    FILETIME FileTime;
    DWORD dwSeconds;

    GetSystemTimeAsFileTime(&FileTime);
    Time.u.LowPart = FileTime.dwLowDateTime;
    Time.u.HighPart = FileTime.dwHighDateTime;
    RtlTimeToSecondsSince1970(&Time, &dwSeconds);

    return dwSeconds;
}


static
NET_API_STATUS
DisplayUser(LPWSTR lpUserName)
{
    PUSER_MODALS_INFO_0 pUserModals = NULL;
    PUSER_INFO_4 pUserInfo = NULL;
    PLOCALGROUP_USERS_INFO_0 pLocalGroupInfo = NULL;
    PGROUP_USERS_INFO_0 pGroupInfo = NULL;
    DWORD dwLocalGroupRead, dwLocalGroupTotal;
    DWORD dwGroupRead, dwGroupTotal;
    DWORD dwLastSet;
    DWORD i;
    INT nPaddedLength = 29;
    NET_API_STATUS Status;

    /* Modify the user */
    Status = NetUserGetInfo(NULL,
                            lpUserName,
                            4,
                            (LPBYTE*)&pUserInfo);
    if (Status != NERR_Success)
        return Status;

    Status = NetUserModalsGet(NULL,
                              0,
                              (LPBYTE*)&pUserModals);
    if (Status != NERR_Success)
        goto done;

    Status = NetUserGetLocalGroups(NULL,
                                   lpUserName,
                                   0,
                                   0,
                                   (LPBYTE*)&pLocalGroupInfo,
                                   MAX_PREFERRED_LENGTH,
                                   &dwLocalGroupRead,
                                   &dwLocalGroupTotal);
    if (Status != NERR_Success)
        goto done;

    Status = NetUserGetGroups(NULL,
                              lpUserName,
                              0,
                              (LPBYTE*)&pGroupInfo,
                              MAX_PREFERRED_LENGTH,
                              &dwGroupRead,
                              &dwGroupTotal);
    if (Status != NERR_Success)
        goto done;

    PrintPaddedResourceString(IDS_USER_NAME, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_name);

    PrintPaddedResourceString(IDS_USER_FULL_NAME, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_full_name);

    PrintPaddedResourceString(IDS_USER_COMMENT, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_comment);

    PrintPaddedResourceString(IDS_USER_USER_COMMENT, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_usr_comment);

    PrintPaddedResourceString(IDS_USER_COUNTRY_CODE, nPaddedLength);
    ConPrintf(StdOut, L"%03ld ()\n", pUserInfo->usri4_country_code);

    PrintPaddedResourceString(IDS_USER_ACCOUNT_ACTIVE, nPaddedLength);
    if (pUserInfo->usri4_flags & UF_ACCOUNTDISABLE)
        ConResPuts(StdOut, IDS_GENERIC_NO);
    else if (pUserInfo->usri4_flags & UF_LOCKOUT)
        ConResPuts(StdOut, IDS_GENERIC_LOCKED);
    else
        ConResPuts(StdOut, IDS_GENERIC_YES);
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_USER_ACCOUNT_EXPIRES, nPaddedLength);
    if (pUserInfo->usri4_acct_expires == TIMEQ_FOREVER)
        ConResPuts(StdOut, IDS_GENERIC_NEVER);
    else
        PrintDateTime(pUserInfo->usri4_acct_expires);
    ConPuts(StdOut, L"\n\n");

    PrintPaddedResourceString(IDS_USER_PW_LAST_SET, nPaddedLength);
    dwLastSet = GetTimeInSeconds() - pUserInfo->usri4_password_age;
    PrintDateTime(dwLastSet);
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_USER_PW_EXPIRES, nPaddedLength);
    if ((pUserInfo->usri4_flags & UF_DONT_EXPIRE_PASSWD) || pUserModals->usrmod0_max_passwd_age == TIMEQ_FOREVER)
        ConResPuts(StdOut, IDS_GENERIC_NEVER);
    else
        PrintDateTime(dwLastSet + pUserModals->usrmod0_max_passwd_age);
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_USER_PW_CHANGEABLE, nPaddedLength);
    PrintDateTime(dwLastSet + pUserModals->usrmod0_min_passwd_age);
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_USER_PW_REQUIRED, nPaddedLength);
    ConResPuts(StdOut, (pUserInfo->usri4_flags & UF_PASSWD_NOTREQD) ? IDS_GENERIC_NO : IDS_GENERIC_YES);
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_USER_CHANGE_PW, nPaddedLength);
    ConResPuts(StdOut, (pUserInfo->usri4_flags & UF_PASSWD_CANT_CHANGE) ? IDS_GENERIC_NO : IDS_GENERIC_YES);
    ConPuts(StdOut, L"\n\n");

    PrintPaddedResourceString(IDS_USER_WORKSTATIONS, nPaddedLength);
    if (pUserInfo->usri4_workstations == NULL || wcslen(pUserInfo->usri4_workstations) == 0)
        ConResPuts(StdOut, IDS_GENERIC_ALL);
    else
        ConPrintf(StdOut, L"%s", pUserInfo->usri4_workstations);
    ConPuts(StdOut, L"\n");

    PrintPaddedResourceString(IDS_USER_LOGON_SCRIPT, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_script_path);

    PrintPaddedResourceString(IDS_USER_PROFILE, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_profile);

    PrintPaddedResourceString(IDS_USER_HOME_DIR, nPaddedLength);
    ConPrintf(StdOut, L"%s\n", pUserInfo->usri4_home_dir);

    PrintPaddedResourceString(IDS_USER_LAST_LOGON, nPaddedLength);
    if (pUserInfo->usri4_last_logon == 0)
        ConResPuts(StdOut, IDS_GENERIC_NEVER);
    else
        PrintDateTime(pUserInfo->usri4_last_logon);
    ConPuts(StdOut, L"\n\n");

    PrintPaddedResourceString(IDS_USER_LOGON_HOURS, nPaddedLength);
    if (pUserInfo->usri4_logon_hours == NULL)
        ConResPuts(StdOut, IDS_GENERIC_ALL);
    ConPuts(StdOut, L"\n\n");

    ConPuts(StdOut, L"\n");
    PrintPaddedResourceString(IDS_USER_LOCAL_GROUPS, nPaddedLength);
    if (dwLocalGroupTotal != 0 && pLocalGroupInfo != NULL)
    {
        for (i = 0; i < dwLocalGroupTotal; i++)
        {
            if (i != 0)
                PrintPadding(L' ', nPaddedLength);
            ConPrintf(StdOut, L"*%s\n", pLocalGroupInfo[i].lgrui0_name);
        }
    }
    else
    {
        ConPuts(StdOut, L"\n");
    }

    PrintPaddedResourceString(IDS_USER_GLOBAL_GROUPS, nPaddedLength);
    if (dwGroupTotal != 0 && pGroupInfo != NULL)
    {
        for (i = 0; i < dwGroupTotal; i++)
        {
            if (i != 0)
                PrintPadding(L' ', nPaddedLength);
            ConPrintf(StdOut, L"*%s\n", pGroupInfo[i].grui0_name);
        }
    }
    else
    {
        ConPuts(StdOut, L"\n");
    }

done:
    if (pGroupInfo != NULL)
        NetApiBufferFree(pGroupInfo);

    if (pLocalGroupInfo != NULL)
        NetApiBufferFree(pLocalGroupInfo);

    if (pUserModals != NULL)
        NetApiBufferFree(pUserModals);

    if (pUserInfo != NULL)
        NetApiBufferFree(pUserInfo);

    return NERR_Success;
}


static
VOID
ReadPassword(
    LPWSTR *lpPassword,
    LPBOOL lpAllocated)
{
    WCHAR szPassword1[PWLEN + 1];
    WCHAR szPassword2[PWLEN + 1];
    LPWSTR ptr;

    *lpAllocated = FALSE;

    while (TRUE)
    {
        ConResPuts(StdOut, IDS_USER_ENTER_PASSWORD1);
        ReadFromConsole(szPassword1, PWLEN + 1, FALSE);
        ConPuts(StdOut, L"\n");

        ConResPuts(StdOut, IDS_USER_ENTER_PASSWORD2);
        ReadFromConsole(szPassword2, PWLEN + 1, FALSE);
        ConPuts(StdOut, L"\n");

        if (wcslen(szPassword1) == wcslen(szPassword2) &&
            wcscmp(szPassword1, szPassword2) == 0)
        {
            ptr = HeapAlloc(GetProcessHeap(),
                            0,
                            (wcslen(szPassword1) + 1) * sizeof(WCHAR));
            if (ptr != NULL)
            {
                wcscpy(ptr, szPassword1);
                *lpPassword = ptr;
                *lpAllocated = TRUE;
                return;
            }
        }
        else
        {
            ConPuts(StdOut, L"\n");
            ConResPuts(StdOut, IDS_USER_NO_PASSWORD_MATCH);
            ConPuts(StdOut, L"\n");
            *lpPassword = NULL;
        }
    }
}


static
VOID
GenerateRandomPassword(
    LPWSTR *lpPassword,
    LPBOOL lpAllocated)
{
    LPWSTR pPassword = NULL;
    INT nCharsLen, i, nLength = 8;

    srand(GetTickCount());

    pPassword = HeapAlloc(GetProcessHeap(),
                          HEAP_ZERO_MEMORY,
                          (nLength + 1) * sizeof(WCHAR));
    if (pPassword == NULL)
        return;

    nCharsLen = wcslen(szPasswordChars);

    for (i = 0; i < nLength; i++)
    {
        pPassword[i] = szPasswordChars[rand() % nCharsLen];
    }

    *lpPassword = pPassword;
    *lpAllocated = TRUE;
}


static
NET_API_STATUS
BuildWorkstationsList(
    _Out_ PWSTR *pWorkstationsList,
    _In_ PWSTR pRaw)
{
    BOOL isLastSep, isSep;
    INT i, j;
    WCHAR c;
    INT nLength = 0;
    INT nArgs = 0;
    INT nRawLength;
    PWSTR pList;

    /* Check for invalid characters in the raw string */
    if (wcspbrk(pRaw, L"/[]=?\\+:.") != NULL)
        return 3952;

    /* Count the number of workstations in the list and
     * the required buffer size */
    isLastSep = FALSE;
    isSep = FALSE;
    nRawLength = wcslen(pRaw);
    for (i = 0; i < nRawLength; i++)
    {
        c = pRaw[i];
        if (c == L',' || c == L';')
            isSep = TRUE;

        if (isSep == TRUE)
        {
            if ((isLastSep == FALSE) && (i != 0) && (i != nRawLength - 1))
                nLength++;
        }
        else
        {
            nLength++;

            if (isLastSep == TRUE || (isLastSep == FALSE && i == 0))
                nArgs++;
        }

        isLastSep = isSep;
        isSep = FALSE;
    }

    nLength++;

    /* Leave, if there are no workstations in the list */
    if (nArgs == 0)
    {
        pWorkstationsList = NULL;
        return NERR_Success;
    }

    /* Fail if there are more than eight workstations in the list */
    if (nArgs > 8)
        return 3951;

    /* Allocate the buffer for the clean workstation list */
    pList = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nLength * sizeof(WCHAR));
    if (pList == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /* Build the clean workstation list */
    isLastSep = FALSE;
    isSep = FALSE;
    nRawLength = wcslen(pRaw);
    for (i = 0, j = 0; i < nRawLength; i++)
    {
        c = pRaw[i];
        if (c == L',' || c == L';')
            isSep = TRUE;

        if (isSep == TRUE)
        {
            if ((isLastSep == FALSE) && (i != 0) && (i != nRawLength - 1))
            {
                pList[j] = L',';
                j++;
            }
        }
        else
        {
            pList[j] = c;
            j++;

            if (isLastSep == TRUE || (isLastSep == FALSE && i == 0))
                nArgs++;
        }

        isLastSep = isSep;
        isSep = FALSE;
    }

    *pWorkstationsList = pList;

    return NERR_Success;
}


static
BOOL
ReadNumber(
    PWSTR *s,
    PWORD pwValue)
{
    if (!iswdigit(**s))
        return FALSE;

    while (iswdigit(**s))
    {
        *pwValue = *pwValue * 10 + **s - L'0';
        (*s)++;
    }

    return TRUE;
}


static
BOOL
ReadSeparator(
    PWSTR *s)
{
    if (**s == L'/' || **s == L'.')
    {
        (*s)++;
        return TRUE;
    }

    return FALSE;
}


static
BOOL
ParseDate(
    PWSTR s,
    PULONG pSeconds)
{
    SYSTEMTIME SystemTime = {0};
    FILETIME LocalFileTime, FileTime;
    LARGE_INTEGER Time;
    INT nDateFormat = 0;
    PWSTR p = s;

    if (!*s)
        return FALSE;

    GetLocaleInfoW(LOCALE_USER_DEFAULT,
                   LOCALE_IDATE,
                   (PWSTR)&nDateFormat,
                   sizeof(INT));

    switch (nDateFormat)
    {
        case 0: /* mmddyy */
        default:
            if (!ReadNumber(&p, &SystemTime.wMonth))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wDay))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wYear))
                return FALSE;
            break;

        case 1: /* ddmmyy */
            if (!ReadNumber(&p, &SystemTime.wDay))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wMonth))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wYear))
                return FALSE;
            break;

        case 2: /* yymmdd */
            if (!ReadNumber(&p, &SystemTime.wYear))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wMonth))
                return FALSE;
            if (!ReadSeparator(&p))
                return FALSE;
            if (!ReadNumber(&p, &SystemTime.wDay))
                return FALSE;
            break;
    }

    /* if only entered two digits: */
    /*   assume 2000's if value less than 80 */
    /*   assume 1900's if value greater or equal 80 */
    if (SystemTime.wYear <= 99)
    {
        if (SystemTime.wYear >= 80)
            SystemTime.wYear += 1900;
        else
            SystemTime.wYear += 2000;
    }

    if (!SystemTimeToFileTime(&SystemTime, &LocalFileTime))
        return FALSE;

    if (!LocalFileTimeToFileTime(&LocalFileTime, &FileTime))
        return FALSE;

    Time.u.LowPart = FileTime.dwLowDateTime;
    Time.u.HighPart = FileTime.dwHighDateTime;

    if (!RtlTimeToSecondsSince1970(&Time, pSeconds))
        return FALSE;

    return TRUE;
}


INT
cmdUser(
    INT argc,
    WCHAR **argv)
{
    INT i, j;
    INT result = 0;
    BOOL bAdd = FALSE;
    BOOL bDelete = FALSE;
#if 0
    BOOL bDomain = FALSE;
#endif
    BOOL bRandomPassword = FALSE;
    LPWSTR lpUserName = NULL;
    LPWSTR lpPassword = NULL;
    PUSER_INFO_4 pUserInfo = NULL;
    USER_INFO_4 UserInfo;
    LPWSTR pWorkstations = NULL;
    LPWSTR p;
    LPWSTR endptr;
    DWORD value;
    BOOL bPasswordAllocated = FALSE;
    NET_API_STATUS Status;

    if (argc == 2)
    {
        Status = EnumerateUsers();
        ConPrintf(StdOut, L"Status: %lu\n", Status);
        return 0;
    }
    else if (argc == 3)
    {
        Status = DisplayUser(argv[2]);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
        return 0;
    }

    i = 2;
    if (argv[i][0] != L'/')
    {
        lpUserName = argv[i];
//        ConPrintf(StdOut, L"User: %s\n", lpUserName);
        i++;
    }

    if (argv[i][0] != L'/')
    {
        lpPassword = argv[i];
//        ConPrintf(StdOut, L"Password: %s\n", lpPassword);
        i++;
    }

    for (j = i; j < argc; j++)
    {
        if (_wcsicmp(argv[j], L"/help") == 0)
        {
            ConResPuts(StdOut, IDS_USER_HELP);
            return 0;
        }
        else if (_wcsicmp(argv[j], L"/add") == 0)
        {
            bAdd = TRUE;
        }
        else if (_wcsicmp(argv[j], L"/delete") == 0)
        {
            bDelete = TRUE;
        }
        else if (_wcsicmp(argv[j], L"/domain") == 0)
        {
            ConResPrintf(StdErr, IDS_ERROR_OPTION_NOT_SUPPORTED, L"/DOMAIN");
#if 0
            bDomain = TRUE;
#endif
        }
        else if (_wcsicmp(argv[j], L"/random") == 0)
        {
            bRandomPassword = TRUE;
            GenerateRandomPassword(&lpPassword,
                                   &bPasswordAllocated);
        }
    }

    if (bAdd && bDelete)
    {
        result = 1;
        goto done;
    }

    /* Interactive password input */
    if (lpPassword != NULL && wcscmp(lpPassword, L"*") == 0)
    {
        ReadPassword(&lpPassword,
                     &bPasswordAllocated);
    }

    if (!bAdd && !bDelete)
    {
        /* Modify the user */
        Status = NetUserGetInfo(NULL,
                                lpUserName,
                                4,
                                (LPBYTE*)&pUserInfo);
        if (Status != NERR_Success)
        {
            ConPrintf(StdOut, L"Status: %lu\n", Status);
            result = 1;
            goto done;
        }
    }
    else if (bAdd && !bDelete)
    {
        /* Add the user */
        ZeroMemory(&UserInfo, sizeof(USER_INFO_4));

        UserInfo.usri4_name = lpUserName;
        UserInfo.usri4_password = lpPassword;
        UserInfo.usri4_flags = UF_SCRIPT | UF_NORMAL_ACCOUNT;

        pUserInfo = &UserInfo;
    }

    for (j = i; j < argc; j++)
    {
        if (_wcsnicmp(argv[j], L"/active:", 8) == 0)
        {
            p = &argv[i][8];
            if (_wcsicmp(p, L"yes") == 0)
            {
                pUserInfo->usri4_flags &= ~UF_ACCOUNTDISABLE;
            }
            else if (_wcsicmp(p, L"no") == 0)
            {
                pUserInfo->usri4_flags |= UF_ACCOUNTDISABLE;
            }
            else
            {
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/ACTIVE");
                result = 1;
                goto done;
            }
        }
        else if (_wcsnicmp(argv[j], L"/comment:", 9) == 0)
        {
            pUserInfo->usri4_comment = &argv[j][9];
        }
        else if (_wcsnicmp(argv[j], L"/countrycode:", 13) == 0)
        {
            p = &argv[i][13];
            value = wcstoul(p, &endptr, 10);
            if (*endptr != 0)
            {
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/COUNTRYCODE");
                result = 1;
                goto done;
            }

            /* FIXME: verify the country code */

            pUserInfo->usri4_country_code = value;
        }
        else if (_wcsnicmp(argv[j], L"/expires:", 9) == 0)
        {
            p = &argv[i][9];
            if (_wcsicmp(p, L"never") == 0)
            {
                pUserInfo->usri4_acct_expires = TIMEQ_FOREVER;
            }
            else if (!ParseDate(p, &pUserInfo->usri4_acct_expires))
            {
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/EXPIRES");
                result = 1;
                goto done;

                /* FIXME: Parse the date */
//                ConResPrintf(StdErr, IDS_ERROR_OPTION_NOT_SUPPORTED, L"/EXPIRES");
            }
        }
        else if (_wcsnicmp(argv[j], L"/fullname:", 10) == 0)
        {
            pUserInfo->usri4_full_name = &argv[j][10];
        }
        else if (_wcsnicmp(argv[j], L"/homedir:", 9) == 0)
        {
            pUserInfo->usri4_home_dir = &argv[j][9];
        }
        else if (_wcsnicmp(argv[j], L"/passwordchg:", 13) == 0)
        {
            p = &argv[i][13];
            if (_wcsicmp(p, L"yes") == 0)
            {
                pUserInfo->usri4_flags &= ~UF_PASSWD_CANT_CHANGE;
            }
            else if (_wcsicmp(p, L"no") == 0)
            {
                pUserInfo->usri4_flags |= UF_PASSWD_CANT_CHANGE;
            }
            else
            {
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/PASSWORDCHG");
                result = 1;
                goto done;
            }
        }
        else if (_wcsnicmp(argv[j], L"/passwordreq:", 13) == 0)
        {
            p = &argv[i][13];
            if (_wcsicmp(p, L"yes") == 0)
            {
                pUserInfo->usri4_flags &= ~UF_PASSWD_NOTREQD;
            }
            else if (_wcsicmp(p, L"no") == 0)
            {
                pUserInfo->usri4_flags |= UF_PASSWD_NOTREQD;
            }
            else
            {
                ConResPrintf(StdErr, IDS_ERROR_INVALID_OPTION_VALUE, L"/PASSWORDREQ");
                result = 1;
                goto done;
            }
        }
        else if (_wcsnicmp(argv[j], L"/profilepath:", 13) == 0)
        {
            pUserInfo->usri4_profile = &argv[j][13];
        }
        else if (_wcsnicmp(argv[j], L"/scriptpath:", 12) == 0)
        {
            pUserInfo->usri4_script_path = &argv[j][12];
        }
        else if (_wcsnicmp(argv[j], L"/times:", 7) == 0)
        {
            /* FIXME */
            ConResPrintf(StdErr, IDS_ERROR_OPTION_NOT_SUPPORTED, L"/TIMES");
        }
        else if (_wcsnicmp(argv[j], L"/usercomment:", 13) == 0)
        {
            pUserInfo->usri4_usr_comment = &argv[j][13];
        }
        else if (_wcsnicmp(argv[j], L"/workstations:", 14) == 0)
        {
            p = &argv[i][14];
            if (wcscmp(p, L"*") == 0 || wcscmp(p, L"") == 0)
            {
                pUserInfo->usri4_workstations = NULL;
            }
            else
            {
                Status = BuildWorkstationsList(&pWorkstations, p);
                if (Status == NERR_Success)
                {
                    pUserInfo->usri4_workstations = pWorkstations;
                }
                else
                {
                    ConPrintf(StdOut, L"Status %lu\n\n", Status);
                    result = 1;
                    goto done;
                }
            }
        }
    }

    if (!bAdd && !bDelete)
    {
        /* Modify the user */
        Status = NetUserSetInfo(NULL,
                                lpUserName,
                                4,
                                (LPBYTE)pUserInfo,
                                NULL);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }
    else if (bAdd && !bDelete)
    {
        /* Add the user */
        Status = NetUserAdd(NULL,
                            4,
                            (LPBYTE)pUserInfo,
                            NULL);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }
    else if (!bAdd && bDelete)
    {
        /* Delete the user */
        Status = NetUserDel(NULL,
                            lpUserName);
        ConPrintf(StdOut, L"Status: %lu\n", Status);
    }

    if (Status == NERR_Success &&
        lpPassword != NULL &&
        bRandomPassword == TRUE)
    {
        ConPrintf(StdOut, L"The password for %s is: %s\n", lpUserName, lpPassword);
    }

done:
    if (pWorkstations != NULL)
        HeapFree(GetProcessHeap(), 0, pWorkstations);

    if ((bPasswordAllocated == TRUE) && (lpPassword != NULL))
        HeapFree(GetProcessHeap(), 0, lpPassword);

    if (!bAdd && !bDelete && pUserInfo != NULL)
        NetApiBufferFree(pUserInfo);

    if (result != 0)
    {
        ConResPuts(StdOut, IDS_GENERIC_SYNTAX);
        ConResPuts(StdOut, IDS_USER_SYNTAX);
    }

    return result;
}

/* EOF */
