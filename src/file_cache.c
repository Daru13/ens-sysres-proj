#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
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

void initFile (File* file, char* name, char* content, int size)
{
    file->name    = name;
    file->content = content;
    file->size    = size;
}

File* createAndInitFile (char* name, char* content, int size)
{
    File* new_file = createFile();
    initFile(new_file, name, content, size);

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
    int new_size = folder->size + file->size;
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

// TODO: split this in smaller functions
Folder* recursivelyBuildFolderFromDisk (char* path)
{
    DIR* directory = opendir(path);
    if (directory == NULL)
        handleErrorAndExit("opendir() failed in recursivelyBuildFolderFromDisk()");

    // Read the directory a first time, to count how many
    // folders and regular files are there inside
    int   nb_files      = 0;
    int   nb_subfolders = 0;
    char* current_path  = malloc(MAX_PATH_LENGTH * sizeof(char));
    if (current_path == NULL)
        handleErrorAndExit("malloc() failed");
    struct stat file_info;

    struct dirent* current_entry = readdir(directory);
    while (current_entry != NULL)
    {
        // Build the path to the current entry
        current_path = strcpy(current_path, path);
        current_path = strcat(current_path, current_entry->d_name);

        // Get info on the current entry
        int return_value = stat(current_path, &file_info);
        if (return_value < 0)
            handleErrorAndExit("stat() failed in recursivelyBuildFolderFromDisk()");

        // Check which kind of file the current entry is, 
        // and update counters accordingly
        if (S_ISREG(file_info.st_mode))
            nb_files++;
        else if (S_ISDIR(file_info.st_mode))
            nb_subfolders++;

        // Move to the next entry
        current_entry = readdir(directory);
    }

    // Create an actual Folder structure, and fill it recursively
    Folder* new_folder = createEmptyFolder(path, nb_files, nb_subfolders);
    rewinddir(directory);

    current_entry = readdir(directory);
    while (current_entry != NULL)
    {
        char* current_entry_name = current_entry->d_name;

        // Build the path to the current entry
        current_path = strcpy(current_path, path);
        current_path = strcat(current_path, current_entry_name);

        // Get info on the current entry
        int return_value = stat(current_path, &file_info);
        if (return_value < 0)
            handleErrorAndExit("stat() failed in recursivelyBuildFolderFromDisk()");

        // If the current entry is a regular file, open it, copy all its content in
        // a newly created File structure's buffer, and close it
        if (S_ISREG(file_info.st_mode))
        {
            // TODO: check stuff here (bad return values, etc)

            int current_entry_size = file_info.st_size;

            // Copy the file's content into a buffer
            char* file_content_buffer = malloc(current_entry_size * sizeof(char));
            if (file_content_buffer == NULL)
                handleErrorAndExit("malloc() failed in recursivelyBuildFolderFromDisk()");

            FILE* current_file = fopen(current_path, "r");
            int nb_bytes_read = fread(file_content_buffer, sizeof(char),
                                      current_entry_size, current_file);
            fclose(current_file);

            // Create a File structure, and add it to the current folder
            File* new_file_node = createAndInitFile(current_entry_name,
                                                    file_content_buffer,
                                                    current_entry_size);
            addFileToFolder(new_folder, new_file_node);
        }

        // If the current entry is a directory, recursively build it
        else if (S_ISDIR(file_info.st_mode))
        {
            Folder* new_subfolder = recursivelyBuildFolderFromDisk(current_path);
            addSubfolderToFolder(new_folder, new_subfolder);
        }

        // Move to the next entry
        current_entry = readdir(directory);
    }

    closedir(directory);
    return new_folder;
}

// TODO: stop ignoring max size :)

FileCache* buildCacheFromDisk (char* root_path, const int max_size)
{
    // Build the root folder recursively
    Folder* root_folder = recursivelyBuildFolderFromDisk(root_path);

    FileCache* new_cache = createFileCache();
    initFileCache(new_cache, root_folder, max_size);

    return new_cache;
}
