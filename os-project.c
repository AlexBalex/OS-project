#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
size_t byte_count(char *f)
{
  size_t count = 0;
  int c = 0;
  int len = strlen(f);
  while (c < len)
  {
    count++;
    c++;
  }

  return count;
}
size_t word_count(char *f)
{
  int i, len = strlen(f);
  size_t count = 0;
  char lastChar = ' ';

  for (i = 0; i < len; i++)
  {
    if (f[i] == ' ' || f[i] == '\n' || f[i] == '\t')
      if (lastChar != ' ' && lastChar != '\n' && lastChar != '\t')
      {
        count++;
      }
    lastChar = f[i];
  }

  if (lastChar == ' ' || lastChar == '\n' || lastChar == '\t')
  {
    count--;
  }

  return count + 1;
}
size_t newline_count(char *f)
{
  size_t count = 0;
  int c = 0;
  while (f[c] != '\0')
  {
    if (f[c] == '\n')
    {
      count++;
    }
    c++;
  }

  return count;
}
size_t max_width(char *f, int tab_width)
{
  int i, len = strlen(f), max_width = 0, width = 0;
  for (i = 0; i < len; i++)
  {
    if (f[i] == '\n')
    {
      if (width > max_width)
      {
        max_width = width;
      }
      width = 0;
    }
    else
    {
      if (f[i] == '\t')
      {
        width += tab_width - (width % tab_width);
      }
      else
      {
        width++;
      }
    }
  }
  if (width > max_width)
  {
    max_width = width;
  }

  return max_width;
}
char *expand_tabs(const char *input, int i, size_t tab_width)
{

  size_t input_length = strlen(input);
  size_t output_length = input_length + 3 * tab_width;
  char *output = (char *)malloc(sizeof(char) * output_length);

  size_t input_index = 0;
  size_t output_index = 0;
  size_t nonblanks = 0;
  if (i == 0)
  {
    while (input[input_index] != '\0')
    {
      if (input[input_index] == '\t')
      {

        size_t num_spaces = tab_width - (output_index % tab_width);
        for (size_t i = 0; i < num_spaces; i++)
        {
          output[output_index] = ' ';
          output_index++;
        }
      }
      else
      {

        output[output_index] = input[input_index];
        output_index++;
      }
      input_index++;
    }
  }
  else if (i == 1)
  {
    while (input[input_index] != '\0')
    {
      if (!isspace(input[input_index]))
      {
        nonblanks++;
      }
      if (input[input_index] == '\t')
      {
        if (nonblanks)
        {
          output[output_index] = input[input_index];
          output_index++;
        }
        else
        {
          size_t num_spaces = tab_width - (output_index % tab_width);
          for (size_t i = 0; i < num_spaces; i++)
          {
            output[output_index] = ' ';
            output_index++;
          }
        }
      }
      else
      {
        output[output_index] = input[input_index];
        output_index++;
      }
      input_index++;
    }
  }
  output[output_index] = '\0';

  return output;
}
int execute_command(char **args, int argnr)
{
  int input_fd = STDIN_FILENO;
  int output_fd = STDOUT_FILENO;
  char *input_file = NULL;
  char *output_file = NULL;
  int saved_stdout_fd = dup(STDOUT_FILENO);
  int saved_stdin_fd = dup(STDIN_FILENO);
  int in = 0;
  int out = 0;
  int append = 0;
  for (int i = 0; args[i] != NULL; i++)
  {
    if (strcmp(args[i], "<") == 0)
    {
      input_file = args[i + 1];
      input_fd = open(input_file, O_RDONLY);
      if (input_fd == -1)
      {
        perror("open");
        exit(1);
      }
      args[i] = NULL;
      in = 1;
    }
    else if (strcmp(args[i], ">") == 0)
    {
      output_file = args[i + 1];
      output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (output_fd == -1)
      {
        perror("open");
        exit(1);
      }
      args[i] = NULL;
      out = 1;
    }
    else if (strcmp(args[i], ">>") == 0)
    {
      output_file = args[i + 1];
      output_fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
      if (output_fd == -1)
      {
        perror("open");
        exit(1);
      }
      args[i] = NULL;
      append = 1;
    }
  }
  if (input_fd != STDIN_FILENO)
  {
    if (dup2(input_fd, STDIN_FILENO) == -1)
    {
      perror("dup2 error");
      exit(1);
    }
    close(input_fd);
  }
  if (output_fd != STDOUT_FILENO)
  {
    if (dup2(output_fd, STDOUT_FILENO) == -1)
    {
      perror("dup2 error");
      exit(1);
    }
    close(output_fd);
  }
  if (strcmp(args[0], "exit") == 0)
  {
    exit(0);
  }
  // env command
  else if (strcmp(args[0], "env") == 0)
  {
    int i, j = 1;
    for (i = 1; args[i] != NULL; i++)
    {
      if (strcmp(args[i], "-u") == 0)
      {
        if (args[i + 1] != NULL)
          unsetenv(args[i + 1]);
        else
        {
          printf("You need to specify an argument for -u\n");
          j = 0;
        }
      }
      else
      {
        break;
      }
    }
    if (j == 1)
    {
      extern char **environ;
      for (char **eLine = environ; *eLine != NULL; eLine++)
      {

        printf("%s\n", *eLine);
      }
    }
  }
  // expand command
  else if (strcmp(args[0], "expand") == 0)
  {
    int t = 0, i = 0, tab_width = 8, opt, j;
    int frominput = 0;
    for (j = 1; args[j] != NULL; j++)
    {
      if (strcmp(args[j], "-t") == 0 || strncmp(args[j], "-t", 2) == 0)
      {
        t = 1;
        if (args[j + 1] != NULL && isdigit(*args[j + 1]))
        {
          tab_width = atoi(args[j + 1]);
          j++;
        }
        else if (strncmp(args[j], "-t", 2) == 0)
        {
          const char *arg = args[j] + 2;
          if (*arg && isdigit(*arg))
          {
            tab_width = atoi(arg);
          }
          else
          {
            printf("Invalid argument for -t\n");
            return 1;
          }
        }
        else
        {
          printf("Invalid argument for -t\n");
          return 1;
        }
      }
      else if (strcmp(args[j], "-i") == 0)
      {
        i = 1;
      }
      else
      {
        break;
      }
    }

    FILE *file;
    if (args[j] == NULL || strcmp(args[j], "-") == 0)
    {
      frominput = 1;
      file = fdopen(STDIN_FILENO, "r");
    }
    else
    {
      file = fopen(args[j], "r");
    }
    if (file == NULL)
    {
      printf("%s:File does not exist\n",args[j]);
      return 1;
    }
    char *line = NULL;
    size_t line_size = 0;
    while (getline(&line, &line_size, file) != -1)
    {
      char *expanded = expand_tabs(line, i, tab_width);
      printf("%s", expanded);
      free(expanded);
    }
    free(line);
    if (frominput == 0)
      fclose(file);
  }
  // wc command
  else if (strcmp(args[0], "wc") == 0)
  {
    char *input_string;
    int size = 0;
    char character;
    int i = 0, j, frominput = 0;
    int c = 0, w = 0, l = 0, L = 0;
    int a = 0, k, total = 0;
    char *files[100];
    int wordsT = 0, linesT = 0, bytesT = 0, maxWidthMax = 0;
    int noFiles = 0;
    if (argnr > 1)
    {
      for (i = 1; args[i] != NULL; i++)
      {
        if (strcmp(args[i], "-c") == 0)
        {
          c = 1;
        }
        else if (strcmp(args[i], "-w") == 0)
        {
          w = 1;
        }
        else if (strcmp(args[i], "-l") == 0)
        {
          l = 1;
        }
        else if (strcmp(args[i], "-L") == 0)
        {
          L = 1;
        }
        else if (strcmp(args[i], "-") == 0)
        {
          noFiles = 1;
          total = 1;
          break;
        }
        else if (strcmp(args[i], ">") != 0 && strcmp(args[i], ">>") != 0 && strcmp(args[i], "<") != 0)
        {
          if (access(args[i], F_OK) != -1 && strcmp(args[i], ".") != 0)
          {
            files[total] = args[i];
            total++;
          }
          else
          {
            printf("%s:File does not exist\n",args[i]);
            return 1;
          }
        }
        else
        {
          break;
        }
      }
    }
    if (total == 0)
    {
      noFiles = 1;
      total = 1;
    }
    FILE *file;
    for (k = 0; k < total; k++)
    {
      {
        if (noFiles == 1)
        {
          file = fdopen(STDIN_FILENO, "r");
          frominput = 1;
        }
        else
          file = fopen(files[k], "r");
        if (file == NULL)
        {
          fprintf(stderr, "Error: Unable to open  file\n");
          return 1;
        }
        size = 4096;
        j = 0;
        input_string = malloc(size);
        while ((character = fgetc(file)) != EOF)
        {
          input_string[j] = character;
          j++;
        }
        input_string[j] = '\0';
      }

      size_t words = 0;
      long bytes = 0;
      size_t lines = 0;
      long maxWidth = 0;
      if (l == 0 && w == 0 && c == 0 && L == 0)
      {
        lines = newline_count(input_string);
        words = word_count(input_string);
        bytes = byte_count(input_string);
        linesT += lines;
        wordsT += words;
        bytesT += bytes;
        if (frominput == 0)
          printf("%lu %lu %ld %s\n", lines, words, bytes, files[k]);
        else
          printf("%lu %lu %ld\n", lines, words, bytes);
      }
      else
      {
        if (l == 1)
        {
          lines = newline_count(input_string);
          linesT += lines;
          printf("%lu ", lines);
          a = 1;
        }
        if (w == 1)
        {
          words = word_count(input_string);
          wordsT += words;
          printf("%lu ", words);
          a = 1;
        }
        if (c == 1)
        {
          bytes = byte_count(input_string);
          bytesT += bytes;
          printf("%ld ", bytes);
          a = 1;
        }
        if (L == 1)
        {
          maxWidth = max_width(input_string, 8);
          if (maxWidth > maxWidthMax)
            maxWidthMax = maxWidth;
          printf("%ld ", maxWidth);
          a = 1;
        }
        printf("%s\n", files[k]);
      }
      fclose(file);
    }
    free(input_string);
    if (total > 1)
    {
      if (a == 1)
      {
        if (l == 1)
          printf("%d ", linesT);
        if (w == 1)
          printf("%d ", wordsT);
        if (c == 1)
          printf("%d ", bytesT);
        if (L == 1)
          printf("%d ", maxWidthMax);
        printf("total\n");
      }
      else
      {
        printf("%d %d %d total\n", linesT, wordsT, bytesT);
      }
    }
  }
  // help
  else if (strcmp(args[0], "help") == 0)
  {
    printf("Self implemented commands you can use:\n");
    printf("env with the following flags:    -u\n");
    printf("expand with the following flags: -t -i\n");
    printf("wc with the following flags:     -c -w -l -L\n");
    printf("All the other commands are handled by the UNIX shell\n");
    printf("\nIf you want to exit the shell you can type exit\n");
  }
  else
  {
    pid_t pid = fork();
    if (pid < 0)
    {
      perror("error when forking");
    }
    else if (pid == 0)
    {

      execvp(args[0], args);
      perror(args[0]);
      exit(1);
    }
    else
    {
      int status;
      if (waitpid(pid, &status, 0) == -1)
      {
        perror("waitpid");
      }
      if (WIFEXITED(status))
      {
        if (WEXITSTATUS(status) == 1)
        {
          fprintf(stderr, "Command failed: %s\n", args[0]);
        }
      }

      if (input_fd != STDIN_FILENO)
      {
        close(input_fd);
      }
      if (output_fd != STDOUT_FILENO)
      {
        close(output_fd);
      }
    }
  }
  dup2(saved_stdout_fd, STDOUT_FILENO);
  close(saved_stdout_fd);
  dup2(saved_stdin_fd, STDIN_FILENO);
  close(saved_stdin_fd);
  return 1;
}
char **split_line(char *line)
{
  int result;
  char *expanded_line;
  result = history_expand(line, &expanded_line);
  if (result)
  {
    fprintf(stderr, "%s\n", expanded_line);
  }

  int buf_size = 64, position = 0;
  char **tokens = malloc(buf_size * sizeof(char *));
  char *token;

  if (!tokens)
  {
    fprintf(stderr, "Error allocating enough memory\n");
    exit(1);
  }

  token = strtok(line, " \t\r\n\a");
  while (token != NULL)
  {
    tokens[position] = token;
    position++;

    if (position >= buf_size)
    {
      buf_size += 64;
      tokens = realloc(tokens, buf_size * sizeof(char *));
      if (!tokens)
      {
        fprintf(stderr, "Error allocating enough memory\n");
        exit(1);
      }
    }

    token = strtok(NULL, " \t\r\n\a");
  }
  tokens[position] = NULL;
  free(expanded_line);
  return tokens;
}

void loop(void)
{
  char *line;
  char **args;
  int status;
  int argnr = 0;
  char c[1024];
  do
  {
    printf("\033[0;35m");
    printf("%s", getenv("USER"));
    printf("\033[0;36m");
    printf(":~%s\n", getcwd(c, sizeof(c)));
    printf("\033[0m");
    line = readline("> ");
    add_history(line);
    args = split_line(line);
    while (args[argnr] != NULL)
      argnr++;
    status = execute_command(args, argnr);
    free(line);
    free(args);

  } while (status);
}

int main(int argc, char **argv)
{
  system("clear");
  printf("Welcome to Bashell,hope you have fun :D\n");
  printf("(Type help to see what self implemented commands you can use)\n");
  loop();
  return 0;
}