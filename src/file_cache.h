#ifndef __H_FILE_CACHE__
#define __H_FILE_CACHE__

#include <stdbool.h>

// Structures representing files and folders
// in order to cache them in memory

typedef enum FileState {
    STATE_NOT_LOADED,
    STATE_LOADED_RAW,
    STATE_LOADED_COMPRESSED
} FileState;

typedef struct File {
    char*     name;
    FileState state;

    char* content;
    int   size;

    char* type;     // MIME type
    char* encoding; // Compression format (possibly NO_ENCODING)
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

#define MAX_NAME_LENGTH          1024
#define MAX_PATH_LENGTH          1024

#define MAX_FILE_TYPE_LENGTH     256
#define MAX_FILE_ENCODING_LENGTH 64

#define MIN_FILE_SIZE_FOR_GZIP   64 // bytes

#define NOT_FOUND                NULL
#define NO_ENCODING              NULL

// -----------------------------------------------------------------------------

File* createFile ();
void initFile (File* file);
File* createAndInitFile ();
void deleteFile (File* file);
char* getFileStateAsString (const FileState state);
void printFile (const File* file, const int indent);

Folder* createFolder ();
void initEmptyFolder (Folder* folder, char* name,
                      const int max_nb_files, const int max_nb_subfolders);
Folder* createEmptyFolder (char* name, const int max_nb_files, const int max_nb_subfolders);
void recursivelyDeleteFolder (Folder* folder);
void recursivelyPrintFolder (const Folder* folder, const int indent);
int addFileToFolder (Folder* folder, File* file);
int addSubfolderToFolder (Folder* folder, Folder* subfolder);

FileCache* createFileCache ();
bool initFileCache (FileCache* cache, Folder* root, const int max_size);
void deleteFileCache (FileCache* cache);
void printFileCache (const FileCache* cache);

void setFileType (File* file, char* path);
void compressFile (File* file, char* path);
Folder* recursivelyBuildFolderFromDisk (char* path);
FileCache* buildCacheFromDisk (char* root_path, const int max_size);

Folder* findSubfolderInFolder (const Folder* folder, const char* subfolder_name);
File* findFileInFolder (const Folder* folder, const char* file_name);
File* findFileInCache (const FileCache* cache, char* path);

#endif
