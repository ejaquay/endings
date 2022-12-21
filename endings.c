// Determine line ending types for files in a directory tree.
// Should work for ascii and UTF-8 encoded text, others not.

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

int checkendings(const char *);
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
            scandir(argv[1],NULL);
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
    } else {                                                 // Else not dir
        checkendings(path);                                  //   check endings
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
            if (match)                                       //   If a match spec
                if (! PathMatchSpec(f.cFileName,match))      //     Skip unmatched files
                    continue;
            checkendings(buf);                               //   Check line endings
        }
    } while (FindNextFile(h, &f));                           // Next file

    FindClose(h);
    return 1;
}

int checkendings(const char *file)
{
    FILE *in;
    int num_cr = 0;
    int num_lf = 0;
    int num_crlf = 0;
    int last_cr = 0;

    if (istypebin(file)) return 0;          // Skip obvious binary files.
    in = fopen(file, "rb");                 // Open file for bin read
    if (in == NULL) return 0;               // Abort if open failed

    int ch;                                 // Count CR, LF, and CRLF
    while ((ch = fgetc(in)) != EOF) {
        switch (ch) {
        case '\r':                          // CR found
            num_cr++;                       // Count the CR
            last_cr = 1;
            break;
        case '\n':                          // LF found
            if (last_cr) {                  // If last chr was CR
                num_crlf++;                 //   Count as CRLF
                num_cr--;                   //   Uncount the CR
            } else {                        // else
                num_lf++;                   //   Count the LF
            }
        default:
            last_cr = 0;                    // Clear last char CR flag
            break;
        }
    }
    fclose(in);                              // Close the file

    int lines = num_cr + num_lf + num_crlf;  // Sum the lines
    if (num_cr == lines) {                   // Text all lines CR
        printf(" CR    %s\n", file);
    } else if (num_lf == lines) {            // Text all lines LF
        printf(" LF    %s\n", file);
    } else if (num_crlf == lines) {          // Text all lines CRLF
        printf(" CRLF  %s\n", file);
    }
    return 1;
}

int istypebin(const char *file)
{
    char *ext = strrchr(file, '.');
    if (ext == NULL) return 0;
    char *list[] = {  // List of types assumed to be binary
        ".obj",
        ".exe",
        ".dll",
        ".lib",
        ".pdb",
        ".sbr",
        ".pdf",
        ".png",
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
