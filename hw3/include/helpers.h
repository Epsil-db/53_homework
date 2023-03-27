#ifndef HELPERS_H
#define HELPERS_H
#include <stdio.h>
#include "linkedList.h"
#include "icssh.h"

static void sio_ltoa(long v, char s[], int b);

static size_t sio_strlen(char s[]);

ssize_t sio_puts(char s[]);

ssize_t sio_puti(int pid);

ssize_t Sio_puts(char s[]);

ssize_t Sio_putl(long pid);

void sio_error(char s[]);

static void sio_reverse(char s[]);

void freeAndNull(job_info* job, char* line);

void changeDir(job_info* job);

int redirectionCheck(job_info* job);

int openIn(job_info* job, char* line);

int openOut(job_info* job, char* line);

int openErr(job_info* job, char* line);

void piping(job_info* job, char* line, List_t* bgList);

#endif