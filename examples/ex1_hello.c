//#include "../lib/libifdb.h"
#include <libifdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int rc = 0;
  // define credentials and info
  char *hostname     = "192.168.86.53";
  uint32_t port      = 8086;
  char *database     = "test_bucket1";
  char *token        = "rtGd3JEQgWFW5rEgCKyNLYIlQCoSu-"
                       "H8bMYE3ejQHI3I2tUbkNIZySWDZVfEuGanvH1ttow9ZBpLbi6EPKquFw==";
  char *organization = "akiel";

  // initialize InfluxDB connection
  InfluxInfo *ifdb_info =
    ifdb_init(token, hostname, organization, port, database);
  if (ifdb_info == NULL) {
    printf("Error initializing InfluxDB\n");
    return -1;
  }

  
  rc = ifdb_insert(ifdb_info, "measurement_a", 55.5594);
  if (rc != 0) {
    printf("[!] insert #1 failed with rc: %d\n", rc);
    return -1;
  }
  
  rc = ifdb_insert(ifdb_info, "measurement_a", 45.5598);
  if (rc != 0) {
    printf("[!] insert #2 failed with rc: %d\n", rc);
    return -1;
  }

  char start_time[] = "1970-01-01T00:00:00Z";
  char stop_time[]  = "2025-01-03T00:00:00Z";

  //ifdb_delete(ifdb_info, "measurement_a", start_time, stop_time);

  // print the database
  ifdb_show_db(ifdb_info);

  // close connection with the database/bucket
  ifdb_close(ifdb_info);

  return 0;
}
/*
    // pass in a custom query as a string and store the result
    InfluxResult *result = ifdb_query(ifdb_info, "USE %s", database);
    if (result == NULL) {
      printf("Query to use database/bucket: %s failed\n", database);
      //return -2;
    }


    // show the bucket's measurements
    InfluxResult *result2 = ifdb_query(ifdb_info, "SHOW MEASUREMENTS");
    if (result2 == NULL) {
      printf("Query to show measurements in %s failed\n", database);
      //return -3;
    }

    ifdb_show_db(result2);


    // display the result of the query
    num_fields = ifdb_num_fields(result);
    InfluxRow row2;
    while ((row2 = ifdb_fetch_row(result))) {
      for (uint64_t i = 0; i < 5; i++) {
        printf("%s\t", row2[i] ? row2[i] : "NULL");
      }
      printf("\n");
    }


  // close connection with the database/bucket
  ifdb_close(ifdb_info);

  return 0;
}
*/
