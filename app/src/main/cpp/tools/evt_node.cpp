//
// Created by Administrator on 2026/1/29.
//
//#include "jni.h"
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

void test()
{
    std::vector<int> v;
}

#define SYS_INPUT_PATH "/sys/class/input"
//#define SYS_INPUT_PATH "/sys/class/input/input10"
//#define SYS_INPUT_PATH "/sys/class/input/event10/device"
#define EV_ABS_BIT 0x08  // bit 3


int read_file_content(const char *path, char *buffer,  int size) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }
    if (fgets(buffer, size, fp) == NULL) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
    return 0;
}


int is_touchscreen(const char *event_name) {
    char path[512];
    char buffer[256];
    snprintf(path, sizeof(path), "%s/%s/device/name", SYS_INPUT_PATH, event_name);
    if (read_file_content(path, buffer, sizeof(buffer)) != 0) {
        return 0;
    }
    int name_match = (strstr(buffer, "TouchScreen") != NULL ||
                      strstr(buffer, "touchscreen") != NULL ||
                      strstr(buffer, "Touchscreen") != NULL);
    if (!name_match) {
        return 0;
    }
    snprintf(path, sizeof(path), "%s/%s/device/capabilities/ev", SYS_INPUT_PATH, event_name);
    if (read_file_content(path, buffer, sizeof(buffer)) != 0) {
        return 0;
    }
    unsigned long ev_value = strtoul(buffer, NULL, 16);
    if (!(ev_value & EV_ABS_BIT)) {
        return 0;
    }
    snprintf(path, sizeof(path), "%s/%s/device/capabilities/abs", SYS_INPUT_PATH, event_name);
    if (read_file_content(path, buffer, sizeof(buffer)) != 0) {
        return 0;
    }
    int is_zero = 1;
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] != '0' && buffer[i] != ' ' && buffer[i] != '\n') {
            is_zero = 0;
            break;
        }
    }
    if (is_zero) {
        return 0;
    }
    return 1;
}

int main() {
    DIR *dir;
    struct dirent *entry;
    dir = opendir(SYS_INPUT_PATH);
    if (!dir) {
        perror("Failed to open /sys/class/input");
        return 1;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }
        if (is_touchscreen(entry->d_name)) {
            char dev_path[256];
            char name_path[256];
            char name[256];
            char abs[256];
            char ev[256];
            snprintf(name_path, sizeof(name_path), "%s/%s/device/name",
                     SYS_INPUT_PATH, entry->d_name);
            read_file_content(name_path, name, sizeof(name));
            snprintf(name_path, sizeof(name_path), "%s/%s/device/capabilities/abs",
                     SYS_INPUT_PATH, entry->d_name);
            read_file_content(name_path, abs, sizeof(abs));
            snprintf(name_path, sizeof(name_path), "%s/%s/device/capabilities/ev",
                     SYS_INPUT_PATH, entry->d_name);
            read_file_content(name_path, ev, sizeof(ev));
            snprintf(dev_path, sizeof(dev_path), "/dev/input/%s", entry->d_name);
            printf("%s",dev_path);
        }
    }
    closedir(dir);
    return 0;
}

