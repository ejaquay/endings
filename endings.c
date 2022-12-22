// Determine line ending types for files in a directory tree.
// Should work for ascii and UTF-8 encoded text, others not.

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

int checkendings(const char *);           // Report file line ending types
int scantree(const char *,const char *);  // Scan tree for files that match
int istypebin(const char *);              // Check extension for binary types

int ChSk=0; // path chars skipped when printing

// Call scantree with args filled in
int main(int argc, char **argv)
{
    char * path;
    char * match;

    if (argc > 2) {
        path = argv[1];
        match = argv[2];
    } else if (argc > 1) {
        if (strpbrk(argv[1],"*?")) {
            path = "."; ChSk = 2;
            match = argv[1];
        } else {
            path = argv[1];
            match = NULL;
        }
    } else {
        path = "."; ChSk = 2;
        match = NULL;
    }
    scantree(path,match);
    return 1;
}

// Scan tree for matching files and check their endings
int scantree(const char *path, const char *match)
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
            if (strcmp(f.cFileName,".git") == 0) continue;   //   Skip .git subdir
            scantree(buf,match);                             //   Recurse
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

// Count file endings and report results.  Skip
// that have known extensions of binary files.
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

    // Count CR, LF, and CRLF endings in file
    int ch;
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

    // Report results for files with consistent endings.
    int lines = num_cr + num_lf + num_crlf;
    if (num_cr == lines) {                   // All line endings CR
        printf(" CR    %s\n", file+ChSk);
    } else if (num_lf == lines) {            // All line endings LF
        printf(" LF    %s\n", file+ChSk);
    } else if (num_crlf == lines) {          // All line endings CRLF
        printf(" CRLF  %s\n", file+ChSk);
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
