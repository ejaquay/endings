// Determine line ending types for files in directory tree

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

int checkfile(const char *);
int scandir(const char *,const char *);
int istypebin(const char *);

int main(int argc, char **argv)
{
    if (argc > 2) {
        scandir(argv[1],argv[2]);
    } else if (argc > 1) {
        char * wild=strpbrk(argv[1],"*?");
        if (wild) {
            scandir(".",argv[1]);
        } else {
            scandir(argv[1],"*");
        }
    } else {
        scandir(".",NULL);
    }
    return 1;
}

int scandir(const char *path, const char *match)
{
    WIN32_FIND_DATA f;
    HANDLE h;
    char buf[MAX_PATH];

    DWORD attr = GetFileAttributes(path);                    // Check path

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {                   // If path a directory
        strcpy_s(buf, MAX_PATH, path);                       //   Set up wildcard
        strcat_s(buf, MAX_PATH, "\\*");
        h = FindFirstFile(buf, &f);                          //   Find first file
        if (h == INVALID_HANDLE_VALUE) return 0;             //   Abort on error
    } else {                                                 // Else
        checkfile(path);                                     //   Just check file
        return 1;                                            //   and return
    }

    do {                                                     // Do the rest
        strcpy_s(buf, MAX_PATH, path);                       //   Prepend path to filename
        strcat_s(buf, MAX_PATH, "\\");
        strcat_s(buf, MAX_PATH, f.cFileName);

        if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { // If a sub directory
            if (strcmp(f.cFileName,".") == 0) continue;      //   Skip current
            if (strcmp(f.cFileName,"..") == 0) continue;     //   Skip parent
            scandir(buf,match);                              //   Recurse
        } else {                                             // Else
            if (f.nFileSizeLow == 0) continue;               //   Skip empty files
            if (match != NULL)                               //   Skip obvious binaries
                if (! PathMatchSpec(f.cFileName,match)) continue;
            checkfile(buf);                                  //   Check endings
        }
    } while (FindNextFile(h, &f));                           // Next file

    FindClose(h);
    return 1;
}

int checkfile(const char *file)
{
    FILE *in;
    int num_cr = 0;
    int num_lf = 0;
    int num_crlf = 0;
    int last_cr = 0;

    // Skip obvious binary files.
    if (istypebin(file)) return 0;

    // Open file for bin read
    in = fopen(file, "rb");
    if (in == NULL) return 0;

    // Count lines and line ending characters
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        switch (ch) {
        case '\r':    // CR
            num_cr++;
            last_cr = 1;
            break;
        case '\n':    // LF
            if (last_cr) {
                num_crlf++;
                num_cr--;
            } else {
                num_lf++;
            }
        default:
            last_cr = 0;
            break;
        }
    }

    // close the file
    fclose(in);

    // Print result for file that appears to be text
    int lines = num_cr + num_lf + num_crlf;
    if (num_cr == lines) {
        printf(" CR    %s\n", file);
    } else if (num_lf == lines) {
        printf(" LF    %s\n", file);
    } else if (num_crlf == lines) {
        printf(" CRLF  %s\n", file);
    }

    return 1;
}

int istypebin(const char *file)
{
    char *ext = strrchr(file, '.');
    if (ext == NULL) return 0;

    char *list[] = {
        ".png",
        ".obj",
        ".exe",
        ".dll",
        ".lib",
        ".pdb",
        ".sbr",
        ".pdf",
        ".res",
        ".tlog",
        ".ico",
        ".db",
        ".pack",
        ".zip",
        ".idx",
        ".dsk",
        ".wav",
        ".vhd",
        ".rom",
        ".tar",
        ".tgz",
        ".iso",
        ".ova",
        NULL
    };

    int i = 0;
    while(list[i]) if (stricmp(ext,list[i++]) == 0) return 1;
    return 0;
}
