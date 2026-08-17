#include "../csapp.h"

void call_python_script(const char *target_url, char *output_m3u8, size_t max_len)
{
    char command[MAXBUF];
    // Linux 环境下使用 python3 并在脚本名和参数加上引号
    snprintf(command, sizeof(command), "python3 get_m3u8.py \"%s\"", target_url);

    // Linux 环境下使用 popen 代替 Windows 的 _popen
    FILE *fp = popen(command, "r");
    if (fp == NULL)
    {
        strncpy(output_m3u8, "Error executing Python script", max_len);
        return;
    }

    if (fgets(output_m3u8, max_len, fp) != NULL)
    {
        output_m3u8[strcspn(output_m3u8, "\r\n")] = 0;
    }
    else
    {
        strncpy(output_m3u8, "NOT_FOUND", max_len);
    }

    pclose(fp);
}

int main(void)
{
    char *buf = getenv("QUERY_STRING");
    char *method = getenv("REQUEST_METHOD");
    char m3u8[MAXLINE] = {0}, content[MAXBUF];

    if (buf == NULL)
        buf = "";
    if (method == NULL)
        method = "GET";

    // 剥离表单提交自带的 "url=" 前缀
    char *raw_url = buf;
    if (strncmp(buf, "url=", 4) == 0)
    {
        raw_url = buf + 4;
    }

    call_python_script(raw_url, m3u8, sizeof(m3u8));

    int ofs = 0;
    ofs += sprintf(content + ofs, "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head><body>\r\n");
    ofs += sprintf(content + ofs, "<h2>M3U8 解析结果</h2>\r\n");
    if (strlen(m3u8) > 0 && strcmp(m3u8, "NOT_FOUND") != 0)
    {
        ofs += sprintf(content + ofs, "<p>提取成功: <a href=\"%s\" target=\"_blank\">%s</a></p>\r\n", m3u8, m3u8);
    }
    else
    {
        ofs += sprintf(content + ofs, "<p style=\"color:red;\">未找到 M3U8 地址，请检查目标网页</p>\r\n");
    }
    ofs += sprintf(content + ofs, "<hr><a href=\"/home.html\">返回首页</a>\r\n");
    ofs += sprintf(content + ofs, "</body></html>\r\n");

    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");

    if (strcasecmp("HEAD", method) != 0)
        printf("%s", content);

    fflush(stdout);
    exit(0);
}