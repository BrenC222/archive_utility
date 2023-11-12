#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h> // see man page look at stat struct

//--------------------------
// Data
//--------------------------

#define ARCH_KEY 0x222

struct header {
  char name[256];
  size_t size;
  // type of file? file or directory? maybe just check if directory else just a file, since some files arent classified as regular but technically are
  // link to directory? soft links? since name can change?
  // number of links? hard links? since its a directory?
};

//------------------
// Function Headers
//------------------
int archive_create(const char * archive_path, const unsigned int key);
int archive_valid(const char * archive_path);
int archive_add(const char * archive_path, const char * file_path);
int archive_list(const char * archive_path);
int archive_extract(const char * archive_path, const char * file_name, const char * dest_path);

//------------
// Main
//------------
int main(int argc, char * argv[]){
  // Usage information
  if (argc < 2){
    printf("Usage: %s <command> <archive> ...\n", argv[0]);
    printf("Try command \"-h\" to see options\n");
    exit(1);
  }
  if (argc == 2 && strcmp(argv[1], "-h") == 0){
    printf("Description of usage: COMMAND FORMAT\n");
    printf("[-h] Displays options: %s -h\n", argv[0]);
    printf("[-c] Creates recognizable archive file: %s -c <archive>\n", argv[0]);
    printf("[-a] Adds file to the archive: %s -a <archive> <file>\n", argv[0]);
    printf("[-l] Lists the files in the archive: %s -l <archive>\n", argv[0]);
    printf("[-e] Extracts file from archive: %s -e <archive> <archive_entry> <destination>\n", argv[0]);
    exit(0);
  }

  // Set archive path
  const char * arch_path = argv[2];

  // Control flow
  if (archive_valid(arch_path)){
    if (strcmp(argv[1], "-a") == 0 && argc == 4){
      archive_add(arch_path, argv[3]);
    }
    else if (strcmp(argv[1], "-l") == 0 && argc == 3){
      archive_list(arch_path);
    }
    else if (strcmp(argv[1], "-e") == 0 && argc == 5){
      archive_extract(arch_path, argv[3], argv[4]);
    }
    else {
      printf("Usage: %s <command> <archive> ...\n", argv[0]);
      printf("Try command [-h] to see options\n");
      exit(1);
    }
  }
  else if (strcmp(argv[1], "-c") == 0 && argc == 3){
    // Create command is special, since it will make the file if it doesn't exist
    // Funny enough once an archive is created it is impossible for this program to overwrite it by accident
    // This is because the file is checked if it's an archive first
    archive_create(arch_path, ARCH_KEY);
  }
  else {
    printf("Invalid archive\n");
  }// End control flow
  
  // Clean up
  return 0;
}// End main

//---------------------
// Function Definitions
//---------------------

//-------------------
// Validate archive
//--------------------
int archive_valid(const char * archive_path){
  // Open archive
  FILE * archive;
  if ((archive = fopen(archive_path, "rb")) == NULL){
    return 0;
  }

  // Validate
  unsigned int is_archive;
  fread(&is_archive, sizeof(unsigned int), 1, archive);
  if (is_archive != ARCH_KEY){
    fclose(archive);
    return 0;
  }

  // Clean up
  fclose(archive);
  return 1;
}// End archive_valid

//---------------
// Create archive
//---------------
int archive_create(const char * archive_path, const unsigned int key){
  // Open archive
  FILE * archive;
  if ((archive = fopen(archive_path, "wb")) == NULL){
    perror("idk");
  }

  // Write key NOTE!! This overwrites the given file
  fwrite(&key, sizeof(unsigned int), 1, archive);

  // Clean up
  fclose(archive);
  return 1;
}// End archive_create

//----------------------
// Add to archive
//----------------------
int archive_add(const char * archive_path, const char * file_path){
  // Open archive and entry files
  FILE * archive = fopen(archive_path, "ab");
  if (archive == NULL){
    perror("Error opening file to append STOOPID");
    return 0;
  }
  FILE * entry = fopen(file_path, "rb");
  if (entry == NULL){
    perror("Error opening file to read STOOPID");
    return 0;
  }
  
  // Init struct stat
  struct stat file_info;
  stat(file_path, &file_info);

  // Init struct header
  struct header * archive_entry;
  archive_entry = (struct header *)malloc(sizeof(struct header));
  strcpy(archive_entry->name, file_path);
  archive_entry->size = file_info.st_size;
  
  // Write archive_entry header to file
  if (fwrite(archive_entry, sizeof(struct header), 1, archive) != 1){
    perror("Error writing to file BOZO");
    fclose(entry);
    fclose(archive);
    free(archive_entry);
  }

  // Write file contents
  char read_byte;
  size_t bytes_written;
  while (fread(&read_byte, 1, 1, entry) > 0){
    if ((bytes_written = fwrite(&read_byte, 1, 1, archive)) != 1){
      perror("Error writing to file");
      fclose(entry);
      fclose(archive);
      free(archive_entry);
      return 0;
    }
  }

  // Clean up
  fclose(entry);
  fclose(archive);
  free(archive_entry);
  return 1;
}// End archive_add

//------------------
// List archive
//--------------------
int archive_list(const char * archive_path){
  // Open archive
  FILE * archive = fopen(archive_path, "rb");
  if (archive == NULL){
    perror("Error open file read GOOF");
    return 0;
  }
  fseek(archive, sizeof(unsigned int), SEEK_SET); // Skip ARCH_KEY

  // Init struct header
  struct header * archive_entry;
  archive_entry = (struct header *)malloc(sizeof(struct header));

  // Read file and load headers
  int count = 0;
  while(fread(archive_entry, sizeof(struct header), 1, archive) == 1){
    count++;
    printf("{\n");
    printf("Entry: %d, Name: %s, Size: %d", count, archive_entry->name, archive_entry->size);
    printf("\n}\n");

    fseek(archive, archive_entry->size, SEEK_CUR);
  }
  printf("Total files: %d\n", count);
  
  // Clean up
  fclose(archive);
  free(archive_entry);
  return 1;
}// End archive_list

//----------------------
// Extract archive
//-----------------------
int archive_extract(const char * archive_path, const char * file_name, const char * dest_path){
  // Open archive
  FILE * archive = fopen(archive_path, "rb");
  if (archive == NULL){
    perror("Error open file read KIDDO");
    return 0;
  }
  fseek(archive, sizeof(unsigned int), SEEK_SET); // Skip ARCH_KEY

  // Init struct header
  struct header * archive_entry;
  archive_entry = (struct header *)malloc(sizeof(struct header));

  // Look for file
  int found = 0;
  while (fread(archive_entry, sizeof(struct header), 1, archive) == 1){
    if (strcmp(archive_entry->name, file_name) == 0){
      found = 1;
      break;
    }
    fseek(archive, archive_entry->size, SEEK_CUR);
  }
  if(!found){
    perror("File not found in archive");
    fclose(archive);
    free(archive_entry);
    return 0;
  }

  // Read the file and write to destination
  FILE * dest_file = fopen(dest_path, "wb");
  if (dest_file == NULL){
    perror("Error open file write LOSER");
    fclose(archive);
    free(archive_entry);
    return 0;
  }
  char read_byte;
  size_t bytes_written = 0;
  while (bytes_written < archive_entry->size && (fread(&read_byte, 1, 1, archive) > 0)){
    if (fwrite(&read_byte, 1, 1, dest_file) != 1){
      perror("Error writing to file");
      fclose(archive);
      fclose(dest_file);
      free(archive_entry);
      return 0;
    }
    bytes_written++;
  }

  // Clean up
  fclose(archive);
  fclose(dest_file);
  free(archive_entry);
  return 1;
}// End archive_extract
