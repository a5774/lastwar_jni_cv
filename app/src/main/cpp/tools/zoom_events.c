#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/time.h>
#include <math.h>
#include <time.h>


#define EVENT_DEVICE "/dev/input/event10"
#define ROTATION_0  0
#define ROTATION_90  1
#define ROTATION_180  2
#define ROTATION_270  3
#define RAW_WIDTH_DEVICE 1800
#define RAW_HEIGHT_DEVICE 2880
#define RAW_RATIO_DEVICE 10


#define PINCH_FIELDS \
    X(initial_distance, int, 300) \
    X(final_distance, int, 50) \
    X(steps, int, 30)

typedef const struct {
#define X(name, type, value) type name;
    PINCH_FIELDS
#undef X
} PinchConfig;

PinchConfig fast = {
#define X(name, type, value) .name = value,
        PINCH_FIELDS
#undef X
};

static const struct timespec t5ms = {0, 5 * 1000 * 1000};
static const struct timespec t20ms = {0, 20 * 1000 * 1000};
static const struct timespec t500ms = {0, 500 * 1000 * 1000};


int send_event(int fd, __u16 type, __u16 code, __s32 value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    gettimeofday(&ev.time, NULL);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    if (write(fd, &ev, sizeof(ev)) < sizeof(ev)) {
        perror("write event");
        return -1;
    }
    return 0;
}

int send_sync(int fd) {
    return send_event(fd, EV_SYN, SYN_REPORT, 0);
}

//typedef const struct {
//    int initial_distance;
//    int final_distance;
//    int steps;
//    struct timespec step_delay;
//} PinchConfig;
//PinchConfig fast = {
//        .initial_distance = 300,
//        .final_distance = 50,
//        .steps = 30,
//        .step_delay = {0, 2000 * 1000}
//};


float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

void mapCoordinates(__u16 *x, __u16 *y, int rotation) {
    int map_x, map_y;
    switch (rotation) {
        case ROTATION_0:
            map_x = *x;
            map_y = *y;
            break;
        case ROTATION_90:
            map_x = RAW_WIDTH_DEVICE - *y;
            map_y = *x;
            break;
        case ROTATION_180:
            map_x = RAW_WIDTH_DEVICE - *x;
            map_y = RAW_HEIGHT_DEVICE - *y;
            break;
        case ROTATION_270:
            map_x = *y;
            map_y = RAW_HEIGHT_DEVICE - *x;
            break;
        default:
            map_x = *x;
            map_y = *y;
            break;
    }
    *x = map_x;
    *y = map_y;
}


void simulate_swipe_down(int fd, PinchConfig *config, char rotation) {
    __u16 center_x = RAW_HEIGHT_DEVICE / 2;
    __u16 center_y = RAW_WIDTH_DEVICE / 2;
    __u16 swipe_x_end = center_x;
    __u16 swipe_y_end = 1050;
    mapCoordinates(&swipe_x_end, &swipe_y_end, rotation);
    mapCoordinates(&center_x, &center_y, rotation);


    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, 0);
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, center_x * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y, center_y * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 8);
    send_event(fd, EV_ABS, ABS_MT_PRESSURE, 1);
    send_event(fd, EV_KEY, BTN_TOUCH, 1);
    send_sync(fd);
    nanosleep(&t5ms, NULL);
    for (int i = 1; i <= config->steps; i++) {
        float t = (float) i / (float) config->steps;
        float progress = smoothstep(t);
        int current_x = center_x + (int) (((float) (swipe_x_end - center_x)) * progress);
        int current_y = center_y + (int) (((float) (swipe_y_end - center_y)) * progress);
        send_event(fd, EV_ABS, ABS_MT_POSITION_X, current_x * RAW_RATIO_DEVICE);
        send_event(fd, EV_ABS, ABS_MT_POSITION_Y, current_y * RAW_RATIO_DEVICE);
        send_sync(fd);
        nanosleep(&t5ms, NULL);
    }
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    send_event(fd, EV_KEY, BTN_TOUCH, 0);
    send_sync(fd);
}

void simulate_pinch_zoom(int fd, PinchConfig *config) {
    int center_x = RAW_WIDTH_DEVICE / 2;
    int center_y = RAW_HEIGHT_DEVICE / 2;
    send_event(fd, EV_ABS, ABS_MT_SLOT, 0);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, 0);
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, center_x * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y,
               (center_y - config->initial_distance) * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 8);
    send_event(fd, EV_ABS, ABS_MT_PRESSURE, 1);


    send_event(fd, EV_ABS, ABS_MT_SLOT, 1);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, 1);
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, center_x * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y,
               (center_y + config->initial_distance) * RAW_RATIO_DEVICE);
    send_event(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 8);
    send_event(fd, EV_ABS, ABS_MT_PRESSURE, 1);

    send_event(fd, EV_KEY, BTN_TOUCH, 1);
    send_sync(fd);
    nanosleep(&t5ms, NULL);

    for (int i = 1; i <= config->steps; i++) {
        float t = (float) i / (float) config->steps;
        float progress = smoothstep(t);
        int current_distance = config->initial_distance -
                               (int) ((double) (config->initial_distance - config->final_distance) *
                                      progress);
//        printf("Smoothstep=%d\n", current_distance);
        send_event(fd, EV_ABS, ABS_MT_SLOT, 0);
        send_event(fd, EV_ABS, ABS_MT_POSITION_X, center_x * RAW_RATIO_DEVICE);
        send_event(fd, EV_ABS, ABS_MT_POSITION_Y, (center_y - current_distance) * RAW_RATIO_DEVICE);

        send_event(fd, EV_ABS, ABS_MT_SLOT, 1);
        send_event(fd, EV_ABS, ABS_MT_POSITION_X, center_x * RAW_RATIO_DEVICE);
        send_event(fd, EV_ABS, ABS_MT_POSITION_Y, (center_y + current_distance) * RAW_RATIO_DEVICE);
        send_sync(fd);
        nanosleep(&t20ms, NULL);
    }
    nanosleep(&t5ms, NULL);
    send_event(fd, EV_ABS, ABS_MT_SLOT, 0);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);

    send_event(fd, EV_ABS, ABS_MT_SLOT, 1);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);

    send_event(fd, EV_KEY, BTN_TOUCH, 0);
    send_sync(fd);
}

void ioctl_test() {
    int fd = open("/dev/input/event10", O_RDWR);
    if (fd < 0) {
        printf("fd erroe");
    }
    float rawX = 0, rawY = 0;
    struct input_event ev;
    struct input_absinfo abs_x, abs_y;
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_x);
    ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs_y);
    printf("screenX:%d,screenY:%d", abs_x.maximum, abs_y.maximum);
//    printf("screenX:%zd", read(fd, &ev, sizeof(struct input_event)));

    while (read(fd, &ev, sizeof(struct input_event)) > 0) {
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_MT_POSITION_X) {
                rawX = ev.value;
            }
            if (ev.code == ABS_MT_POSITION_Y) {
                rawY = ev.value;
            }
            float screenX = (rawX - (float) abs_x.minimum) /
                            (float) (abs_x.maximum - abs_x.minimum) * 1800;
            float screenY = (float) (rawY - (float) abs_y.minimum) /
                            (float) (abs_y.maximum - abs_y.minimum) * 2880;
            printf("screenX:%f,screenY:%f", screenX, screenY);
        }
        if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
            // 一次触摸事件完整了，进行旋转变换
        }
    }
    close(fd);
//       ratio = (int) (abs_info.maximum / (RAW_WIDTH_DEVICE - 1));


}

int main(int argc, char *argv[]) {
    int fd;
//    ioctl_test();
//    return 0;
    printf("argc:%d\n", argc);
    if (argc >= 2) {
        fd = open(argv[1], O_WRONLY);
        if (fd < 0) {
            perror("Input device not found!");
            return 1;
        }
        /*
        struct input_absinfo abs_info;
        ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs_info);
        ratio = (int) (abs_info.maximum / (RAW_WIDTH_DEVICE - 1));
         */
        printf( "simulate_pinch_zoom");
        simulate_pinch_zoom(fd, &fast);
//        nanosleep(&t500ms, NULL);
//        char rotation = (char) atoi(argv[2]);
//        simulate_swipe_down(fd, &fast, rotation);
        close(fd);
        return 0;
    }
}


