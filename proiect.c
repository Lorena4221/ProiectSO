#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX 10 //nu se vor putea introduce mai mult de MAX argumente in linia de comanda

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

    //inregistram momentul la  care a fost creata ultima metadata
    metadata.last_modified = file_stat.st_mtime;

    return metadata;
}

//functie pentru realizarea unei capturi a directorului dat ca argument
void takeSnapshot(char *directory, char *output_dir)
{
    char snapshot_path[256];

    snprintf(snapshot_path, sizeof(snapshot_path), "%s/snapshot.txt", output_dir);

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
    if(argc < 3 || MAX + 2) //verificam daca avem intre 3 si MAX+2 argumente in linia de comanda 
    {
        printf("Numar incorect de argumente.\n Folosiți: ./program -o director_ieșire director1 [director2 ...]\n");
        exit(EXIT_FAILURE);
    }

    char *output_dir = NULL;
    char *directories[MAX];
    int count_directories = 0;

    for (int i = 1; i < argc; i++) 
    {
        if (strcmp(argv[i], "-o") == 0) 
        {
            if (i + 1 >= argc) 
            {
                printf("Opțiunea -o necesită un director de ieșire.\n");
                exit(EXIT_FAILURE);
            }
            output_dir = argv[i + 1];
            i++; // Trecem peste directorul de ieșire pentru a nu îl procesa ca director de intrare
        } 
        else 
        {
            if (count_directories >= MAX) 
            {
                printf("Prea multe directoare. Maxim %d sunt permise.\n", MAX);
                exit(EXIT_FAILURE);
            }
            directories[count_directories++] = argv[i];
        }
    }

    if (output_dir == NULL) 
    {
        printf("Nu a fost specificat un director de ieșire.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < count_directories; i++) 
    {
        takeSnapshot(directories[i], output_dir);
        printf("Captura de la directorul %s a fost realizată cu succes.\n", directories[i]);
    }

    return 0;
}