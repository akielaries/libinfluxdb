#include "../lib/libifdb.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // InfluxInfo struct
  struct InfluxInfo *ifdb_info;

  char *hostname = "192.168.86.53";
  uint32_t port  = 8086;
  // char *database = "LIBIFDB_TEST";
  char *database = "test_bucket1";
  char *user     = "root";
  char *pass     = "root";
  char *tags     = "host=kokopuffs"; // TODO: make tags optional

  // initialize the database/bucket. if it does not exist, this will create it
  ifdb_info = ifdb_init(hostname, port, database, user, pass, tags);
  if (ifdb_info == NULL) {
    printf("Error initializing InfluxDB\n");
    return -1;
  }

  // pass in a custom query as a string and store the result
  InfluxResult *result = ifdb_query(ifdb_info, "USE %s", test_bucket1);
  if (result == NULL) {
    printf("Query to use database/bucket: %s failed\n", database);
    return -2;
  }

  // display the result of the query
  uint64_t num_fields = ifdb_num_fields(result);
  InfluxRow row;
  while ((row = ifdb_fetch_row(result))) {
    for (uint64_t i = 0; i < num_fields; i++) {
      printf("%s\t", row[i] ? row[i] : "NULL");
    }
    printf("\n");
  }

  // show the bucket's measurements
  InfluxResult *result2 = ifdb_query(ifdb_info, "SHOW MEASUREMENTS");
  if (result2 == NULL) {
    printf("Query to show measurements in %s failed\n", database);
  }

  // display the result of the query
  num_fields = ifdb_num_fields(result);
  InfluxRow row2;
  while ((row2 = ifdb_fetch_row(result))) {
    for (uint64_t i = 0; i < num_fields; i++) {
      printf("%s\t", row2[i] ? row2[i] : "NULL");
    }
    printf("\n");
  }

  return 0;
}
