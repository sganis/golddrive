#include <winsock2.h>
#include <Winhttp.h>
#include <windows.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "winhttp.lib")


typedef struct loggerdata {
    char url[MAX_PATH];
    char data[1024];
} loggerdata;

int post(const char* url, const char* data)
{
    wchar_t wurl[MAX_PATH];
    mbstowcs_s(0, wurl, strlen(url) + 1, url, MAX_PATH);
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;

    BOOL success = WinHttpCrackUrl(wurl, (DWORD)wcslen(wurl), 0, &urlComp);
    if (!success) {
        int error = GetLastError();
        return 1;
    }
    wchar_t wschema[6];
    wchar_t whost[100];
    wchar_t wpath[100];
    ZeroMemory(wschema, sizeof(wschema));
    ZeroMemory(whost, sizeof(whost));
    ZeroMemory(wpath, sizeof(wpath));
    wcsncpy_s(wschema, 6,
        urlComp.lpszScheme, (rsize_t)urlComp.dwSchemeLength);
    wcsncpy_s(whost, 100,
        urlComp.lpszHostName, (rsize_t)urlComp.dwHostNameLength);
    wcsncpy_s(wpath, 100,
        urlComp.lpszUrlPath, (rsize_t)urlComp.dwUrlPathLength);
    //wcsncpy_s(wpath, sizeof(wpath), urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    int port = urlComp.nPort;


    DWORD datalen = (DWORD)strlen(data);
    //wchar_t* wpath = L"/";
    // convert char* to LPWSTR
    //wchar_t whost[MAX_PATH];
    //wchar_t wpath[MAX_PATH];
    //mbstowcs_s(0, whost, strlen(host) + 1, host, MAX_PATH);
    //mbstowcs_s(0, wpath, strlen(page), page, sizeof(wchar_t) * 100);
    //wpath[strlen(page)] = 0;
    printf("schema: %ls\n", wschema);
    printf("host: %ls\n", whost);
    printf("port: %d\n", port);
    printf("path: %ls\n", wpath);

    LPWSTR phost = whost;
    LPWSTR ppath = wpath;
    LPCWSTR additionalHeaders = L"Content-Type: application/x-www-form-urlencoded\r\n";
    BOOL  bResults = FALSE;
    HINTERNET  hSession = NULL;
    HINTERNET  hConnect = NULL;
    HINTERNET  hRequest = NULL;

    hSession = WinHttpOpen(L"HTTP Logger/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession) {
        if (!WinHttpSetTimeouts(hSession, 4000, 4000, 2000, 10000)) {
            printf("Error %u in WinHttpSetTimeouts.\n", GetLastError());
            return 1;
        }
        hConnect = WinHttpConnect(hSession, phost, port, 0);
    }
    //int secflag = https ? WINHTTP_FLAG_SECURE : 0;
    int secflag = WINHTTP_FLAG_SECURE;
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"POST", ppath,
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            secflag); //WINHTTP_FLAG_SECURE
    DWORD headersLength = -1;


    //bResults = WinHttpSendRequest(hRequest,
    //     additionalHeaders, headersLength, (LPVOID)data,
    //     datalen, datalen, 0);

    int retry = 0;
    int optionset = 0;
    int retries = 0;
    int maxretries = 2;
    do
    {
        retry = 0;

        int result = NO_ERROR;
        // no retry on success, possible retry on failure
        bResults = WinHttpSendRequest(hRequest,
            additionalHeaders, headersLength, (LPVOID)data,
            datalen, datalen, 0);

        if (bResults == FALSE)
        {


            result = GetLastError();

            // (1) If you want to allow SSL certificate errors and continue
            // with the connection, you must allow and initial failure and then
            // reset the security flags. From: "HOWTO: Handle Invalid Certificate
            // Authority Error with WinInet"
            // http://support.microsoft.com/default.aspx?scid=kb;EN-US;182888
            if (result == ERROR_WINHTTP_SECURE_FAILURE)
            {
                DWORD dwFlags =
                    SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                    SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
                    SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                    SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;

                if (optionset)
                    break;

                if (WinHttpSetOption(
                    hRequest,
                    WINHTTP_OPTION_SECURITY_FLAGS,
                    &dwFlags,
                    sizeof(dwFlags)))
                {
                    retry = 1;
                    optionset = 1;
                }
            }
            // (2) Negotiate authorization handshakes may return this error
            // and require multiple attempts
            // http://msdn.microsoft.com/en-us/library/windows/desktop/aa383144%28v=vs.85%29.aspx
            else if (result == ERROR_WINHTTP_RESEND_REQUEST)
            {
                retry = 1;
                retries++;
                if (retries > maxretries)
                    break;
            }
        }
    } while (retry);

    if (!bResults) {
        int err = GetLastError();
        printf("Error %d has occurred.\n", err);
        return 1;
    }
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);
    return 0;
}

DWORD WINAPI post_background(LPVOID data)
{
    loggerdata d = *(loggerdata*)data;
    return post(d.url, d.data);
}

#define MAX_THREADS 10

int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("usage: logger.exe <url> <message>");
        return 1;
    }

    //for (int i = 0; i < argc; i++)
    //     printf_s("argv[%d]   %s\n", 1, argv[i]);

    const char* url = argv[1];
    const char* message = argv[2];

    //printf_s("url : %s\n", url);
    //printf_s("message: %s\n", message);

    HANDLE  threads[MAX_THREADS];
    loggerdata* datas[MAX_THREADS];

    for (int i = 0; i < MAX_THREADS; i++) {
        datas[i] = malloc(sizeof(loggerdata));
        memset(datas[i], 0, sizeof(loggerdata));
        strcpy_s(datas[i]->url, MAX_PATH, url);
        char tries[10];
        memset(tries, 0, 10);
        sprintf_s(tries, 10, "try=%d", i);
        strcpy_s(datas[i]->data, MAX_PATH, "message=");
        strcat_s(datas[i]->data, MAX_PATH, message);
        strcat_s(datas[i]->data, MAX_PATH, "&");
        strcat_s(datas[i]->data, MAX_PATH, tries);
        //printf_s("postdata: %s\n", datas[i]->data);
        threads[i] = CreateThread(NULL, 0, post_background, datas[i], 0, NULL);
    }

    WaitForMultipleObjects(MAX_THREADS, threads, TRUE, 5000);
    for (int i = 0; i < MAX_THREADS; i++) {
        CloseHandle(threads[i]);
        free(datas[i]);
    }


    return 0;
}

