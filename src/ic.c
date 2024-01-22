#include "../lib/libifdb.h"
#include "../lib/utils.h"
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

#define DEBUG if (debug)
/* Used as the default buffer sizes */
#define MEGABYTE (1024 * 1024)

/* 0=off, 1=on basic, 2=trace like output */
int debug = 0;

char influx_hostname[1024 + 1] = {0};
char influx_ip[16 + 1] = {0};
long influx_port = 0;

char influx_database[256 + 1];
char influx_username[64 + 1];
char influx_password[64 + 1];

char *output;
long output_size = 0;
long output_char = 0;

char *influx_tags;

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

// TODO LIBIFDB prefix
void ic_debug(int level) {
    debug = level;
}

/* ic_tags() argument is the measurement tags for influddb */
/* example: "host=vm1234"   note:the comma & hostname of the virtual machine
 * sending the data */
/* complex: "host=lpar42,serialnum=987654,arch=power9" note:the comma separated
 * list */
void ic_tags(char *t, InfluxInfo *info) {
    fprintf(stderr, "ic_tags(%s)\n", t);
    size_t length = strlen(t);

    influx_tags = (char *)malloc(length + 1); // +1 for the null terminator

    if (influx_tags == NULL) {
        error("failed to malloc() tags buffer");
    }

    strncpy(influx_tags, t, length);

    // null-terminate the influx_tags string
    influx_tags[length] = '\0';
    
}

/* converts influxdb hostname to IPv4 addr */
void ic_influx_database(
    char *host,
    long port,
    char *db) {
    struct hostent *he;
    char errorbuf[1024 + 1];

    influx_port = port;
    strncpy(influx_database, db, 256);

    if (host[0] <= '0' && host[0] <= '9') {
        DEBUG fprintf(stderr,
                      "ic_influx(ipaddr=%s,port=%ld,database=%s))\n",
                      host,
                      port,
                      db);
        strncpy(influx_ip, host, 16);
    } else {
        DEBUG fprintf(stderr,
                      "ic_influx_by_hostname(host=%s,port=%ld,database=%s))\n",
                      host,
                      port,
                      db);
        strncpy(influx_hostname, host, 1024);
        if (isalpha(host[0])) {

            he = gethostbyname(host);
            if (he == NULL) {
                sprintf(errorbuf,
                        "influx host=%s to ip address convertion failed "
                        "gethostbyname(), bailing out\n",
                        host);
                error(errorbuf);
            }
            /* this could return multiple ip addresses but we assume its the
             * first one */
            if (he->h_addr_list[0] != NULL) {
                strcpy(influx_ip,
                       inet_ntoa(*(struct in_addr *)(he->h_addr_list[0])));
                DEBUG fprintf(stderr,
                              "ic_influx_by_hostname hostname=%s converted to "
                              "ip address %s))\n",
                              host,
                              influx_ip);
            } else {
                sprintf(errorbuf,
                        "influx host=%s to ip address convertion failed (empty "
                        "list), bailing out\n",
                        host);
                error(errorbuf);
            }
        } else {
            strcpy(influx_ip,
                   host); /* perhaps the hostname is actually an ip address */
        }
    }
}

void ic_influx_userpw(char *user, char *pw) {
    DEBUG fprintf(stderr,
                  "ic_influx_userpw(username=%s,pssword=%s))\n",
                  user,
                  pw);
    strncpy(influx_username, user, 64);
    strncpy(influx_password, pw, 64);
}

int create_socket() /* returns 1 for error and 0 for ok */
{
    int i;
    static char buffer[4096];
    static struct sockaddr_in serv_addr;

    if (debug)
        DEBUG fprintf(stderr,
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

    /* connect tot he socket offered by the web server */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        DEBUG fprintf(stderr, " connect() call failed errno=%d", errno);
        return 0;
    }
    return 1;
}

/* check buffer space */
void ic_check(long adding) {
    /* first time create the buffer */
    if (output == (char *)0) { 
        /*
        if( (output = (char *)malloc(MEGABYTE)) == (char *)-1) {
            error("failed to malloc() output buffer");
        }
        // when near the end of the output
        if(output_char + (2*adding) > output_size) {
            buffer, extend it
        }*/
        if ((output = (char *)realloc(output, output_size + MEGABYTE)) == (char *)-1) {
            error("failed to realloc() output buffer");
        }
    }
}

void remove_ending_comma_if_any() {
    if (output[output_char - 1] == ',') {
        /* remove the char */
        output[output_char - 1] = 0;
        output_char--;
    }
}

void ic_measure(char *section) {
    ic_check(strlen(section) + strlen(influx_tags) + 3);

    output_char +=
        sprintf(&output[output_char], "%s,%s ", section, influx_tags);
    strcpy(saved_section, section);
    first_sub = 1;
    subended = 0;
    DEBUG fprintf(stderr,
                  "ic_measure(\"%s\") count=%ld\n",
                  section,
                  output_char);
}

void ic_measureend() {
    ic_check(4);
    remove_ending_comma_if_any();
    if (!subended) {
        output_char += sprintf(&output[output_char], "   \n");
    }
    subended = 0;
    DEBUG fprintf(stderr, "ic_measureend()\n");
}

/* Note this added a further tag to the measurement of the "resource_name" */
/* measurement might be "disks" */
/* sub might be "sda1", "sdb1", etc */
void ic_sub(char *resource) {
    int i;

    ic_check(strlen(saved_section) + strlen(influx_tags) + strlen(saved_sub) +
             strlen(resource) + 9);

    /* remove previously added section */
    if (first_sub) {
        for (i = output_char - 1; i > 0; i--) {
            if (output[i] == '\n') {
                output[i + 1] = 0;
                output_char = i + 1;
                break;
            }
        }
    }
    first_sub = 0;

    /* remove the trailing s */
    strcpy(saved_sub, saved_section);
    if (saved_sub[strlen(saved_sub) - 1] == 's') {
        saved_sub[strlen(saved_sub) - 1] = 0;
    }
    output_char += sprintf(&output[output_char],
                           "%s,%s,%s_name=%s ",
                           saved_section,
                           influx_tags,
                           saved_sub,
                           resource);
    subended = 0;
    DEBUG fprintf(stderr, "ic_sub(\"%s\") count=%ld\n", resource, output_char);
}

void ic_subend() {
    ic_check(4);
    remove_ending_comma_if_any();
    output_char += sprintf(&output[output_char], "   \n");
    subended = 1;
    DEBUG fprintf(stderr, "ic_subend()\n");
}

void ic_long(char *name, long long value) {
    ic_check(strlen(name) + 16 + 4);
    output_char += sprintf(&output[output_char], "%s=%lldi,", name, value);
    DEBUG fprintf(stderr,
                  "ic_long(\"%s\",%lld) count=%ld\n",
                  name,
                  value,
                  output_char);
}

void ic_double(char *name, double value) {
    ic_check(strlen(name) + 16 + 4);
    if (isnan(value) || isinf(value)) { /* not-a-number or infinity */
        DEBUG fprintf(stderr, "ic_double(%s,%.1f) - nan error\n", name, value);
    } else {
        output_char += sprintf(&output[output_char], "%s=%.3f,", name, value);
        DEBUG fprintf(stderr,
                      "ic_double(\"%s\",%.1f) count=%ld\n",
                      name,
                      value,
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
    output_char += sprintf(&output[output_char], "%s=\"%s\",", name, value);
    DEBUG fprintf(stderr,
                  "ic_string(\"%s\",\"%s\") count=%ld\n",
                  name,
                  value,
                  output_char);
}

void ic_push() {
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

    if (influx_port) {
        DEBUG fprintf(stderr, "ic_push() size=%ld\n", output_char);
        if (create_socket() == 1) {

            sprintf(buffer,
                    "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: "
                    "%s:%ld\r\nContent-Length: %ld\r\n\r\n",
                    influx_database,
                    influx_username,
                    influx_password,
                    influx_hostname,
                    influx_port,
                    output_char);
            DEBUG fprintf(stderr,
                          "buffer size=%ld\nbuffer=<%s>\n",
                          strlen(buffer),
                          buffer);
            if ((ret = write(sockfd, buffer, strlen(buffer))) !=
                strlen(buffer)) {
                fprintf(stderr,
                        "warning: \"write post to sockfd failed.\" errno=%d\n",
                        errno);
            }
            total = output_char;
            sent = 0;
            if (debug == 2)
                fprintf(stderr,
                        "output size=%d output=\n<%s>\n",
                        total,
                        output);
            while (sent < total) {
                ret = write(sockfd, &output[sent], total - sent);
                DEBUG fprintf(stderr,
                              "written=%d bytes sent=%d total=%d\n",
                              ret,
                              sent,
                              total);
                if (ret < 0) {
                    fprintf(
                        stderr,
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
                DEBUG fprintf(stderr,
                              "received bytes=%d data=<%s>\n",
                              ret,
                              result);
                sscanf(result, "HTTP/1.1 %d", &code);
                for (i = 13; i < 1024; i++)
                    if (result[i] == '\r')
                        result[i] = 0;
                if (debug == 2)
                    fprintf(stderr,
                            "http-code=%d text=%s [204=Success]\n",
                            code,
                            &result[13]);
                if (code != 204)
                    fprintf(stderr, "code %d -->%s<--\n", code, result);
            }
            close(sockfd);
            sockfd = 0;
            DEBUG fprintf(stderr, "ic_push complete\n");
        } else {
            DEBUG fprintf(stderr, "socket create failed\n");
        }
    } else
        error("influx port is not set, bailing out");

    output[0] = 0;
    output_char = 0;
}
