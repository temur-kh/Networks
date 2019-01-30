#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#define BUFFER_LIMIT 20
#define SLEEP_MILLISECONDS 100000

struct stack {
  int capacity;
  int *data;
  int size;
};

int empty(struct stack* stk) {
  return stk->size == 0 ? 1 : 0;
}

int peek(struct stack* stk) {
  if (!empty(stk)) {
    return stk->data[stk->size - 1];
  } else {
    printf("Could not peek an element! The stack is empty!\n");
    return INT_MIN;
  }
}

int pop(struct stack* stk) {
  if (!empty(stk)) {
    if (4 * stk->size < stk->capacity) {
      // shrink the data structure
      stk->data = (int *) realloc(stk->data, (stk->capacity / 2) * sizeof(int));
    }
    return stk->data[--stk->size];
  } else {
    printf("Could not pop an element! The stack is empty!\n");
    return INT_MIN;
  }
}

void push(struct stack* stk, int data) {
  if (stk->size == stk->capacity) {
    // enlarge the data structure
    stk->data = (int *) realloc(stk->data, (stk->capacity * 2) * sizeof(int));
  }
  stk->data[stk->size++] = data;
  printf("New element was added to the stack\n");
}

void stack_size(struct stack* stk) {
  printf("The size of the stack: %d\n", stk->size);
}

struct stack* create() {
  struct stack* new_stk = malloc(sizeof(struct stack));
  new_stk->size = 0;
  new_stk->capacity = 2;
  new_stk->data = (int *) malloc(new_stk->capacity * sizeof(int));
  return new_stk;
}

void display(struct stack* stk) {
  if (!stk->size) {
    printf("The stack is empty!\n");
    return;
  }
  int i;
  printf("Elements of the stack:");
  for (i = 0; i < stk->size; i++) {
    printf(" %d", stk->data[i]);
  }
  printf("\n");
}

char** split(char* str, int n) {
  char** words = malloc(n * sizeof(char*));
  int size = strlen(str);
  char splitter[] = " ";

  str[strcspn(str, "\n")] = 0;
  char *ptr = strtok(str, splitter);

  int i = 0;
  while(ptr != NULL) {
    if (i >= n) {
      return NULL;
    }
    words[i] = ptr;
    ptr = strtok(NULL, splitter);
    i++;
  }
  return words;
}

int isNumeric(char *num) {
    char* str = num;
    while(*str != '\0') {
        if(*str < '0' || *str > '9')
            return 0;
        str++;
    }
    return 1;
}

int main() {
  int fds[2];
  pipe(fds);
  printf("A client-server program.\nList of available commands:\n");
  printf("create - creates new stack (at the start there is already a stack initialized)\n");
  printf("push x - push data x to the stack (x - integer)\n");
  printf("peek - output the top element from the stack if exists\n");
  printf("pop - output and remove the top element from the stack if exists\n");
  printf("empty - True if the stack is empty, otherwise False\n");
  printf("stack_size - output the size of the stack\n");
  printf("display - output the list of elements from the stack\n");
  printf("exit - terminate the program\n");
  int pid = fork();
  if (pid > 0) {
    // parent process - client
    while (1) {
      char *str = malloc(BUFFER_LIMIT * sizeof(char));
      close(fds[0]);
      usleep(SLEEP_MILLISECONDS);
      printf(">>> ");
      fgets(str, BUFFER_LIMIT, stdin);
      write(fds[1], str, strlen(str));
      printf("The client has sent a message to the server\n");
      if (!strcmp(str, "exit\n")) {
        printf("Client is terminated!\n");
        return EXIT_SUCCESS;
      }
      free(str);
    }
  } else if (pid == 0) {
    // child process - server
    struct stack* stk = create();
    while (1) {
      char *str = malloc(BUFFER_LIMIT * sizeof(char));
      close(fds[1]);
      read(fds[0], str, BUFFER_LIMIT);
      char** words = split(str, 2);

      if (words == NULL) {
        printf("Wrong format of input! Try again!\n");
        free(str);
        continue;
      }

      if (!strcmp(str, "exit")) {
        printf("Server is terminated!\n");
        return EXIT_SUCCESS;
      } else if (!strcmp(words[0], "peek")) {
        int data = peek(stk);
        if (data != INT_MIN) {
          printf("Peeked element: %d\n", data);
        }
      } else if (!strcmp(words[0], "push") && words[1] != NULL && isNumeric(words[1])) {
        int data = strtol(words[1], NULL, 10);
        push(stk, data);
      } else if (!strcmp(words[0], "pop")) {
        int data = pop(stk);
        if (data != INT_MIN) {
          printf("Popped element: %d\n", data);
        }
      } else if (!strcmp(words[0], "empty")) {
        printf("Is the stack empty: %s\n", (empty(stk) ? "True" : "False"));
      } else if (!strcmp(words[0], "display")) {
        display(stk);
      } else if (!strcmp(words[0], "create")) {
        free(stk->data);
        free(stk);
        stk = create();
        printf("A new stack was created!\n");
      } else if (!strcmp(words[0], "stack_size")) {
        stack_size(stk);
      } else {
        printf("Wrong format of input! Try again!\n");
        free(str);
        continue;
      }
      
      free(str);
    }
  } else {
    // error
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
