/* A proxy for Cygwin's git:
 * this invokes C:/cygwin64/bin/bash -c 'git-wrapper "%1" "%2" "%3"...'
 * 
 * How to build: x86_64-w64-mingw32-gcc -o git git.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>

void getCwd(CHAR* buf, size_t len) {
    HMODULE hm = GetModuleHandle(NULL);
    GetModuleFileName(hm, buf, len);
    CHAR* p = strrchr(buf, '\\');
    *(++p) = '\0';
    while (p != buf) {
        if (*p == '\\')
            *p = '/';

        --p;
    }
}

void findCygwinPath(CHAR* buf, size_t len) {
    if (!buf || len <= 5)
        return;

    // read cygwin path from git.cfg
    CHAR path[MAX_PATH] = {};
    getCwd(path, MAX_PATH);
    strcat(path, "git.cfg");
    FILE* cfg = fopen(path, "r");
    if (cfg) {
        int size = fread(buf, sizeof(CHAR), len, cfg);
        fclose(cfg);
        cfg = NULL;
        if (size > 0) {
            CHAR* p = buf + size - 1;
            if (*p != '\\' && *p != '/') {
                ++p;
                *p = '/';
                ++p;
                *p = 0;
            }

            for (size_t i = 0; i < size; ++i) {
                if (*buf == '\\')
                    *buf = '/';

                ++buf;
            }

            return;
        }
    }

    strcpy(path, "c:/cygwin64/bin/");
    for (char d = 'c'; d <= 'z'; ++d) {
        path[0] = d;
        if (access(path, 0) == 0) {
            strncpy(buf, path, strlen(path));
            return;
        }
    }

    strcpy(path, "c:/cygwin/bin/");
    for (char d = 'c'; d <= 'z'; ++d) {
        path[0] = d;
        if (access(path, 0) == 0) {
            strncpy(buf, path, strlen(path));
            return;
        }
    }
}

int main(int argc, char **argv)
{
    // Get the path of this program
    CHAR path[MAX_PATH];
    getCwd(path, MAX_PATH);
    
    char buf[4000] = {};
    findCygwinPath(buf, 4000);
    strcat(buf, "sh -c '");
    strcat(buf, path);
    strcat(buf, "git-wrapper");

    int rest = sizeof(buf) - strlen(buf) - 2; /* 2 for "'\0" */

    for (int i = 1; i < argc; i++) {
        rest -= strlen(argv[i]) + 3; /* 3 for " \"\"" */
        if (rest < 0) {
            fprintf(stderr, "arguments too long\n");
            return 1;
        }
        bool quote = !strstr(argv[i], "\"");
        strcat(buf, " ");
        if (quote) strcat(buf, "\"");
        strcat(buf, argv[i]);
        if (quote) strcat(buf, "\"");
    }
    strcat(buf, "'");
    return system(buf);
}
