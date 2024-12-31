/*
 * Influx C (ic) client for data capture header file
 * Developer: Nigel Griffiths.
 * (C) Copyright 2021 Nigel Griffiths
 */
#ifndef __LIBIFDB_H__
#define __LIBIFDB_H__

#include "utils.h"
#include <stdint.h>

// main libinfluxdb macro
#define __LIBIFDB__

typedef struct InfluxInfo {
    char *influx_hostname;
    char *influx_ip;
    uint32_t influx_port;
    char *influx_database;
    char *influx_username;
    char *influx_password;
    char *influx_tags;
} InfluxInfo;

typedef struct InfluxData {
    char *output;
    uint32_t output_size;
    uint32_t output_char;
    char saved_section[64];
    char saved_sub[64];
} InfluxData;

/* Public API functions */
//void ifdb_init(char *host, long port, char *db, InfluxInfo *info);
/**
 * @brief initialize and connect to InfluxDB instance
 *
 * @param[in] host host name of the device influx is running on
 * @param[in] port port number of the influx connection
 * @param[in] db   database name
 * @param[in] user username
 * @param[in] pass password
 * @param[in] tags database tags
 *
 * @return populated InfluxInfo pointer
 */
InfluxInfo* ifdb_init(char *host, uint32_t port, char *db, 
                     char *user, char *pass, char *tags);

int ifdb_close(InfluxInfo *info);

void ic_influx_userpw(char *user, char *pw);

void ic_tags(char *tags, InfluxInfo *info);

void ic_measure(char *section, InfluxInfo *info);

void ic_measureend();

void ic_sub(char *sub_name, InfluxInfo *info);

void ic_subend();

void ic_long(char *name, long long value);

void ic_double(char *name, double value);

void ic_string(char *name, char *value);

void ic_push(InfluxInfo *info);

void ic_debug(int level);



#endif
