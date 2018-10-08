//
//  vxsh.c
//  vxsh
//
//  Copyright © 2017 Kacper Raczy. All rights reserved.
//

#include "vxsh.h"

static int task_pid = 0;
static const char special_chr[6] = {' ', '|', '<', '>', '^', '&'};

// MARK: utility

int vxsh_task_pid(void) {
    return task_pid;
}

char* home_dir() {
    struct passwd *pw = getpwuid(getuid());
    char* homedir = pw->pw_dir;
    return homedir;
}

bool special_contains(char str) {
    bool b = false;
    for (int i = 0; i < 6; i++) {
        if (special_chr[i] == str) {
            b = true;
            break;
        }
    }
    
    return b;
}

// MARK: struct related operations

fd_fn* fd_fn_create(int fd, char* filename) {
    fd_fn* f = malloc(sizeof(fd_fn));
    f->fd = fd;
    f->filename = filename;
    return f;
}

shell_node* empty_shell_node() {
    shell_node* node = malloc(sizeof(shell_node));
    node->cmdc = 0;
    node->cmds = NULL;
    node->next = NULL;
    node->background = false;
    node->in_ff = NULL;
    node->out_ff = NULL;
    node->err_ff = NULL;
    
    return node;
}

void print_node(shell_node* node) {
    char dest[128] = "";
    for (int i = 0; i < node->cmdc; i++) {
        strcat(dest, " ");
        strcat(dest, *(node->cmds + i));
    }
    printf("node - cmdc: %d\n\tcmds: %s\n", node->cmdc, dest);
}

void free_node(shell_node* node) {
    shell_node* current = node;
    shell_node* temp;
    while (current != NULL) {
        temp = current;
        current = current->next;
        free(temp);
    }
}

// MARK: parse

int vxsh_parse_str(shell_node* head, char* str) {
    char c;
    int pos = 0;
    int prev_pos = 0;
    shell_node* current = head;
    
    // flags
    int fileno = NONE_FILENO;
    short flags = FLAG_NONE;
    
    while ((c = str[pos]) != '\0') {
        if (special_contains(c)) {
            int len = (pos - prev_pos);
            if (c == ' ') {
                flags |= FLAG_NEXT;
            } else if (c == '|') {
                flags |= FLAG_PIPE;
            } else if (c == '&') {
                flags |= FLAG_BG;
            } else if (c == '>' || c == '<' || c == '^') {
                if (c == '<') {
                    fileno = STDIN_FILENO;
                } else if (c == '>') {
                    fileno = STDOUT_FILENO;
                } else if (c == '^') {
                    fileno = STDERR_FILENO;
                }
                flags |= FLAG_FNAM;
            }
            
            if (flags & FLAG_PIPE) {
                // create new node and change reference
                shell_node* next = empty_shell_node();
                current->next = next;
                current = next;
                flags &= ~FLAG_PIPE;
            } else if (flags & FLAG_BG) {
                current->background = true;
                flags &= ~FLAG_BG;
            } else if (len > 0) {
                char* word = malloc(len * sizeof(char));
                strncpy(word, str + prev_pos, len);
                if (flags & FLAG_FNAM) {
                    // apply filenames to current node
                    if (fileno == STDIN_FILENO) {
                        current->in_ff = fd_fn_create(STDIN_FILENO, word);
                    } else if (fileno == STDOUT_FILENO) {
                        current->out_ff = fd_fn_create(STDOUT_FILENO, word);
                    } else if (fileno == STDERR_FILENO) {
                        current->err_ff = fd_fn_create(STDERR_FILENO, word);
                    }
                    flags &= ~FLAG_FNAM;
                } else if (flags & FLAG_NEXT) {
                    // take next word and add to cmds
                    current->cmdc += 1;
                    current->cmds = realloc(current->cmds, current->cmdc * sizeof(char*));
                    current->cmds[current->cmdc - 1] = word;
                    flags &= ~FLAG_NEXT;
                }
            }
            prev_pos = pos + 1;
        }
        pos += 1;
    }
    
    return 0;
}

// MARK: execution

void cd(char** cmds, int cmdc) {
    if (cmdc > 1) {
        if (chdir(cmds[1]) != 0) {
            printf("Usage: cd <path_to_directory>\n");
        }
    } else {
        chdir(home_dir());
    }
}

int setup_fds(shell_node* node) {
    int fd;
    static int null_fd = -1;
    char* filename;
    
    if (node->in_ff != NULL) {
        filename = node->in_ff->filename;
        fd = open(filename, O_RDWR, 0666);
        if (fd < 0) {
            perror("Input file");
            return 1;
        } else {
            node->in_ff->fd = fd;
        }
    } else {
        node->in_ff = fd_fn_create(STDIN_FILENO, NULL);
    }
    
    if (node->out_ff != NULL) {
        filename = node->out_ff->filename;
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("Output file");
            return 2;
        } else {
            node->out_ff->fd = fd;
        }
    } else if (node->background) {
        if (null_fd == -1) {
             null_fd = open("/dev/null", O_WRONLY);
        }
        
        if (null_fd < 0) {
            perror("dev/null output file");
            return 4;
        } else {
            node->out_ff = fd_fn_create(null_fd, NULL);
        }
    } else {
        node->out_ff = fd_fn_create(STDOUT_FILENO, NULL);
    }
    
    if (node->err_ff != NULL) {
        filename = node->err_ff->filename;
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) {
            perror("Error output file");
            return 3;
        } else {
            node->err_ff->fd = fd;
        }
    } else if (node->background) {
        if (null_fd == -1) {
            null_fd = open("/dev/null", O_WRONLY);
        }
        
        if (null_fd < 0) {
            perror("dev/null error output file");
            return 4;
        } else {
            node->err_ff = fd_fn_create(null_fd, NULL);
        }
    } else {
        node->err_ff = fd_fn_create(STDERR_FILENO, NULL);
    }
    
    return 0;
}

void vxsh_exec_single(char** cmds, int cmdc) {
    if (cmds == NULL || cmdc == 0) {
        exit(1);
        return;
    }
    
    char* name = cmds[0];
    char** args = calloc(cmdc + 1, sizeof(char*));
    memcpy(args, cmds, cmdc * sizeof(char**));
    
    if (strcmp(name, "exit") == 0) {
        kill(getppid(), SIGUSR1);
        exit(2);
    }
    
    execvp(name, args);
    perror("execvp: ");
    exit(3);
}

void vxsh_exec_nodes(shell_node* head, shell_node* parent, int* parent_fd) {
    if (head == NULL || head->cmdc == 0) {
        // nothing to do here
        return;
    }
    // cd
    if (strcmp(head->cmds[0], "cd") == 0) {
        cd(head->cmds, head->cmdc);
        return;
    }
    
    int w;
    int fd[2];
    if (pipe(fd) < 0) {
        perror("lsh: pipe");
        return;
    }
    
    task_pid = fork();
    if (task_pid < 0) {
        fprintf(stderr, "fork Failed" );
        return;
    } else if (task_pid == 0) {
        // child
        setup_fds(head);
        close(fd[0]);
        fflush(stdout);
        
        if (parent != NULL && parent_fd != NULL) {
            close(parent_fd[1]);
            head->in_ff->fd = parent_fd[0];
        }
        
        if (head->next != NULL) {
            head->out_ff->fd = fd[1];
        } else {
            close(fd[1]);
        }
        
        dup2(head->in_ff->fd, STDIN_FILENO);
        dup2(head->out_ff->fd, STDOUT_FILENO);
        dup2(head->err_ff->fd, STDERR_FILENO);
        
        vxsh_exec_single(head->cmds, head->cmdc);
        fprintf(stderr, "Error while processing commands.");
        exit(4);
    } else if (task_pid > 0) {
        // parent
        close(fd[1]);
        if (!head->background) {
            wait(&w);
            if (WIFEXITED(w)) {
                return vxsh_exec_nodes(head->next, head, fd);
            } else {
                fprintf(stderr, "Error.\n");
            }
        } else {
            printf("* [%d]\n", task_pid);
            if (head->next != NULL) {
                fprintf(stderr, "-Syntax error near token |.\n");
            }

            return;
        }
    }
}
