/*
 * Influx C (ic) client for data capture header file
 * Developer: Nigel Griffiths.
 * (C) Copyright 2021 Nigel Griffiths
 */
#ifndef __LIBIFDB_H__
#define __LIBIFDB_H__

#include "utils.h"

// main libinfluxdb macro
#define __LIBIFDB__

typedef struct InfluxInfo {
    char influx_hostname[1024 + 1];
    char influx_ip[16 + 1];
    long influx_port;
    char influx_database[256 + 1];
    char influx_username[64 + 1];
    char influx_password[64 + 1];
    char *influx_tags;
} InfluxInfo;

typedef struct InfluxOutput {
    char *output;
    long output_size;
    long output_char;
    char saved_section[64];
    char saved_sub[64];
} InfluxOutput;

void ic_influx_database(char *host, long port, char *db, InfluxInfo *info);
void ic_influx_userpw(char *user, char *pw);
void ic_tags(char *tags, InfluxInfo *info);

void ic_measure(char *section, InfluxInfo *info);
void ic_measureend();

void ic_sub(char *sub_name, InfluxInfo *info);
void ic_subend();

void ic_long(char *name, long long value);
void ic_double(char *name, double value);
void ic_string(char *name, char *value);

void ic_push();
void ic_debug(int level);

#endif
