#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "toolbox.h"
#include "system.h"
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

void initFile (File* file)
{
    file->name    = NULL;
    file->path    = NULL;
    file->state   = STATE_NOT_LOADED;
    file->content = NULL;
    file->size    = 0;

    file->must_unload = false;

    file->type = malloc(MAX_FILE_TYPE_LENGTH * sizeof(char));
    if (file->type == NULL)
        handleErrorAndExit("malloc() failed in initFile()");

    file->encoding = ENCODING_NONE;
}

File* createAndInitFile ()
{
    File* new_file = createFile();
    initFile(new_file);

    return new_file;
}

void deleteFile (File* file)
{
    free(file->name);
    free(file->content);
    free(file->type);

    free(file);
}

char* getFileStateAsString (const FileState state)
{
    switch (state)
    {
        case STATE_NOT_LOADED:
            return "NOT_LOADED";
        case STATE_LOADED_RAW:
            return "LOADED_RAW";
        case STATE_LOADED_COMPRESSED:
            return "LOADED_COMPRESSED";

        default:
            return "UNKNOWN STATE";
    }
}

void printFile (const File* file, const int indent)
{
    char indent_space[indent + 1];
    for (int i = 0; i < indent; i++)
        indent_space[i] = ' ';
    indent_space[indent] = '\0';

    // Use the right unit (b, kB, MB) for a cleaner file size
    float printed_file_size = file->size > 1000
                            ? file->size > 1000000
                              ? (float) file->size / 1000000
                              : (float) file->size / 1000
                            : (float) file->size;

    char* file_size_unit = file->size > 1000
                         ? file->size > 1000000
                           ? "MB"
                           : "kB"
                         : "b";

    printf("%s%s (%sstate:%s %s, %ssize:%s %.1f%s, %stype:%s %s)\n",
           indent_space, file->name,
           COLOR_BOLD, COLOR_RESET, getFileStateAsString(file->state),
           COLOR_BOLD, COLOR_RESET, printed_file_size, file_size_unit,
           COLOR_BOLD, COLOR_RESET, file->type);
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

void recursivelyPrintFolder (const Folder* folder, const int indent)
{
    char indent_space[indent + 1];
    for (int i = 0; i < indent; i++)
        indent_space[i] = ' ';
    indent_space[indent] = '\0';

    printColor(COLOR_BOLD_BLUE, "%s%s\n", indent_space, folder->name);

    // Print the folder's content: files first, subfolders then
    int new_indent = indent + 2;

    for (int i = 0; i < folder->nb_files; i++)
        printFile(folder->files[i], new_indent);

    for (int i = 0; i < folder->nb_subfolders; i++)
        recursivelyPrintFolder(folder->subfolders[i], new_indent);
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

void initEmptyFileCache (FileCache* cache, const int max_size)
{
    cache->root     = NULL;
    cache->size     = 0;
    cache->max_size = max_size;
}

FileCache* createEmptyFileCache (const int max_size)
{
    FileCache* new_cache = createFileCache();
    initEmptyFileCache(new_cache, max_size);

    return new_cache;
}

// Warning: all the file and folder structures of root folder are deleted as well!
void deleteFileCache (FileCache* cache)
{
    if (cache->root != NULL)
        recursivelyDeleteFolder(cache->root);
    free(cache);
}

void printFileCache (const FileCache* cache)
{
    printSubtitle("________________ FILE CACHE ________________");
    printf("\nUsed memory: %.2f/%.2f kb (%3.1f%%)\n\n",
           ((float) cache->size) / 1000, ((float) cache->max_size) / 1000,
           (((float) cache->size) / ((float) cache->max_size)) * 100.0);
    recursivelyPrintFolder(cache->root, 0);
}

// -----------------------------------------------------------------------------
// OPERATIONS ON FILE [CONTENT]
// -----------------------------------------------------------------------------

// By using standard program "file", guess the type and encoding of a file
void setFileType (File* file)
{
    char* command = "file";
    char* execvp_argv[] = {
            "file",     // Command name (as argv[0])
            "--mime",   // Output "[MIME type]; [MIME encoding]"
            "-0b",      // Do not prepend filename
            file->path, // Path to the file
            NULL
    };

    int filetype_length = runReadableProcess(command, execvp_argv,
                                             file->type, MAX_FILE_TYPE_LENGTH - 1);
    
    // Null-terminate the string by replacing the newline sent by 'file'
    // If less than two bytes have been read, print a warning a leave the string empty
    if (filetype_length < 2)
    {
        printWarning("Warning: unable to read a valid filetype of %s", file->path);
        
        free(file->type);
        file->type = NULL;
    }
    else
        file->type[filetype_length - 1] = '\0';
}


// File path and size must be set before calling this function!
void setRawFileContent (File* file)
{
    // Buffer where to store the file data
    file->content = malloc(file->size * sizeof(char));
    if (file->content == NULL)
        handleErrorAndExit("malloc() failed in setRawFileContent()");

    // Get the file content from the disk
    int file_fd = open(file->path, O_RDONLY);
    if (file_fd < 0)
        handleErrorAndExit("open() failed in setRawFileContent()");

    int nb_bytes_read = 0;
    while (nb_bytes_read < file->size)
    {
        nb_bytes_read += read(file_fd, file->content, file->size - nb_bytes_read);
        if (nb_bytes_read < 0)
            handleErrorAndExit("read() failed in setRawFileContent()");
    }

    int return_value = close(file_fd);
    if (return_value < 0)
        handleErrorAndExit("close() failed in setRawFileContent()");

    // Update the file state and encoding
    file->state    = STATE_LOADED_RAW;
    file->encoding = ENCODING_NONE;
}

// By using standard program "gzip", compress the file
// File path and size must be set before calling this function!
void setCompressedFileContent (File* file)
{
    char* command = "gzip";
    char* execvp_argv[] = {
            "gzip",     // Command name (as argv[0])
            "--stdout", // Output compressed data on stdout
            file->path, // Path to the file
            NULL
    };

    // Buffer where to store compressed data, made larger than the normal file size,
    // in case the compressed data is bigger than the raw one
    // (TODO: magic 1.2 heuristic here, which should be improved!)
    int compression_buffer_length = (int) 1.2 * file->size;
    file->content = malloc(compression_buffer_length * sizeof(char));
    if (file->content == NULL)
        handleErrorAndExit("malloc() failed in setCompressedFileContent()");

    // get the compressed file content from 'gzip' output
    int compressed_data_length = runReadableProcess(command, execvp_argv,
                                                    file->content, compression_buffer_length);
    
    // Re-dimension the allocated buffer to fit the actual compressed data size
    file->content = realloc(file->content, compressed_data_length);
    file->size    = compressed_data_length;

    // Update the file state and encoding
    file->state    = STATE_LOADED_COMPRESSED;
    file->encoding = ENCODING_GZIP;
}

// Unload the content of a file
// Warnings are displayed in following cases:
// - if the file is in STATE_NOT_LOADED mode (does nothing)
// - if the content is NULL (does nothing)
void removeFileContent (File* file)
{
    if (file->state == STATE_NOT_LOADED)
    {
        printWarning("Warning: attempt to remove the content of a file in STATE_NOT_LOADED mode!");
        return;
    }

    if (file->content == NULL)
    {
        printWarning("Warning: attempt to remove the content of a file whose content is NULL");
        return;
    }

    free(file->content);
    file->size  = 0;
    file->state = STATE_NOT_LOADED;
}

// Set the file content, which can either be compressed or raw
// File path and size must be set before calling this function!
// If the file is too large to be cached, print a warning and return false
// Otherwise, return true
bool setFileContent (File* file, const int cache_free_space)
{
    // If the file size is too large, do not load it (and return false)
    if (file->size > cache_free_space)
    {
        printWarning("Note: file %s is too large to be cached!", file->path);
        return false;
    }

    // Otherwise, either load the raw content or the compressed one (and return true)
    if (file->size < MIN_FILE_SIZE_FOR_GZIP)
        setRawFileContent(file);
    else
        setCompressedFileContent(file);

    return true;
}

// Compute and set the required file metadata
void setFileMetadata (File* file)
{
    setFileType(file);
}

// -----------------------------------------------------------------------------
// CACHE BUILDING
// -----------------------------------------------------------------------------

// Return true if the file is "." (current) or ".." (parent)
bool filenameIsSpecial (const char* filename)
{
    return stringsAreEqual(filename,  ".")
        || stringsAreEqual(filename, ".."); 
}

// This function assumes directory is rewinded (otherwise, some elements may be ignored)
void countFilesAndFoldersInDirectory (DIR* directory, const char* current_folder_path,
                                      int* nb_files, int* nb_subfolders)
{
    struct stat file_info;
    char*       current_entry_path = malloc(MAX_PATH_LENGTH * sizeof(char));
    if (current_entry_path == NULL)
        handleErrorAndExit("malloc() failed in recursivelyFillFolder()");;

    struct dirent* current_entry = readdir(directory);
    while (current_entry != NULL)
    {
        char* current_entry_name = current_entry->d_name;

        // Ignore special directories "." (current) and ".." (parent)
        if (filenameIsSpecial(current_entry_name))
        {
            // Immediately move to the next entry
            current_entry = readdir(directory);
            continue;
        }

        // Build the path to the current entry
        current_entry_path  = strcpy(current_entry_path, current_folder_path);
        bool path_was_built = appendNameToPath(current_entry_path, current_entry_name, MAX_PATH_LENGTH);

        // If the path is too long (not handled for now), consider it a (fatal) error
        if (! path_was_built)
            handleErrorAndExit("appendNameToPath() failed in recursivelyFillFolder(): path is too long!");

        // Get info on the current entry
        int return_value = stat(current_entry_path, &file_info);
        if (return_value < 0)
            handleErrorAndExit("stat() failed in countFilesAndFoldersInDirectory()");

        // Check which kind of file the current entry is, 
        // and update counters accordingly
        if (S_ISREG(file_info.st_mode))
            (*nb_files)++;
        else if (S_ISDIR(file_info.st_mode))
            (*nb_subfolders)++;

        // Move to the next entry
        current_entry = readdir(directory);
    }

    free(current_entry_path);
}

// Fill a Folder structure according tot a DIR one, at current_path
// If there is no more space in the cache, file content is not loaded
void recursivelyFillFolder (DIR* directory, Folder* folder, const char* current_folder_path,
                            int* cache_free_space)
{
    struct stat file_info;
    char*       current_entry_path = malloc(MAX_PATH_LENGTH * sizeof(char));
    if (current_entry_path == NULL)
        handleErrorAndExit("malloc() failed in recursivelyFillFolder()");

    struct dirent* current_entry = readdir(directory);
    while (current_entry != NULL)
    {
        char* current_entry_name = current_entry->d_name;

        // Ignore special directories
        if (filenameIsSpecial(current_entry_name))
        {
            // Immediately move to the next entry
            current_entry = readdir(directory);
            continue;
        }

        // Build the path to the current entry
        current_entry_path  = strcpy(current_entry_path, current_folder_path);
        bool path_was_built = appendNameToPath(current_entry_path, current_entry_name, MAX_PATH_LENGTH);

        // If the path is too long (not handled for now), consider it a (fatal) error
        if (! path_was_built)
            handleErrorAndExit("appendNameToPath() failed in recursivelyFillFolder(): path is too long!");

        // Get info on the current entry
        int return_value = stat(current_entry_path, &file_info);
        if (return_value < 0)
            handleErrorAndExit("stat() failed in recursivelyBuildFolder()");

        // Case 1: current entry is a regular file
        if (S_ISREG(file_info.st_mode))
        {
            // Create and initialize a File structure
            // printf("Creating file with name %s...\n", current_entry_name);
            File* new_file = createAndInitFile();

            new_file->name = getFreshStringCopy(current_entry_name);
            new_file->path = getFreshStringCopy(current_entry_path);
            new_file->size = file_info.st_size;

            // File (content) must only be unloaded (after reading) if the cache is full
            new_file->must_unload = false;

            // Attempt to load the file content, and update values accordingly
            bool content_was_loaded = setFileContent(new_file, *cache_free_space);
            if (content_was_loaded)
                *cache_free_space -= new_file->size;
            else
                new_file->must_unload = true;

            // Set the file metadata
            setFileMetadata(new_file);

            // Finally, add the file to the folder
            addFileToFolder(folder, new_file);
        }

        // Case 2: current entry is a directory
        else if (S_ISDIR(file_info.st_mode))
        {
            // Recursively build the subfolder...
            Folder* new_subfolder = recursivelyBuildFolder(current_entry_path, cache_free_space);

            // ...and add it to the folder which is currently built
            addSubfolderToFolder(folder, new_subfolder);
        }

        // Move to the next entry
        current_entry = readdir(directory);
    }

    //free(current_entry_path);
}

Folder* recursivelyBuildFolder (const char* path, int* cache_free_space)
{
    // Open the pointed directory to browse it
    DIR* directory = opendir(path);
    if (directory == NULL)
        handleErrorAndExit("opendir() failed in recursivelyBuildFolder()");

    // Read the directory a first time, to count how many
    // folders and regular files are there inside
    int nb_files      = 0;
    int nb_subfolders = 0;
    countFilesAndFoldersInDirectory(directory, path, &nb_files, &nb_subfolders);

    // Create an empty Folder structure
    // printf("Creating new folder with %d file(s) and %d subfolder(s)...\n", nb_files, nb_subfolders);
    char*   new_folder_name = extractLastNameOfPath(path);
    Folder* new_folder      = createEmptyFolder(new_folder_name, nb_files, nb_subfolders);
    
    // Rewind the directory, and fill the Folder structure recursively
    rewinddir(directory);
    recursivelyFillFolder(directory, new_folder, path, cache_free_space);

    // Finally, close the directory
    int return_value = closedir(directory);
    if (return_value < 0)
        handleErrorAndExit("closedir() failed in recursivelyBuildFolder()");

    return new_folder;
}

FileCache* buildCacheFromDisk (char* root_path, const int max_size)
{
    // Create a fresh, empty file cache
    FileCache* new_cache = createEmptyFileCache(max_size);

    // Build the root folder recursively
    int cache_free_space = max_size;
    Folder* root_folder = recursivelyBuildFolder(root_path, &cache_free_space);

    // Set some cache fields
    new_cache->root = root_folder;
    new_cache->size = max_size - cache_free_space;

    return new_cache;
}

// -----------------------------------------------------------------------------
// FILE FETCHING
// -----------------------------------------------------------------------------

// Find a subfolder in a given folder, thanks to its name
// If not found, returns NOT_FOUND (NULL alias)
Folder* findSubfolderInFolder (const Folder* folder, const char* subfolder_name)
{
    for (int i = 0; i < folder->nb_subfolders; i++)
    {
        Folder* current_subfolder = folder->subfolders[i];
        if (stringsAreEqual(subfolder_name, current_subfolder->name))
            return current_subfolder;
    }

    return NOT_FOUND;
}

// Find a file in a given folder, thanks to its name
// If not found, returns NOT_FOUND (NULL alias)
File* findFileInFolder (const Folder* folder, const char* file_name)
{
    for (int i = 0; i < folder->nb_files; i++)
    {
        File* current_file = folder->files[i];
        if (stringsAreEqual(file_name, current_file->name))
            return current_file;
    }

    return NOT_FOUND;
}

// Find a file from a full path in a file cache
// If not found, returns NOT_FOUND (NULL alias)
// If path is longer than MAX_PATH_LENGTH, returns NOT_FOUND
// If path contains a folder or file name longer than MAX_NAME_LENGTH, returns NOT_FOUND
File* findFileInCache (const FileCache* cache, char* path)
{
    char    next_folder_name[MAX_NAME_LENGTH];
    Folder* current_folder = cache->root;

    // Read the remaining path until either '/' or '\0' is found
    // At most MAX_PATH_LENGTH characters can be read and tested
    char* remaining_path = path;

    // Start by ignoring all leading '/'
    while (remaining_path[0] == '/')
        remaining_path++;

    for (;;)
    {
        char* next_remaining_path = strchr(remaining_path, '/');

        // If no '/' was found, try to fetch the file in current folder
        if (next_remaining_path == NULL)
            return findFileInFolder(current_folder, remaining_path /* i.e. file name */);

        // Otherwise, get the current name to use
        // (it ranges from the first character to the last one before '/')
        int name_length = (next_remaining_path - remaining_path) / sizeof(char);
        memcpy(next_folder_name, remaining_path, name_length);
        next_folder_name[name_length] = '\0';

        // Try to move into the right subfolder
        current_folder = findSubfolderInFolder(current_folder, next_folder_name);
        if (current_folder == NOT_FOUND)
            return NOT_FOUND;

        // Ignore all leading '/' in the remaining path
        remaining_path = next_remaining_path;
        while (remaining_path[0] == '/')
            remaining_path++;
    }
}
