#ifndef __H_FILE_CACHE__
#define __H_FILE_CACHE__

#include <stdbool.h>
#include <dirent.h>

typedef enum FileState {
    STATE_NOT_LOADED,
    STATE_LOADED_RAW,
    STATE_LOADED_COMPRESSED
} FileState;

typedef enum FileEncoding {
    ENCODING_GZIP,
    ENCODING_NONE
} FileEncoding;

// Structures representing files and folders
// in order to cache them in memory

typedef struct File {
    char*     name;
    char*     path;
    FileState state;

    char* content;
    int   size;

    bool  must_unload;

    char*        type;     // MIME type
    FileEncoding encoding; // Compression format
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
bool initFileCache (FileCache* cache, const int max_size);
FileCache* createEmptyFileCache (const int max_size);
void deleteFileCache (FileCache* cache);
void printFileCache (const FileCache* cache);

void setFileType (File* file);
void setRawFileContent (File* file);
void setCompressedFileContent (File* file);
void removeFileContent (File* file);
bool setFileContent (File* file, const int cache_free_space);
void setFileMetadata (File* file);

bool filenameIsSpecial (const char* filename);
void countFilesAndFoldersInDirectory (DIR* directory, const char* current_folder_path,
                                      int* nb_files, int* nb_subfolders);
void recursivelyFillFolder (DIR* directory, Folder* folder, const char* current_folder_path,
                            int* cache_free_space);
Folder* recursivelyBuildFolder (const char* path, int* cache_free_space);
FileCache* buildCacheFromDisk (char* root_path, const int max_size);

Folder* findSubfolderInFolder (const Folder* folder, const char* subfolder_name);
File* findFileInFolder (const Folder* folder, const char* file_name);
File* findFileInCache (const FileCache* cache, char* path);

#endif
