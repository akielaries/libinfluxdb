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

// Helper function to get system data (mocked for now)
void get_system_data(char *cpu_temp, char *disk_space, char *ipv4) {
  // Example of mocked data - you can replace this with actual system commands
  snprintf(cpu_temp, MAX_BUF_SIZE, "72.5"); // Mocked CPU temperature in Celsius
  snprintf(disk_space,
           MAX_BUF_SIZE,
           "50"); // Mocked disk space usage percentage
  snprintf(ipv4, MAX_BUF_SIZE, "192.168.1.100"); // Mocked IPv4 address
}

// Helper function to build the data payload with multiple measurements
void build_payload(char *payload) {
  char cpu_temp[MAX_BUF_SIZE], disk_space[MAX_BUF_SIZE], ipv4[MAX_BUF_SIZE];

  // Get system data
  get_system_data(cpu_temp, disk_space, ipv4);

  // Format the measurements into InfluxDB line protocol format
  snprintf(payload,
           MAX_BUF_SIZE,
           "cpu_temp,host=server01 value=%s\n"
           "disk_space,host=server01 value=%s\n"
           "ipv4,host=server01 value=\"%s\"\n"
           "hostname,host=server01 value=\"%s\"\n",
           cpu_temp,
           disk_space,
           ipv4,
           hostname);
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
