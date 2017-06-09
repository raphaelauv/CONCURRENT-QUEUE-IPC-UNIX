#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "../conduct.h"

int
main()
{
    struct conduct *c;
    int n, rc;
    unsigned char buf[4096];
    pid_t pid;

    printf("Trivial...\n");

    c = conduct_create("toto", 1, 4096);
    if(c == NULL){
        abort();
    }
    n = conduct_write(c, "toto", 4);
    if(n != 4){
    	abort();
    }
    rc = conduct_write_eof(c);
    if(rc != 1){
    	abort();
    }
    n = conduct_read(c, buf, 4);
    if(n != 4){
        abort();
    }
    if(memcmp(buf, "toto", 4) != 0){
    	abort();
    }
    n = conduct_read(c, buf, 4);
    if(n != 0){
    	abort();
    }
    rc = conduct_write_eof(c);
    if(rc != 0){
        abort();
    }
    conduct_destroy(c);

    printf("Vide...\n");
    c = conduct_create(NULL, 1, 4096);
    if(c == NULL)
        abort();
    rc = conduct_write_eof(c);
    if(rc != 1)
        abort();
    n = conduct_read(c, buf, 4);
    if(n != 0)
        abort();
    conduct_destroy(c);

    printf("Concurrent...\n");

    c = conduct_create(NULL, 4, 4096);
    if(c == NULL)
        abort();
    pid = fork();
    if(pid < 0) {
        perror("fork");
        abort();
    }
    if(pid == 0) {
        for(int i = 0; i < 1024; i++) {
            n = conduct_write(c, "toto", 4);
            if(n != 4)
                abort();
        }
        conduct_write_eof(c);
		printf("write finish\n");fflush(NULL);
        exit(0);

    } else {
        unsigned char buf[32 * 1024];
        int m = 0;
        while(1) {
            n = conduct_read(c, buf, 32 * 1024);
            if(n == 0)
                break;
            if(n < 0){
                printf("LA\n");fflush(NULL);abort();
            }
            m += n;
        }
        if(m != 4 * 1024){
        	printf("ICI\n");fflush(NULL);
            abort();
        }
        wait(NULL);
    }
    conduct_destroy(c);

    printf("Named concurrent...\n");

    c = conduct_create("/tmp/toto", 1, 4096);
    if(c == NULL)
        abort();
    pid = fork();
    if(pid < 0) {
        perror("fork");
        abort();
    }
    if(pid == 0) {
        c = conduct_open("/tmp/toto");
        if(c == NULL)
            abort();
        n = conduct_write(c, "toto", 4);
        if(n != 4)
            abort();
        conduct_write_eof(c);
        conduct_close(c);
        exit(0);
    } else {
        unsigned char buf[1024];
        n = conduct_read(c, buf, 1024);
        if(n != 4)
            abort();
        n = conduct_read(c, buf, 1024);
        if(n != 0)
            abort();
        wait(NULL);
    }
    conduct_destroy(c);


    printf("Atomique...\n");

    c = conduct_create(NULL, 8, 4096);
    if(c == NULL)
        abort();
    pid = fork();
    if(pid < 0) {
        perror("fork");
        abort();
    }

    if(pid == 0) {
        pid = fork();
        if(pid < 0) {
            perror("fork");
            abort();
        }
        for(int i = 0; i < 1000; i++) {
            char *s = pid == 0 ? "AAAAAAX" : "AAY";
            n = conduct_write(c, s, strlen(s));
            if(n != strlen(s))
                abort();
        }
        if(pid > 0) {
            wait(NULL);
            conduct_write_eof(c);
        }
        exit(0);
    } else {
        int i = 0;
        while(1) {
            usleep(rand() % 2000);
            n = conduct_read(c, buf, 4096);
            if(n <= 0)
                break;
            i += n;
            if(memmem(buf, n, "XY", 2) != NULL ||
               memmem(buf, n, "YX", 2) != NULL)
                abort();
        }
        wait(NULL);
        if(i != 10 * 1000)
            abort();
    }
    conduct_destroy(c);

    printf("Gros atomique...\n");

    c = conduct_create(NULL, 1024, 8192);
    if(c == NULL)
        abort();
    pid = fork();
    if(pid < 0) {
        perror("fork");
        abort();
    }

    if(pid == 0) {
        pid = fork();
        if(pid < 0) {
            perror("fork");
            abort();
        }
        for(int i = 0; i < 1000; i++) {
            char a[1024], b[1024];
            memset(a, 'A', 1024);
            memset(b, 'B', 1024);
            char *s = pid == 0 ? a : b;
            n = conduct_write(c, s, 1024);
            if(n != 1024)
                abort();
        }
        if(pid > 0) {
            wait(NULL);
            conduct_write_eof(c);
        }
        exit(0);
    } else {
        char a[1024], b[1024];
        int i = 0;
        memset(a, 'A', 1024);
        memset(b, 'B', 1024);
        while(1) {
            usleep(rand() % 2000);
            n = conduct_read(c, buf, 1024);
            if(n <= 0)
                break;
            i++;
            if(buf[0] == 'A') {
                if(memcmp(buf, a, 1024) != 0)
                    abort();
            } else {
                if(memcmp(buf, b, 1024) != 0)
                    abort();
            }
        }
        if(i != 2000)
            abort();
        wait(NULL);
    }
    conduct_destroy(c);

    return 0;
}
