#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdint.h>

#define MAX_BUF_SIZE 1024

// InfluxDB credentials
char *hostname = "192.168.86.53";
uint32_t port  = 8086;
char *database = "test_bucket1";
char *token    = "rtGd3JEQgWFW5rEgCKyNLYIlQCoSu-"
                 "H8bMYE3ejQHI3I2tUbkNIZySWDZVfEuGanvH1ttow9ZBpLbi6EPKquFw==";

// Helper function to build the InfluxDB URL
void build_influxdb_url(char *url) {
  snprintf(url,
           MAX_BUF_SIZE,
           "http://%s:%u/api/v2/write?org=akiel&bucket=%s&precision=s",
           hostname,
           port,
           database);
}

// Helper function to build the data payload
void build_payload(char *payload) {
  // Example of a simple point with measurement and tags
  snprintf(payload,
           MAX_BUF_SIZE,
           "cpu_load_short,host=server01,region=us-west value=0.64");
}

int main() {
  CURL *curl;
  CURLcode res;

  // InfluxDB HTTP URL
  char url[MAX_BUF_SIZE];
  build_influxdb_url(url);

  // Data to send
  char payload[MAX_BUF_SIZE];
  build_payload(payload);

  // Initialize libcurl
  curl_global_init(CURL_GLOBAL_DEFAULT);
  curl = curl_easy_init();
  if (curl) {
    // Set options for the POST request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);

    // Set the Authorization header with the API token
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: text/plain");
    char auth_header[MAX_BUF_SIZE];
    snprintf(auth_header, MAX_BUF_SIZE, "Authorization: Token %s", token);
    headers = curl_slist_append(headers, auth_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Perform the request
    res = curl_easy_perform(curl);

    // Check for errors
    if (res != CURLE_OK) {
      fprintf(stderr,
              "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    } else {
      printf("Data written successfully to InfluxDB\n");
    }

    // Clean up
    curl_slist_free_all(headers); // Free headers
    curl_easy_cleanup(curl);
  }

  // Clean up global libcurl resources
  curl_global_cleanup();

  return 0;
}
