#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//structura pentru a stoca metadatele unei intrari din director
typedef struct 
{
    char name[256]; //numele intrarii
    time_t last_modified;//timpul ultimei modificari
} Metadata;

//functie pentru a obtine metadatele unei intrari din director 
Metadata getMetadata(const char *path) 
{
    Metadata metadata;
    struct stat file_stat;
    strcpy(metadata.name, path);

    if (stat(path, &file_stat) == -1) {
        perror("Error getting file metadata\n");
        exit(EXIT_FAILURE);
    }

    metadata.last_modified = file_stat.st_mtime;

    return metadata;
}

//functie pentru realizarea unei capturi a directorului dat ca argument
void takeSnapshot(char *directory)
{
    int snapshot_fd = open("snapshot.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (snapshot_fd== -1)
    {
        printf("nu s-a putut crea snapshot file\n");
        exit(EXIT_FAILURE);
    }

    DIR *dir = opendir(directory);
    struct dirent *entry;

    if (dir == NULL) {
        perror("nu s-a putut deschide directorul\n");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
        {
            char entry_path[256];
            snprintf(entry_path, sizeof(entry_path), "%s/%s", directory, entry->d_name);
            Metadata metadata = getMetadata(entry_path);
            dprintf(snapshot_fd, "%s %ld\n", metadata.name, metadata.last_modified);
        }
    }

    closedir(dir);
    close(snapshot_fd);
}

int main(int argc, char* argv[])
{
    if(argc != 2) //verificam daca avem exact 2 argumente in linia de comanda 
    {
        printf("numar incorect de argumente\n");
        exit(EXIT_FAILURE);
    }

    char *directory = argv[1];
    takeSnapshot(directory);

    printf("S-a realizat captura de fisier\n");
    return 0;
}