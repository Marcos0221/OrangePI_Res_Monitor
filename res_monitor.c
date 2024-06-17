#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <oled.h>
#include <font.h>
#include <unistd.h>

#define BUF_SIZE 256

// 执行系统命令并返回结果
char* exec_command(const char* cmd) {
    FILE* pipe;
    char buffer[BUF_SIZE];
    char* result = malloc(BUF_SIZE);
    memset(result, 0, BUF_SIZE);

    if (!(pipe = popen(cmd, "r"))) {
        free(result);
        return NULL;
    }

    while (fgets(buffer, BUF_SIZE, pipe) != NULL) {
        strncat(result, buffer, BUF_SIZE - strlen(result) - 1);
    }

    pclose(pipe);
    return result;
}

void check_intent(struct display_info *disp)
{
    // 检查互联网连接
    char* ping_cmd = "ping -c 1 www.baidu.com > /dev/null 2>&1 && echo 1 || echo 0";
    char* connected = exec_command(ping_cmd);
    int is_connected = atoi(connected);
    free(connected);

    if (is_connected) {
        // 如果连接到互联网，获取局域网IP
        char* ip_cmd = "ip -4 addr show wlan0 | grep 'inet ' | awk '{print $2}' | cut -d'/' -f1";
        char* ip_output = exec_command(ip_cmd);
        char* ip_address = strtok(ip_output, "/");
        oled_putstrto(disp, 24, 45, ip_address);
    } else {
        // 如果没有连接到互联网，返回Disconnected
        oled_putstrto(disp, 24, 45, "Disconnected");
    }
    oled_send_buffer(disp);
}
// I2C 接口地址
static char *I2C_ADDRESS="/dev/i2c-3";

void oled_show(struct display_info *disp)
{
    oled_putstrto(disp, 0, 0, "CPU Temp:");
    oled_putstrto(disp, 0, 9, "Mem total:");
    oled_putstrto(disp, 0, 18, "Mem usage:");
    oled_putstrto(disp, 0, 27, "Mem free:");
    oled_putstrto(disp, 0, 36, "Run time:");
    oled_putstrto(disp, 0, 45, "IP:");

    // 发送缓存
    oled_send_buffer(disp);
}

// 读取内存信息，大小，占用，剩余内存
void read_mem_info(struct display_info *disp)
{
    int mem_total, mem_free;

    char *Mem_Total=(char*)malloc(20 * sizeof(char));
    char *Mem_Usage=(char*)malloc(20 * sizeof(char));
    char *Mem_Free=(char*)malloc(20 * sizeof(char));

    char *token = (char*)malloc(20 * sizeof(char));
    char line[1024];
    FILE *fp;
    char *path="/proc/meminfo";
    fp=fopen(path, "r");

    while (fgets(line, sizeof(line), fp)) {
        // 查找以 "MemTotal:" 开头的行
        if(strncmp(line, "MemTotal:", 9) == 0) {
            // 使用 strtok 分解字符串
            token = strtok(line, " \t:");
            // 跳过 "MemTotal" 和冒号
            token = strtok(NULL, " \t");
            // 转换字符串为长整型
            mem_total = atoll(token);
            break; // 找到后退出循环
        }
    }
    fclose(fp);

    fp=fopen(path, "r");
    while (fgets(line, sizeof(line), fp)) {
        // 查找以 "MemTotal:" 开头的行
        if(strncmp(line, "MemFree:", 8) == 0) {
            // 使用 strtok 分解字符串
            token = strtok(line, " \t:");
            // 跳过 "MemTotal" 和冒号
            token = strtok(NULL, " \t");
            // 转换字符串为长整型
            mem_free = atoll(token);
            break; // 找到后退出循环
        }
    }
    fclose(fp);

    float mem_usage;

    // 计算使用率
    mem_usage = (float)(mem_total - mem_free) / mem_total;

    sprintf(Mem_Total, "%d MB", mem_total / 1024);
    sprintf(Mem_Usage, "%.2f %%", mem_usage * 100.0f);
    sprintf(Mem_Free, "%d MB", mem_free / 1024);

    oled_putstrto(disp, 66, 9, Mem_Total);      // 总内存大小
    oled_putstrto(disp, 66, 18, Mem_Usage);     // 内存使用率
    oled_putstrto(disp, 60, 27, Mem_Free);      // 剩余内存
}

// 读取开机时长
void read_uptime(struct display_info * disp)
{
    FILE *fp;
    int sec;
    char *result=(char*)malloc(20 * sizeof(char));

    fp=fopen("/proc/uptime", "r");
    if (fp == NULL) {
        printf("/proc/uptime is not open\n");
    }

    fscanf(fp, "%d", &sec);

    // 不到一个小时
    if (sec<60) {
        sprintf(result, "%d sec", sec);
    }else if (sec>60 && sec < 3600) {
        sprintf(result, "%d m", sec / 60);
    }else if (sec>3600 && sec<60*60*24) {
        sprintf(result, "%d hour %d m", sec / 60 / 60, sec / 60 % 60);
    }

    oled_putstrto(disp, 60, 36, "          ");
    oled_putstrto(disp, 60, 36, result);
}

// 读取 cpu 温度
void* read_cpu_temp(void *args)
{
    FILE *fp;
    unsigned int temp;
    char *result=(char*)malloc(20 * sizeof(char));
    struct display_info *disp=(struct display_info *)args;

    while (1)
    {
        // 以只读的方式打开文件
        fp=fopen("/sys/class/thermal/thermal_zone0/temp", "r");

        // // 读取温度信息
        fscanf(fp, "%u", &temp);

        // // 拼接成字符串
        sprintf(result, "%.2f C", temp/1000.f);

        // // 关闭文件，显示温度
        fclose(fp);
        oled_putstrto(disp, 60, 0, result);

        // // 读取内存信息
        read_mem_info(disp);

        // // 读取运行时间
        read_uptime(disp);

        // 获取ip信息
        check_intent(disp);

        // 写入缓冲区
        oled_send_buffer(disp);

        // 加点延迟，减少cpu负担
        sleep(5);
    }
}

int main(int argc, char **agrv)
{
    pthread_t tid;
    struct display_info disp;

    memset(&disp, 0, sizeof(disp));

    disp.address=OLED_I2C_ADDR;
    disp.font=font2;

    oled_open(&disp, I2C_ADDRESS);      // 打开对应地址的i2c接口，等待调用
    oled_init(&disp);        // 初始化
    oled_show(&disp);        // 显示

    // 创建线程
    pthread_create(&tid, NULL, read_cpu_temp, &disp);

    // 等待线程结束
    pthread_join(tid, 0);

    return 0;
}