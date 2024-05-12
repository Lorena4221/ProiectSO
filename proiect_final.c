#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h> // For waitpid()
#include <libgen.h>

#define MAX_PATH_LENGTH 1024
#define MAX_METADATA_LENGTH 1024
int checkPermission(const char *);
int checkForMaliciousFiles(const char *);
// Structure to store metadata of a directory entry
typedef struct 
{
    char name[256]; // Entry name
    time_t last_modified; // Last modified timestamp
} Metadata;

// Function to get metadata of a directory entry
Metadata getMetadata(const char *path) 
{
    Metadata metadata;
    struct stat file_stat;
    strcpy(metadata.name, path);

    if (stat(path, &file_stat) == -1) 
    {
        perror("Error getting file metadata\n");
        exit(EXIT_FAILURE);
    }

    metadata.last_modified = file_stat.st_mtime;

    return metadata;
}

// Function to create snapshot for a given directory
void createSnapshotForDirectory(char *directory_path,char *output_dir, int index) 
{
    DIR *dir;
    struct dirent *entry;
    char snapshot_path[MAX_PATH_LENGTH];
    
    // Open directory
    if ((dir = opendir(directory_path)) == NULL) 
    {
        printf("Failed to open directory: %s\n", directory_path);
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    // Create snapshot file path in the output directory
    snprintf(snapshot_path, MAX_PATH_LENGTH, "%s/Snapshot.txt", output_dir);
    int snapshot_fd = open(snapshot_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (snapshot_fd == -1) 
    {
        printf("Failed to create snapshot file: %s\n", snapshot_path);
        closedir(dir);
        close(snapshot_fd);
        exit(EXIT_FAILURE);
    }

    // Iterate through directory entries
    while ((entry = readdir(dir)) != NULL) 
    {
        // Skip "." and ".." directories
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
        {
            char* source_path = (char*)malloc(256);
            strcpy(source_path, directory_path);
            strcat(source_path, "/");
            strcat(source_path, entry->d_name);
            char* dest_path = (char*)malloc(256);
            strcpy(dest_path, "IsolatedSpace/");
            strcat(dest_path, entry->d_name);
            
            int check_permission_pipe[2];

            if(pipe(check_permission_pipe) == -1)
            {
                perror("Error creating pipe");
                free(source_path);
                free(dest_path);
                exit(EXIT_FAILURE);
            }

            pid_t child_proces = fork();
            char response_from_child[256];

            if (child_proces < 0) 
            {
                perror("Error forking process");
                free(source_path);
                free(dest_path);
                exit(EXIT_FAILURE);
            } 
            else if (child_proces == 0) 
            {
                close(check_permission_pipe[0]);
                if (checkPermission(source_path)) 
                {
                    if (checkForMaliciousFiles(source_path)) 
                    {
                        write(check_permission_pipe[1], source_path, strlen(source_path));
                    } else{
                        write(check_permission_pipe[1], "SAFE\0", 5);
                    }
                } else{
                    write(check_permission_pipe[1], "SAFE\0", 5);
                }
            }else{
                waitpid(child_proces, NULL, 0);
                close(check_permission_pipe[1]);
                int bytes_read = read(check_permission_pipe[0], response_from_child, 256);
                if (bytes_read == -1) 
                {
                    perror("Error reading from pipe");
                    free(source_path);  
                    free(dest_path);
                    exit(EXIT_FAILURE);
                }
                if (strcmp(response_from_child, "SAFE") == 0) 
                {
                    printf("File %s is safe\n", source_path);
                    free(source_path);
                    free(dest_path);
                    continue;
                } 
                else 
                {
                    printf("File %s is not safe\n", source_path);
                     if (rename(source_path, dest_path) != 0) {
                        perror("Error moving file");
                        exit(EXIT_FAILURE);
                    }
                    return;
                }
            }

            char entry_path[MAX_PATH_LENGTH];
            snprintf(entry_path, sizeof(entry_path), "%s/%s", directory_path, entry->d_name);
            
            struct stat file_stat;
            if (lstat(entry_path, &file_stat) == -1) 
            {
                printf("Error getting file metadata\n");
                closedir(dir);
                close(snapshot_fd);
                exit(EXIT_FAILURE);
            }

            char entry_info[MAX_METADATA_LENGTH];
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);

            sprintf(entry_info, "Marca Temporala: %d-%02d-%02d %02d:%02d:%02d\nIntrare: %s\nDimensiune: %lld octeti\nUltima Modificare: %sPermisiuni: %d\nNumar Inode:%lld\n\n", 
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, entry->d_name, (long long)file_stat.st_size, ctime(&file_stat.st_ctime), file_stat.st_mode & 0777, (long long)file_stat.st_ino);

            // Write entry info to snapshot file
            if (write(snapshot_fd, entry_info, strlen(entry_info)) == -1) 
            {
                printf("Error writing to snapshot file\n");
                closedir(dir);
                close(snapshot_fd);
                exit(EXIT_FAILURE);
            }
        }
    }
    
    // Close directory and snapshot file
    closedir(dir);
    close(snapshot_fd);

    printf("Snapshot for Directory %d created successfully.\n", index);
}

// Function to create snapshots for all directories in parallel child processes
void createSnapshots(int num_directories, char *directories[], char *output_dir) 
{
    pid_t pid;
    int status;

    for (int i = 0; i < num_directories; i++) 
    {
        pid = fork(); // Create child process
        
        if (pid < 0) 
        {
            printf("Error forking process\n");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) // Child process
        {   
            createSnapshotForDirectory(directories[i], output_dir, i + 1);
            exit(EXIT_SUCCESS);
        }
    }
    
    // Parent process waits for all child processes to complete
    for (int i = 0; i < num_directories; i++) 
    {
        pid_t child_pid = wait(&status);
        printf("Child Process %d terminated with PID %d and exit code %d.\n", i + 1, child_pid, WEXITSTATUS(status));
    }
}

int checkPermission(const char *path)
{
    struct stat file_stat;
    if (stat(path, &file_stat) == -1) 
    {
        perror("Error getting file metadata in checkPermisions\n");
        exit(EXIT_FAILURE);
    }
    // Check if the file lacks read, write, and execute permissions
    if ((file_stat.st_mode & S_IRWXU) == 0)
    {
        return 1; // Permissions missing
    }
    return 0; // Sufficient permissions
}

int checkForMaliciousFiles(const char *file_path) 
{
    
    pid_t pid = fork(); // Create a new process
    
    if (pid < 0) 
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) 
    {
        // Child process
        execl("./verify_for_malicious.sh", "./verify_for_malicious.sh", file_path, NULL);
        // If execl returns, it means it failed
        perror("execl failed");
        exit(EXIT_FAILURE);
    } 
    else 
    {
        // Parent process
        int status;
        waitpid(pid, &status, 0); // Wait for the child process to finish
        // Check if the child process exited normally
        if (WIFEXITED(status)) 
        {
            // Extract the exit status of the child process
            int exit_status = WEXITSTATUS(status);
            // Set global variable based on the exit status
            if (exit_status == 0) 
            {
                printf("File %s is not malicious.\n", file_path);
                return 0;
            } 
            else 
            {
                printf("File %s is malicious.\n", file_path);
                return 1;
            }
        } 
        else 
        {
            perror("Child process did not exit normally");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char *argv[]) 
{
    // Check if directory arguments are provided
    if (argc < 5 || argc > 14) 
    {
        printf("Usage: %s <directory1> <directory2> ... <directory10> [-o output_directory]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Parse command line arguments
    char *output_dir = ".";
    int num_directories = argc - 1;
    if (strcmp(argv[argc - 4], "-o") == 0 && argc >= 6 && strcmp(argv[argc-2], "-s")==0) 
    {
        output_dir = argv[argc - 3];
        num_directories -= 4;
    } 
    else 
    {
        printf("Error: Output directory not specified\n");
        exit(EXIT_FAILURE);
    }

    // Get directory paths
    char *directories[num_directories];
    printf("Directories: %d\n", num_directories);
    for (int i = 1; i <= num_directories; i++) 
    {
        directories[i - 1] = malloc(strlen(argv[i]) + 1);
        strcpy(directories[i - 1], argv[i]);
    }

    // Create snapshots for all directories
    createSnapshots(num_directories, directories, output_dir);

    return 0;
}
