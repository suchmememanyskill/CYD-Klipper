/*
At some point it may be a fun challenge to try to implement a virtual folder structure, but not today.

typedef struct _FILESYSTEM_FILE {
    char* name;
    char* parent_folder_name;
    long level;
} FILESYSTEM_FILE;

typedef struct _FILESYSTEM_FOLDER {
    char** files;
    char* folder_path;
    FILESYSTEM_FOLDER* folders;
} FILESYSTEM_FOLDER;
*/

typedef struct _FILESYSTEM_FILE {
    char* name;
    float modified;
} FILESYSTEM_FILE;

FILESYSTEM_FILE* get_files(int limit);
void clear_files();