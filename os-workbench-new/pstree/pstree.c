#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>

struct PROCESS
{
    char name[256];
    int pid;
    int ppid;
    // 子节点
    struct PROCESS *children;
    // 同级节点
    struct PROCESS *next;
};
typedef struct PROCESS PROCESS;

// 根节点
PROCESS *root = NULL;
// 进程列表
PROCESS *process_list[1024];

int is_numeric(char *str);
void process_traverse();
void read_process_status(char *filename, PROCESS *proc);
void print_proc_list(PROCESS **list);
void insert_node(PROCESS *node);
PROCESS *find_parent(PROCESS *root, int ppid);
void print_tree(PROCESS *root, int level, int show_pids_flag);
void sort(PROCESS *root);

// 打印进程树
int main(int argc, char *argv[])
{

    // PROCESS *proc = (PROCESS *)malloc(sizeof(PROCESS));
    // for (int i = 0; i < argc; i++)
    // {
    //     assert(argv[i]);
    //     printf("argv[%d] = %s\n", i, argv[i]);
    // }
    // assert(!argv[argc]);

    int opt;
    int show_pids_flag = 0;
    int number = 0;
    int version_flag = 0;

    static struct option long_options[] = {
        {"show-pids", no_argument, NULL, 'p'},
        {"number", no_argument, NULL, 'n'},
        {"version", no_argument, NULL, 'V'},
        {NULL, 0, NULL, 0}

    };

    // 使用getopt解析参数
    // :表示需要传递参数
    while ((opt = getopt_long(argc, argv, "pnV", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'p':
            show_pids_flag = 1;
            break;
        case 'n':
            number = 1;
            break;
        case 'V':
            version_flag = 1;
            break;
        default:
            printf("Usage %s [-p] [-n number] [-V]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    process_traverse();

    int i = 0;
    while (process_list[i] != NULL)
    {
        insert_node(process_list[i]);
        i++;
    }
    if (argc == 1)
    {
        print_tree(root, 0, 0);
        return 0;
    }
    if (version_flag)
    {
        printf("pstree (PSmisc) 0.1\nCopyright (C) 2024 w3b5h3ll.\n");
        return 0;
    }

    if (number)
    {
        sort(root);
    }

    print_tree(root, 0, show_pids_flag);

    return 0;
}

int is_numeric(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!isdigit(str[i]))
        {
            return 0;
        }
    }

    return 1;
}

// 遍历进程
void process_traverse()
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/proc");
    if (dir == NULL)
    {
        perror("Error opening /proc directory");
        exit(EXIT_FAILURE);
    }
    char filename[256];
    int i = 0;

    while ((entry = readdir(dir)) != NULL)
    {
        // 获取到了以数字开头的pid文件名
        if (entry->d_type == DT_DIR && is_numeric(entry->d_name))
        {
            // printf("%s\n", entry->d_name);

            memset(filename, 0, sizeof(filename));
            strcat(filename, "/proc/");
            strcat(filename, entry->d_name);
            strcat(filename, "/status");
            // printf("%s\n", filename);
            // snprintf(filename, sizeof(filename), "/proc/%s/status", entry->d_name);
            PROCESS *new_proc = (PROCESS *)malloc(sizeof(PROCESS));
            read_process_status(filename, new_proc);
            process_list[i++] = new_proc;
        }
    }

    closedir(dir);
}

// 读取/proc/pid/status文件内容至进程对象中
void read_process_status(char *filename, PROCESS *proc)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("Error opening process status file");
        exit(EXIT_FAILURE);
    }

    char line[256];

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        char *start = NULL;
        if ((start = strstr(line, "Name:")) != NULL)
        {
            start += strlen("Name:");
            while (*start == ' ' || *start == '\t')
            {
                start++;
            }
            strcpy(proc->name, start);
            char *newline = strchr(proc->name, '\n');
            if (newline != NULL)
            {
                *newline = '\0';
            }
        }
        // 检查是否是行首匹配
        if ((start = strstr(line, "Pid:")) != NULL && start == line)
        {
            int pid;
            if (sscanf(start + strlen("Pid:"), "%d", &pid) == 1)
            {
                proc->pid = pid;
            }
        }

        if ((start = strstr(line, "PPid:")) != NULL)
        {
            int ppid;
            if (sscanf(start + strlen("PPid:"), "%d", &ppid) == 1)
            {
                proc->ppid = ppid;
            }
        }
    }
}

// 插入节点，开始建树
void insert_node(PROCESS *node)
{
    node->children = NULL;
    node->next = NULL;
    if (root == NULL)
    {
        root = node;
        return;
    }

    // 查找父节点
    PROCESS *parent = find_parent(root, node->ppid);
    if (parent == NULL)
    {
        // 每个进程必有父节点
        // free(node);
        return;
    }

    node->next = parent->children;
    parent->children = node;
}

void print_proc_list(PROCESS **list)
{
    int i = 0;
    while (list[i] != NULL)
    {
        printf("PROCESS name: %s, pid: %d, ppid: %d\n", list[i]->name, list[i]->pid, list[i]->ppid);
        i++;
    }
}

PROCESS *find_parent(PROCESS *root, int ppid)
{
    if (root == NULL)
    {
        return NULL;
    }
    if (root->pid == ppid)
    {
        return root;
    }

    // 递归查找
    PROCESS *parent = NULL;
    for (PROCESS *child = root->children; child != NULL; child = child->next)
    {
        parent = find_parent(child, ppid);
        if (parent != NULL)
        {
            break;
        }
    }
    return parent;
}

// DFS打印树
void print_tree(PROCESS *root, int level, int show_pids_flag)
{
    if (root == NULL)
    {
        return;
    }
    for (int i = 0; i < level; i++)
    {
        printf("| ");
    }
    if (root->pid == 1)
    {
        if (show_pids_flag)
        {
            printf("%s(%d)\n", root->name, root->pid);
        }
        else
        {
            printf("%s\n", root->name);
        }
    }
    else
    {
        if (show_pids_flag)
        {
            printf("+-%s(%d)\n", root->name, root->pid);
        }
        else
        {
            printf("+-%s\n", root->name);
        }
    }

    for (PROCESS *child = root->children; child != NULL; child = child->next)
    {
        print_tree(child, level + 1, show_pids_flag);
    }
}

// 排序，按照pid从小到大
// 使用插入排序
// https://leetcode.cn/problems/sort-list/solutions/492301/pai-xu-lian-biao-by-leetcode-solution/
// https://leetcode.cn/problems/insertion-sort-list/description/
PROCESS *insert_sort(PROCESS *head)
{
    if (head == NULL)
    {
        return head;
    }

    // 创造一个伪结点，指向排序后的head
    PROCESS *dummy_head = malloc(sizeof(PROCESS));
    dummy_head->next = head;
    PROCESS *curr = head->next;
    PROCESS *last_sorted = head;

    while (curr != NULL)
    {
        if (last_sorted->pid <= curr->pid)
        {
            last_sorted = last_sorted->next;
        }
        else
        {
            // 已排序移动指针
            PROCESS *prev = dummy_head;
            // 寻找插入位置
            while (prev->next->pid <= curr->pid)
            {
                prev = prev->next;
            }
            last_sorted->next = curr->next;
            curr->next = prev->next;
            prev->next = curr;
        }
        curr = last_sorted->next;
    }
    return dummy_head->next;
}

void sort(PROCESS *root)
{
    if (root == NULL)
    {
        return;
    }
    root->children = insert_sort(root->children);
    PROCESS *current = root->children;
    while (current != NULL)
    {
        sort(current);
        current = current->next;
    }
}