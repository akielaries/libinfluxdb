#include "../lib/libifdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

int main(int argc, char **argv) {
    int count;
    int i = 10;
    char buf[300 + 1];
    char myhostname[256 + 1];

/* RANGE uses to generate some random numbers between the min and max inclusive
 */
#define RANGE(min, max) ((int)random() % ((max) - (min) + 1)) + (min)

    srandom(getpid());

    struct InfluxInfo *ifdb_info;

    char *hostname = "localhost";
    uint32_t port  = 8086;
    char *database = "LIBIFDB_TEST";
    char *user     = "";
    char *pass     = "";
    char *tags     = "host=blueberryhaze";  // TODO: make tags optional


    // hostname, port, database. TODO, create database if it DNE
    // TODO : ifdb_init should return a populate InfluxInfo struct pointer
    //ifdb_init("localhost", 8086, "LIBIFDB_TEST");
    ifdb_info = ifdb_init(hostname, port, database, user, pass, tags);
    // blank for testing
    // FIXME : ifdb_login
    //ic_influx_userpw("", "");

    /* Intitalise */

    // FIXME : ifdb_tag
    //ic_tags(buf, &ifdb_info);

    /* Main capture loop - often data capture agents run until they are killed
     * or the server reboots */
    //for (count = 0; count < 4; count++) {
    for (int i = 0; i < 10; i++) {

        // FIXME : ifdb_table_open
        // TODO: make measurement/table name a parameter somewhere!!
        ic_measure("table1", ifdb_info);
        // FIXME : ifdb_write_int
        ic_long("field1", i);
        // FIXME : ifdb_write_double
        ic_double("field2", i + 0.0);

        if (i > 5)
            // FIXME: ifdb_write_str
            ic_string("field3", "string1");
        else
            ic_string("field3", "string2");
        
        // FIXME : ifdb_table_close
        ic_measureend();

        /* Measure with a single subsection - could be more */

        ic_measure("table2", ifdb_info);
        for (int j = 0; j < 3; j++) {
            sprintf(buf, "subsection?%d", j);
            // FIXME : this and ic_subend are functions used for nested 
            // line inserts by the looks of it?
            ic_sub(buf, ifdb_info);
            ic_long("field1", j);
            ic_double("field2",j + 0.0);
            ic_subend();
        }
        ic_measureend();

        /* Send all data in one packet to InfluxDB */

        ic_push(ifdb_info);

        /* Wait until we need to capture the data again */

        sleep(5); /* Typically, this would be 60 seconds */
    }
    // probably 
    exit(0);
}
