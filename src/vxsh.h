//
//  vxsh.h
//  vxsh
//
//  Copyright © 2017 Kacper Raczy. All rights reserved.
//

#ifndef vxsh_h
#define vxsh_h

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <fcntl.h>
#include <signal.h>

#define NONE_FILENO -1
#define FLAG_NEXT 0xf000
#define FLAG_FNAM 0x0f00
#define FLAG_PIPE 0x00f0
#define FLAG_BG   0x000f
#define FLAG_NONE 0x0000

typedef struct fd_fn {
    int fd;
    char* filename;
} fd_fn;

typedef struct shell_node {
    char** cmds;
    int cmdc;
    struct shell_node* next;
    fd_fn* in_ff;
    fd_fn* out_ff;
    fd_fn* err_ff;
    bool background;
} shell_node;

fd_fn* fd_fn_create(int fd, char* filename);
shell_node* empty_shell_node(void);
void print_node(shell_node* node);
void free_node(shell_node* node);

int vxsh_parse_str(shell_node* head, char* str);
void vxsh_exec_single(char** cmds, int cmdc);
void vxsh_exec_nodes(shell_node* head, shell_node* parent, int* parent_fd);
int vxsh_task_pid(void);

#endif /* vxsh_h */
