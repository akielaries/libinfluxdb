#include "../lib/libifdb.h"
#include "../lib/utils.h"
#include <stdint.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <unistd.h>

/* Used as the default buffer sizes */
#define MEGABYTE (1024 * 1024)

char *output;
long output_size = 0;
long output_char = 0;

int subended = 0;
int first_sub = 0;

char saved_section[64];
char saved_sub[64];

int sockfd;

void error(char *buf) {
    fprintf(stderr,
            "error: \"%s\" errno=%d meaning=\"%s\"\n",
            buf,
            errno,
            strerror(errno));
    close(sockfd);
    sleep(2); /* this can help the socket close cleanly at the remote end */
    exit(1);
}

/* ic_tags() argutment is the measurement tags for influddb */
/* example: "host=vm1234"   note:the comma & hostname of the virtual machine
 * sending the data */
/* complex: "host=lpar42,serialnum=987654,arch=power9" note:the comma separated
 * list */
void ic_tags(char *t, InfluxInfo *info) {
    fprintf(stderr, "ic_tags(%s)\n", t);
    size_t length = strlen(t);

    if (info->influx_tags == NULL) {
        info->influx_tags = (char *)malloc(length + 1);
        if (info->influx_tags == NULL) {
            error("failed to malloc() tags buffer");
        }
    }

    strncpy(info->influx_tags, t, length);

    // null-terminate the influx_tags string
    info->influx_tags[length] = '\0';
}

InfluxInfo* ifdb_alloc(char *host, uint32_t port, char *db,
                       char *user, char *pass, char *tags) {

    // TODO created utility function for allocating memory?
    InfluxInfo *info = (InfluxInfo *)malloc(sizeof(InfluxInfo));

    if (info == NULL) {
        fprintf(stderr, "Failed to allocate memory for InfluxInfo struct\n");
        exit(EXIT_FAILURE);
    }   

    // allocate memory for each struct member
    info->influx_hostname = (char *)malloc(strlen(host) + 1); 
    info->influx_port = port;
    info->influx_database = (char *)malloc(strlen(db) + 1); 
    info->influx_username = (char *)malloc(strlen(user) + 1); 
    info->influx_password = (char *)malloc(strlen(pass) + 1); 
    info->influx_tags = (char *)malloc(strlen(tags) + 1); 
    
    // copy function parameters to InfluxInfo structure
    strncpy(info->influx_hostname, host, strlen(host));
    strncpy(info->influx_database, db, strlen(db));
    // TODO : account for some security here FIXME
    strncpy(info->influx_username, user, strlen(user));
    strncpy(info->influx_password, pass, strlen(pass));
    strncpy(info->influx_tags, tags, strlen(tags));

    // ensure null termination of strings in our struct
    info->influx_hostname[strlen(host)] = '\0';
    info->influx_database[strlen(db)] = '\0';
    info->influx_username[strlen(user)] = '\0';
    info->influx_password[strlen(pass)] = '\0';
    info->influx_tags[strlen(tags)] = '\0';

    return info;
}

int ifdb_dealloc(InfluxInfo *info) {
    if (info == NULL) {
        fprintf(stderr, "InfluxInfo struct memory allready deallocated");
    }
    else {
        // free all memory from the InfluxInfo struct
        free(info->influx_hostname);
        free(info->influx_database);
        free(info->influx_username);
        free(info->influx_password);
        free(info->influx_tags);
        free(info);
    }
}

// returns populated InfluxInfo struct
InfluxInfo* ifdb_init(char *host, uint32_t port, char *db, 
                     char *user, char *pass, char *tags) {

    struct hostent *he;
    char errorbuf[1024 + 1];
    
    // allocate memory for InfluxInfo struct
    InfluxInfo *info = ifdb_alloc(host, port, db, user, pass, tags);

    if (isdigit((unsigned char)host[0])) {
        fprintf(stderr, "ic_influx(ipaddr=%s,port=%ld,database=%s)\n",
                host, port, db);
        strncpy(info->influx_hostname, host, 16);
    } 

    else {
        fprintf(stderr, "ic_influx_by_hostname(host=%s,port=%ld,database=%s)\n",
                host, port, db);

        strncpy(info->influx_hostname, host, strlen(host));

        if (isalpha((unsigned char)host[0])) {
            he = gethostbyname(host);

            if (he == NULL) {
                snprintf(errorbuf, sizeof(errorbuf),
                         "influx host=%s to ip address conversion failed "
                         "gethostbyname(), bailing out\n", host);
                fprintf(stderr, "%s", errorbuf);
            }

            if (he->h_addr_list[0] != NULL) {
                strcpy(info->influx_hostname,
                       inet_ntoa(*(struct in_addr *)(he->h_addr_list[0])));
                fprintf(stderr, "ic_influx_by_hostname hostname=%s converted to "
                                "ip address %s)\n", host, info->influx_hostname);
            } else {
                snprintf(errorbuf, sizeof(errorbuf),
                         "influx host=%s to ip address conversion failed (empty "
                         "list), bailing out\n", host);
                fprintf(stderr, "%s", errorbuf);
            }
        } else {
            // Perhaps the hostname is actually an IP address
            strcpy(info->influx_hostname, host);
        }
    }

    // return populated InfluxInfo struct pointer
    return info;
}

int create_socket(char *influx_ip, uint32_t influx_port) {
    int i;
    static char buffer[4096];
    static struct sockaddr_in serv_addr;
    int socket_fd = 0;

    fprintf(stderr,
            "socket: trying to connect to \"%s\":%ld\n",
            influx_ip,
            influx_port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("socket() call failed");
        return 0;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(influx_ip);
    serv_addr.sin_port = htons(influx_port);

    /* connect to the socket offered by the web server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, " connect() call failed errno=%d\n", errno);
        return 0;
    }
    // TODO populate socket in a struct to use accross the functions that need 
    // it OR return the socket and pass by parameter
    return 1;
}

/* check buffer space */
// void ic_check(long addings, InfluxData *data)
void ic_check(long adding) {
    /* first time create the buffer */
    // if (data->output == (char *)0)
    if (output == (char *)0) {
        // if ((data->output = (char *)realloc(data->output, data->output_size + MEGABYTE))
        if ((output = (char *)realloc(output, output_size + MEGABYTE)) ==
            (char *)-1) {
            error("failed to realloc() InfluxData->output buffer");
        }
    }
}

void remove_ending_comma_if_any() {
    // if (data->output[data->output_char -1] == ',')
    if (output[output_char - 1] == ',') {
        /* remove the char */
        // data->output[data->output_char - 1] = 0;
        output[output_char - 1] = 0;
        // decrement output character
        // data->output_char--
        output_char--;
    }
}

// this seems to create a populate an Influx database table
// void ic_measure(char *table, InfluxInfo *info, InfluxData *data) {
void ic_measure(char *section, InfluxInfo *info) {
    ic_check(strlen(section) + strlen(info->influx_tags) + 3);

    // data->output_char += sprintf(&data->output[data->output_char], "%s,%s ", section, info->influx_tags);
    output_char += sprintf(&output[output_char], "%s,%s ", section, info->influx_tags);
    // strcpy(data->saved_section, section);
    strcpy(saved_section, section);
    first_sub = 1;
    subended = 0;
    // fprintf(stderr, "ic_measure(\"%s\") count=%ld\n", section, data->output_char);
    fprintf(stderr, "ic_measure(\"%s\") count=%ld\n", section, output_char);
}

void ic_measureend() {
    ic_check(4);
    remove_ending_comma_if_any();
    if (!subended) {
        // data->output_char += sprintf(&data->output[data->output_char], "   \n");
        output_char += sprintf(&output[output_char], "   \n");
    }
    subended = 0;
    fprintf(stderr, "ic_measureend()\n");
}

/* Note this added a further tag to the measurement of the "resource_name" */
/* measurement might be "disks" */
/* sub might be "sda1", "sdb1", etc */
void ic_sub(char *resource, InfluxInfo *info) {
    int i;

    // ic_check(strlen(data->saved_section) + strlen(info->influx_tags) + strlen(data->saved_sub) +
    //         strlen(resource) + 9);

    ic_check(strlen(saved_section) + strlen(info->influx_tags) + strlen(saved_sub) +
             strlen(resource) + 9);

    /* remove previously added section */
    if (first_sub) {
        // for (i = data->output_char - 1; i > 0; i--) {
        for (i = output_char - 1; i > 0; i--) {
            // if (data->output[i] == '\n') {
            if (output[i] == '\n') {
                // data->output[i + 1] = 0;
                output[i + 1] = 0;
                // data->output_char = i + 1;
                output_char = i + 1;
                break;
            }
        }
    }
    first_sub = 0;

    /* remove the trailing s */
    // strcpy(data->saved_sub, data->saved_section);
    strcpy(saved_sub, saved_section);
    // if (data->saved_sub[strlen(data->saved_sub) - 1] == 's')
    if (saved_sub[strlen(saved_sub) - 1] == 's') {
        // data->saved_sub[strlen(data->saved_sub) - 1] = 0;
        saved_sub[strlen(saved_sub) - 1] = 0;
    }
    // data->output_char += sprint(&data->output[data->output_char],
    output_char += sprintf(&output[output_char],
                           "%s,%s,%s_name=%s ",
                           // data->saved_section,
                           saved_section,
                           info->influx_tags,
                           // data->saved_sub,
                           saved_sub,
                           resource);
    subended = 0;

    // fprintf(stderr, "ic_sub(\"%s\") count=%ld\n", resource, data->output_char);
    fprintf(stderr, "ic_sub(\"%s\") count=%ld\n", resource, output_char);
}

void ic_subend() {
    ic_check(4);
    remove_ending_comma_if_any();
    // data->output_char += sprintf(&data->output[data->output_char], "   \n");
    output_char += sprintf(&output[output_char], "   \n");
    subended = 1;
    fprintf(stderr, "ic_subend()\n");
}

void ic_long(char *name, long long value) {
    ic_check(strlen(name) + 16 + 4);
    // data->output_char += sprintf(&data->output[data->output_char], "%s=%lldi,", name, value);
    output_char += sprintf(&output[output_char], "%s=%lldi,", name, value);
    fprintf(stderr,
            "ic_long(\"%s\",%lld) count=%ld\n",
            name,
            value,
            // data->output_char);
            output_char);
}

void ic_double(char *name, double value) {
    ic_check(strlen(name) + 16 + 4);
    if (isnan(value) || isinf(value)) { /* not-a-number or infinity */
        fprintf(stderr, "ic_double(%s,%.1f) - nan error\n", name, value);
    } else {
        // data->output_char += sprintf(&data->output[data->output_char], "%s=%.3f,", name, value);
        output_char += sprintf(&output[output_char], "%s=%.3f,", name, value);
        fprintf(stderr,
                "ic_double(\"%s\",%.1f) count=%ld\n",
                name,
                value,
                // data->output_char);
                output_char);
    }
}

void ic_string(char *name, char *value) {
    int i;
    int len;

    ic_check(strlen(name) + strlen(value) + 4);
    len = strlen(value);
    /* replace problem characters and with a space */
    for (i = 0; i < len; i++)
        if (value[i] == '\n' || iscntrl(value[i]))
            value[i] = ' ';
    // data->output_char += sprintf(&data->output[data->output_char], "%s=\"%s\",", name, value);
    output_char += sprintf(&output[output_char], "%s=\"%s\",", name, value);
    fprintf(stderr,
            "ic_string(\"%s\",\"%s\") count=%ld\n",
            name,
            value,
            // data->output_char);
            output_char);
}

// void ic_push(InfluxInfo *info, InfluxData *data) {
void ic_push(InfluxInfo *info) {
    char header[1024];
    char result[1024];
    char buffer[1024 * 8];
    int ret;
    int i;
    int total;
    int sent;
    int code;

    /* nothing to send so skip this operation */
    if (output_char == 0) {
        return;
    }

    if (info->influx_port) {
        // fprintf(stderr, "ic_push() size=%ld\n", data->output_char);
        fprintf(stderr, "ic_push() size=%ld\n", output_char);
        if (create_socket(info->influx_hostname, info->influx_port) == 1) {

            sprintf(buffer,
                    "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: "
                    "%s:%ld\r\nContent-Length: %ld\r\n\r\n",
                    info->influx_database,
                    info->influx_username,
                    info->influx_password,
                    info->influx_hostname,
                    info->influx_port,
                    output_char);
                    //data->output_char);
            
            fprintf(stderr,
                    "buffer size=%ld\nbuffer=<%s>\n",
                    strlen(buffer),
                    buffer);

            if ((ret = write(sockfd, buffer, strlen(buffer))) != strlen(buffer)) {
                fprintf(stderr,
                        "warning: \"write post to sockfd failed.\" errno=%d\n",
                        errno);
            }

            // total = data->output_char
            total = output_char;
            sent = 0;

            //fprintf(stderr, "output size=%d output=\n<%s>\n", total, data->output);
            fprintf(stderr, "output size=%d output=\n<%s>\n", total, output);

            while (sent < total) {
                // ret = write(sockfd, &data->output[sent], total - sent]);
                ret = write(sockfd, &output[sent], total - sent);
                fprintf(stderr,
                        "written=%d bytes sent=%d total=%d\n",
                        ret,
                        sent,
                        total);
                if (ret < 0) {
                    fprintf(stderr,
                            "warning: \"write body to sockfd failed.\" errno=%d\n",
                            errno);
                    break;
                }
                sent = sent + ret;
            }
            /* empty buffer */
            for (i = 0; i < 1024; i++) {
                result[i] = 0;
            }
            if ((ret = read(sockfd, result, sizeof(result))) > 0) {
                result[ret] = 0;
                fprintf(stderr, "received bytes=%d data=<%s>\n", ret, result);
                sscanf(result, "HTTP/1.1 %d", &code);
                for (i = 13; i < 1024; i++) {
                    if (result[i] == '\r') {
                        result[i] = 0;
                    }
                }

                fprintf(stderr,
                        "http-code=%d text=%s [204=Success]\n",
                        code,
                        &result[13]);

                if (code != 204)
                    fprintf(stderr, "code %d -->%s<--\n", code, result);
            }
            close(sockfd);
            sockfd = 0;
            fprintf(stderr, "ic_push complete\n");
        } 
        else {
            fprintf(stderr, "socket create failed\n");
        }
    } 
    else {
        error("influx port is not set, bailing out");
    }

    // data->output[0] = 0;
    // data->output_char = 0;
    output[0] = 0;
    output_char = 0;
}
