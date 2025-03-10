#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAP_SIZE 1048576

void removeVowels(char *str) {
  const char *vowels = "aeiouAEIOU";
  int len = strlen(str);
  int j = 0;
  for (int i = 0; i < len; i++) {
    if (!strchr(vowels, str[i])) {
      str[j++] = str[i];
    }
  }
  str[j] = '\0';
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: child2 <read_fd> <filename>\n");
    return 1;
  }

  int readFd = atoi(argv[1]);
  char *filename = argv[2];

  dup2(readFd, STDIN_FILENO);
  close(readFd);

  int fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd < 0) {
    perror("open child2");
    return 1;
  }

  if (ftruncate(fd, MAP_SIZE) == -1) {
    perror("ftruncate child2");
    close(fd);
    return 1;
  }

  void *mapPtr =
      mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (mapPtr == MAP_FAILED) {
    perror("mmap child2");
    close(fd);
    return 1;
  }

  FILE *fin = fdopen(STDIN_FILENO, "r");
  if (!fin) {
    perror("fdopen stdin child2");
    munmap(mapPtr, MAP_SIZE);
    close(fd);
    return 1;
  }

  size_t currentOffset = 0;
  char buffer[1024];

  while (fgets(buffer, sizeof(buffer), fin)) {
    removeVowels(buffer);
    size_t len = strlen(buffer);

    if (len > 0) {
      if (currentOffset + len > MAP_SIZE) {
        fprintf(stderr, "Не хватает места в mmap для записи.\n");
        break;
      }
      memcpy((char *)mapPtr + currentOffset, buffer, len);
      currentOffset += len;
    }
  }

  fclose(fin);
  munmap(mapPtr, MAP_SIZE);
  close(fd);

  return 0;
}
