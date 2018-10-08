//
//  main.c
//  vxsh
//
//  Copyright © 2017 Kacper Raczy. All rights reserved.
//

#include "vxsh.h"
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

void stop_curent_task_signal(int signo) {
    int task_pid = vxsh_task_pid();
    if(signo == SIGINT && task_pid != 0) {
        kill(task_pid, SIGINT);
    }
    signal(SIGINT, stop_curent_task_signal);
}

void exit_signal(int signo) {
    if(signo == SIGUSR1) {
        exit(0);
    }
}

char* fgets_wrapper(char *buffer, int buflen, FILE *fp) {
    if (fgets(buffer, buflen, fp) != 0) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            // trailing space required
            buffer[len-1] = ' ';
            buffer[len] = '\0';
        }
        
        return buffer;
    }

    return NULL;
}

int main(int argc, const char * argv[]) {
    // insert code here...
    char buf[256];
    shell_node* node;
    signal(SIGINT, stop_curent_task_signal);
    signal(SIGUSR1, exit_signal);
    
    while (true) {
        printf("vxsh$ ");
        fflush(stdout);
        
        if (fgets_wrapper(buf, sizeof(buf), stdin) != NULL) {
            node = empty_shell_node();
            vxsh_parse_str(node, buf);
            vxsh_exec_nodes(node, NULL, NULL);
            free_node(node);
            fflush(stdout);
        } else {
            perror("fgets error: ");
            break;
        }
    }
    
    return 0;
}
