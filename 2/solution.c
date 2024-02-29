#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "parser.h"

#define BUF_SIZE 4096

static void execute_command(const struct expr *e) {
  if (e->cmd.exe == NULL) {
    printf("Error: No command specified\n");
    return;
  }

  if (strcmp(e->cmd.exe, "cd") == 0) {
    if (e->cmd.arg_count != 1) {
      printf("Usage: cd <directory>\n");
    }

    if (chdir(e->cmd.args[0]) == -1) {
      perror("Error");
    }
    return;
  }

  if (strcmp(e->cmd.exe, "echo") == 0) {
    for (uint32_t i = 0; i < e->cmd.arg_count; ++i) {
      printf("%s ", e->cmd.args[i]);
    }
    printf("\n");
    return;
  }

  if (strcmp(e->cmd.exe, "mkdir") == 0) {
    for (uint32_t i = 0; i < e->cmd.arg_count; ++i) {
      if (mkdir(e->cmd.args[i], 0777) == -1) {
        perror("Error");
      }
    }
    return;
  }

  if (strcmp(e->cmd.exe, "cat") == 0) {
    if (e->cmd.arg_count < 1) {
      printf("Usage: cat <file1> <file2> ... <fileN>\n");
      return;
    }
    for (uint32_t i = 0; i < e->cmd.arg_count; i++) {
      int fd = open(e->cmd.args[i], O_RDONLY);
      if (fd == -1) {
        perror("Error opening file");
        continue;
      }
      char buf[BUF_SIZE];
      ssize_t bytes_read;
      while ((bytes_read = read(fd, buf, BUF_SIZE)) > 0) {
        if (write(STDOUT_FILENO, buf, bytes_read) != bytes_read) {
          perror("Error writing to stdout");
          close(fd);
          return;
        }
      }
      if (bytes_read == -1) {
        perror("Error reading file");
      }
      close(fd);
    }
    return;
  }

  if (strcmp(e->cmd.exe, "touch") == 0) {
    for (uint32_t i = 0; i < e->cmd.arg_count; i++) {
      int fd = open(e->cmd.args[i], O_CREAT | O_WRONLY, 0644);
      if (fd == -1) {
        perror("Error opening file");
        continue;
      }
      if (close(fd) == -1) {
        perror("Error closing file");
      }
    }
    return;
  }

  pid_t pid = fork();

  if (pid == -1) {
    perror("fork");
    return;
  }

  if (pid == 0) {
    if (execvp(e->cmd.exe, e->cmd.args) == -1) {
      perror("Error");
      _exit(1);
    }
  } else {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
      perror("waitpid");
      return;
    }
    if (!WIFEXITED(status)) {
      printf("Child process terminated abnormally\n");
    }
  }
}

static void execute_command_line(const struct command_line *line) {
  /* REPLACE THIS CODE WITH ACTUAL COMMAND EXECUTION */

  assert(line != NULL);
  const struct expr *e = line->head;
  while (e != NULL) {
    if (e->type == EXPR_TYPE_COMMAND) {
      execute_command(e);
    } else if (e->type == EXPR_TYPE_PIPE) {
      printf("\tPIPE\n");
    } else if (e->type == EXPR_TYPE_AND) {
      printf("\tAND\n");
    } else if (e->type == EXPR_TYPE_OR) {
      printf("\tOR\n");
    } else {
      assert(false);
    }
    e = e->next;
  }
}

int main(void) {
  const size_t buf_size = 1024;
  char buf[buf_size];
  int rc;
  struct parser *p = parser_new();
  while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
    parser_feed(p, buf, rc);
    struct command_line *line = NULL;
    while (true) {
      enum parser_error err = parser_pop_next(p, &line);
      if (err == PARSER_ERR_NONE && line == NULL) break;
      if (err != PARSER_ERR_NONE) {
        printf("Error: %d\n", (int)err);
        continue;
      }
      execute_command_line(line);
      command_line_delete(line);
    }
  }
  parser_delete(p);
  return 0;
}
