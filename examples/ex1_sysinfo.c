#include "../lib/libifdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int count;
    int i = 10;
    char buf[300 + 1];
    char myhostname[256 + 1];

/* RANGE uses to generate some random numbers between the min and max inclusive
 */
#define RANGE(min, max) ((int)random() % ((max) - (min) + 1)) + (min)

    srandom(getpid());

    // ic_debug(1); /* maximum output */

    // TODO: perhaps let the user populate the majority of this struct:
    // typedef struct InfluxInfo {
    //  char influx_hostname[1024 + 1];
    //  char influx_ip[16 + 1];
    //  long influx_port;
    //  char influx_database[256 + 1];
    //  char influx_username[64 + 1];
    //  char influx_password[64 + 1];
    //  char *influx_tags;
    // } InfluxInfo;
    struct InfluxInfo ifdb_info;

    //ifdb_info.influx_hostname = "localhost";
    //ifdb_info.influx_port = 8086;
    //ifdb_info.influx_database = "LIBIFDB_TEST";
    //ifdb_info.influx_username = "";
    //ifdb_info.influx_password = "";
    //ifdb_info.influx_tags = "host=friedicecream";


    // hostname, port, database. TODO, create database if it DNE
    // FIXME : ifdb_init
    ic_influx_database("localhost", 8086, "LIBIFDB_TEST", &ifdb_info);
    // blank for testing
    // FIXME : ifdb_login
    ic_influx_userpw("", "");

    /* Intitalise */

    /* get the local machine hostname */
    if (gethostname(myhostname, sizeof(myhostname)) == -1) {
        error("gethostname() failed");
    }
    snprintf(buf, 300, "host=%s", myhostname);
    // FIXME : ifdb_tag
    ic_tags(buf, &ifdb_info);

    /* Main capture loop - often data capture agents run until they are killed
     * or the server reboots */

    for (count = 0; count < 4; count++) {

        /* Simple Measure */

        // FIXME : ifdb_table_open
        // TODO: make measurement/table name a parameter somewhere!!
        ic_measure("cpu", &ifdb_info);
        // FIXME : ifdb_write_int
        ic_long("user", RANGE(20, 70));
        // FIXME : ifdb_write_double
        ic_double("system", RANGE(2, 5) * 3.142);
        if (RANGE(0, 10) > 6)
            // FIXME: ifdb_write_str
            ic_string("status", "high");
        else
            ic_string("status", "low");
        // FIXME : ifdb_table_close
        ic_measureend();

        /* Measure with a single subsection - could be more */

        ic_measure("disks", &ifdb_info);
        for (i = 0; i < 3; i++) {
            sprintf(buf, "sda%d", i);
            // FIXME : this and ic_subend are functions used for nested 
            // line inserts by the looks of it?
            ic_sub(buf, &ifdb_info);
            ic_long("reads", (long long)(i * RANGE(1, 30)));
            ic_double("writes", (double)(i * 3.142 * RANGE(1, 30)));
            ic_subend();
        }
        ic_measureend();

        /* Send all data in one packet to InfluxDB */

        ic_push();

        /* Wait until we need to capture the data again */

        sleep(5); /* Typically, this would be 60 seconds */
    }
    exit(0);
}
