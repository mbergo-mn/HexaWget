#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>

#define NUM_THREADS 6

// Structure to hold download parameters
typedef struct {
    long start;
    long end;
    int partNum;
    char url[1024];
} download_part_args;

// Function to write data received from HTTP request
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

// Function to download part of a file
void *download_part(void *arguments) {
    download_part_args *args = (download_part_args *)arguments;
    
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char range[64];
    char outfilename[64];

    // Prepare file name and range
    sprintf(outfilename, "part%d", args->partNum);
    sprintf(range, "%ld-%ld", args->start, args->end);

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename, "wb");

        // Set CURL options
        curl_easy_setopt(curl, CURLOPT_URL, args->url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_RANGE, range);

        // Perform the download
        res = curl_easy_perform(curl);

        // Cleanup
        fclose(fp);
        curl_easy_cleanup(curl);
    }

    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        return -1;
    }

    // Get the size of the file
    CURL *curl = curl_easy_init();
    double filesize = 0.0;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, argv[1]);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0);
        curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
        if(curl_easy_perform(curl) == CURLE_OK) {
            if(curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &filesize) != CURLE_OK) {
                perror("curl_easy_getinfo");
                return -1;
            }
        } else {
            perror("curl_easy_perform");
            return -1;
        }
        curl_easy_cleanup(curl);
    }

    // Prepare thread data
    pthread_t threads[NUM_THREADS];
    download_part_args args[NUM_THREADS];
    long part_size = (long)(filesize / NUM_THREADS);

    // Start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start = i * part_size;
        args[i].end = (i + 1) * part_size - 1;
        if (i == NUM_THREADS - 1) args[i].end += (long)(filesize % NUM_THREADS);  // Last part gets remainder
        args[i].partNum = i;
        strcpy(args[i].url, argv[1]);

        if (pthread_create(&threads[i], NULL, download_part, (void *)&args[i])) {
            perror("pthread_create");
            return -1;
        }
    }

    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL)) {
            perror("pthread_join");
            return -1;
        }
    }

    // Concatenate parts
    FILE *fp = fopen("downloaded_file", "wb");
    for (int i = 0; i < NUM_THREADS; i++) {
        char outfilename[64];
        sprintf(outfilename, "part%d", i);

        FILE *part_fp = fopen(outfilename, "rb");
        if (part_fp == NULL) {
            perror("fopen");
            return -1;
        }

        char buffer[8192];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), part_fp)) > 0) {
            fwrite(buffer, 1, bytes, fp);
        }
        fclose(part_fp);

        // Remove the part file
        remove(outfilename);
    }
    fclose(fp);

    return 0;
}
