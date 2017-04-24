#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "toolbox.h"
#include "file_cache.h"

// -----------------------------------------------------------------------------
// BASIC OPERATIONS ON FILES AND FOLDERS
// -----------------------------------------------------------------------------

File* createFile ()
{
    File* new_file = malloc(sizeof(File));
    if (new_file == NULL)
        handleErrorAndExit("malloc() failed in createFile()");

    return new_file;
}

void initFile (File* file, char* name, char* content, int length)
{
    file->name    = name;
    file->content = content;
    file->length  = length;
}

File* createAndInitFile (char* name, char* content, int length)
{
    File* new_file = createFile();
    initFile(new_file, name, content, length);

    return new_file;
}

void deleteFile (File* file)
{
    free(file->name);
    free(file->content);

    free(file);
}

// -----------------------------------------------------------------------------

Folder* createFolder ()
{
    Folder* new_folder = malloc(sizeof(Folder));
    if (new_folder == NULL)
        handleErrorAndExit("malloc() failed in createFolder()");

    return new_folder;
}

void initEmptyFolder (Folder* folder, char* name,
                      const int max_nb_files, const int max_nb_subfolders)
{
    folder->name = name;
    folder->size = 0;

    folder->files = malloc(max_nb_files * sizeof(File*));
    if (folder->files == NULL)
        handleErrorAndExit("malloc() failed in initEmptyFolder()");
    folder->nb_files = 0;

    folder->subfolders = malloc(max_nb_subfolders * sizeof(Folder*));
    if (folder->subfolders == NULL)
        handleErrorAndExit("malloc() failed in initEmptyFolder()");
    folder->nb_subfolders = 0;
}

Folder* createEmptyFolder (char* name, const int max_nb_files, const int max_nb_subfolders)
{
    Folder* new_folder = createFolder();
    initEmptyFolder(new_folder, name, max_nb_files, max_nb_subfolders);

    return new_folder;
}

// Warning: RECURSIVELY delete subfolders and files therein!
void recursivelyDeleteFolder (Folder* folder)
{
    free(folder->name);
    
    for (int i = 0; i < folder->nb_files; i++)
        deleteFile(folder->files[i]);
    free(folder->files);

    for (int i = 0; i < folder->nb_subfolders; i++)
        recursivelyDeleteFolder(folder->subfolders[i]);
    free(folder->subfolders);

    free(folder);
}

// Assumes the files array has free space at nb_files index
// Returns the new size of the folder
int addFileToFolder (Folder* folder, File* file)
{
    // Add the file to the folder
    folder->files[folder->nb_files] = file;
    (folder->nb_files)++;

    // Recompute the folder size, and returns it
    int new_size = folder->size + file->length;
    folder->size = new_size;

    return new_size;
}

// Assumes the subfolders array has free space at nb_subfolders index
// Returns the new size of the folder
int addSubfolderToFolder (Folder* folder, Folder* subfolder)
{
    folder->subfolders[folder->nb_subfolders] = subfolder;
    (folder->nb_subfolders)++;

    // Recompute the folder size, and returns it
    int new_size = folder->size + subfolder->size;
    folder->size = new_size;

    return new_size;
}

// -----------------------------------------------------------------------------
// BASIC OPERATIONS ON FILE CACHE
// -----------------------------------------------------------------------------

FileCache* createFileCache ()
{
    FileCache* new_cache = malloc(sizeof(FileCache));
    if (new_cache == NULL)
        handleErrorAndExit("malloc() failed in createFileCache()");

    return new_cache;
}

// Fails, print a warning and returns false if root folder is larger than allowed max_size
bool initFileCache (FileCache* cache, Folder* root, const int max_size)
{
    if (root->size > max_size)
    {
        printWarning("cache could not be initialized: root folder is too large");
        return false;
    }

    cache->root     = root;
    cache->size     = root->size;
    cache->max_size = max_size;

    return true;
}

/*
FileCache* createAndInitFileCache (const char* name, const char* content, const int length)
{
    FileCache* new_file = createFileCache();
    initFileCache(new_file, name, content, length);

    return new_file;
}
*/

// Warning: all the file and folder structures of root folder are deleted as well!
void deleteFileCache (FileCache* cache)
{
    recursivelyDeleteFolder(cache->root);
    free(cache);
}

// -----------------------------------------------------------------------------
// CACHE BUILDING
// -----------------------------------------------------------------------------


