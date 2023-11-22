#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <curl/curl.h>

#define NUM_THREADS 6

// A struct to hold download details - because who doesn't like being organized?? 📚
typedef struct {
    long start;
    long end;
    int partNum;
    char url[1024];
} download_part_args;

// A function so small yet so crucial, like the tiny screw in your glasses 🤓
size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

// Our heroic function downloading parts of the file, like a ninja in the night 🥷
void *download_part(void *arguments) {
    download_part_args *args = (download_part_args *)arguments;
    
    CURL *curl;
    FILE *fp;
    CURLcode res;
    char range[64];
    char outfilename[64];

    // Crafting filenames and ranges like an artisanal baker 🍞
    sprintf(outfilename, "part%d", args->partNum);
    sprintf(range, "%ld-%ld", args->start, args->end);

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outfilename, "wb");

        // Setting all the dials and knobs on our CURL machine 🎛️
        curl_easy_setopt(curl, CURLOPT_URL, args->url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_RANGE, range);

        // Off to the races! 🏁
        res = curl_easy_perform(curl);
        
        // Cleaning up after the party 🧹
        fclose(fp);
        curl_easy_cleanup(curl);
    }

    return NULL;
}

// The grand conductor of our multi-threaded orchestra 🎼
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <URL>\n", argv[0]);
        exit(-1);
    }

    // First, we ask politely how big the file is 🤔
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
                exit(-1);
            }
        } else {
            perror("curl_easy_perform");
            exit(-1);
        }
        curl_easy_cleanup(curl);
    }

    pthread_t threads[NUM_THREADS];
    download_part_args args[NUM_THREADS];
    long part_size = (long)(filesize / NUM_THREADS);

    // Spinning up threads like a DJ spins records 🎧
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start = i * part_size;
        args[i].end = (i + 1) * part_size - 1;
        if (i == NUM_THREADS - 1) args[i].end += (long)(filesize % NUM_THREADS);  // Last thread gets the leftovers 🍔
        args[i].partNum = i;
        strcpy(args[i].url, argv[1]);

        // Unleashing the thread hounds!
        if (pthread_create(&threads[i], NULL, download_part, (void *)&args[i])) {
            perror("pthread_create");
            exit(-1);
        }
    }

    // Waiting for our thread hounds to return 🐕
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL)) {
            perror("pthread_join");
            exit(-1);
        }
    }

    // Time to stitch it all together, like a digital Frankenstein 🧟
    FILE *fp = fopen("downloaded_file", "wb");
    for (int i = 0; i < NUM_THREADS; i++) {
        char outfilename[64];
        sprintf(outfilename, "part%d", i);

        FILE *part_fp = fopen(outfilename, "rb");
        if (part_fp == NULL) {
            perror("fopen");
            exit(-1);
        }

        char buffer[8192];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), part_fp)) > 0) {
            fwrite(buffer, 1, bytes, fp);
        }
        fclose(part_fp);
        remove(outfilename);  // Cleaning up our temporary mess 🗑️
    }
    fclose(fp);

    // Victory dance, because we just downloaded a file in style 💃🕺
    printf("Download complete! File assembled and ready for action.\n");
    return 0;
}
