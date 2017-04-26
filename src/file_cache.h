#ifndef __H_FILE_CACHE__
#define __H_FILE_CACHE__

#include <stdbool.h>

// Structures representing files and folders
// in order to cache them in memory

typedef struct File {
    char* name;

    char* content;
    int   size;

    // Encoding or more?
} File;

typedef struct Folder {
    char* name;
    int   size;

    struct File** files;
    int    nb_files;

    struct Folder** subfolders;
    int      nb_subfolders;
} Folder;

// Cache structure, containing the above ones

typedef struct FileCache {
    Folder* root;

    int     size;
    int     max_size;
} FileCache;

// -----------------------------------------------------------------------------

#define MAX_PATH_LENGTH 1024

// -----------------------------------------------------------------------------

File* createFile ();
void initFile (File* file, char* name, char* content, int size);
File* createAndInitFile (char* name, char* content, int size);
void deleteFile (File* file);

Folder* createFolder ();
void initEmptyFolder (Folder* folder, char* name,
                      const int max_nb_files, const int max_nb_subfolders);
Folder* createEmptyFolder (char* name, const int max_nb_files, const int max_nb_subfolders);
void recursivelyDeleteFolder (Folder* folder);
int addFileToFolder (Folder* folder, File* file);
int addSubfolderToFolder (Folder* folder, Folder* subfolder);

FileCache* createFileCache ();
bool initFileCache (FileCache* cache, Folder* root, const int max_size);
void deleteFileCache (FileCache* cache);

#endif
