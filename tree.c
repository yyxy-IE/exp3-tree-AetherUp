/**
 * 实验：目录树查看器（仿 Linux tree 命令）
 * 学号：2504020342 姓名：胡涛
 * 说明：请补全所有标记为 TODO 的函数体，不要修改其他代码。
 * 目录树查看器（仿 Linux tree 命令）
 * 完整实现版本（C语言，左孩子右兄弟二叉树）
 * 编译：gcc -o tree tree.c -std=c99
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// ================== 二叉树结点定义 ==================
typedef struct FileNode {
    char *name;                  // 文件/目录名
    int isDir;                   // 1:目录 0:文件
    struct FileNode *firstChild; // 左孩子：第一个子项
    struct FileNode *nextSibling;// 右兄弟：下一个同层项
} FileNode;

// ================== 函数声明 ==================
FileNode* createNode(const char *name, int isDir);
int cmpNode(const void *a, const void *b);
FileNode* buildTree(const char *path);
void printTree(FileNode *node, const char *prefix, int isLast);
int countNodes(FileNode *root);
int countLeaves(FileNode *root);
int treeHeight(FileNode *root);
void countDirFile(FileNode *root, int *dirs, int *files);
void freeTree(FileNode *root);
char* getBaseName(void);

// ================== 需要补全的函数 ==================

// 创建新结点（分配内存、复制字符串、初始化指针）
FileNode* createNode(const char *name, int isDir) {
    // 分配结点内存
    FileNode *node = (FileNode *)malloc(sizeof(FileNode));
    if (!node) {
        perror("malloc");
        return NULL;
    }
    
    // 分配并复制字符串名称
    node->name = (char *)malloc(strlen(name) + 1);
    if (!node->name) {
        perror("malloc");
        free(node);
        return NULL;
    }
    strcpy(node->name, name);
    
    // 初始化字段
    node->isDir = isDir;
    node->firstChild = NULL;
    node->nextSibling = NULL;
    
    return node;
}

// 比较函数，用于 qsort 对子项按名称排序
int cmpNode(const void *a, const void *b) {
    // 强转为指针
    FileNode *nodeA = *(FileNode **)a;
    FileNode *nodeB = *(FileNode **)b;
    
    // 按名称升序排序
    return strcmp(nodeA->name, nodeB->name);
}

// 递归构建目录树（核心难点）
FileNode* buildTree(const char *path) {
    // 打开目录
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return NULL;
    }
    
    // 从 path 中提取最后的目录名作为当前结点名
    const char *baseName = strrchr(path, '/');
    if (!baseName) {
        baseName = path;  // 相对路径或没有斜杠
    } else {
        baseName++;  // 跳过斜杠
    }
    if (*baseName == '\0') {  // 根目录"/"的情况
        baseName = "/";
    }
    
    // 创建当前目录结点
    FileNode *node = createNode(baseName, 1);
    if (!node) {
        closedir(dir);
        return NULL;
    }
    
    // 临时数组存储子结点
    FileNode **children = (FileNode **)malloc(sizeof(FileNode *) * 10000);
    if (!children) {
        perror("malloc");
        freeTree(node);
        closedir(dir);
        return NULL;
    }
    
    int childCount = 0;
    struct dirent *entry;
    
    // 循环 readdir，跳过 "." 和 ".."
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 拼接完整路径
        char fullPath[4096];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        
        // 用 stat 判断类型
        struct stat st;
        if (stat(fullPath, &st) != 0) {
            perror("stat");
            continue;
        }
        
        FileNode *child = NULL;
        
        // 若是目录，递归调用 buildTree；若是普通文件，调用 createNode
        if (S_ISDIR(st.st_mode)) {
            child = buildTree(fullPath);
        } else if (S_ISREG(st.st_mode)) {
            child = createNode(entry->d_name, 0);
        }
        
        // 将得到的子结点存入临时数组
        if (child) {
            children[childCount++] = child;
        }
    }
    
    // 关闭目录
    closedir(dir);
    
    // 对子结点数组排序（调用 qsort 和 cmpNode）
    if (childCount > 0) {
        qsort(children, childCount, sizeof(FileNode *), cmpNode);
        
        // 将排序后的子结点链接成兄弟链表
        // firstChild 指向第一个，后续 nextSibling
        node->firstChild = children[0];
        for (int i = 0; i < childCount - 1; i++) {
            children[i]->nextSibling = children[i + 1];
        }
    }
    
    // 释放临时数组
    free(children);
    
    return node;
}

// 树形输出（仿 tree 命令）
void printTree(FileNode *node, const char *prefix, int isLast) {
    // 如果 node 为空，返回
    if (!node) {
        return;
    }
    
    // 输出前缀、分支符号（isLast ? "`-- " : "|-- "）、结点名
    printf("%s%s%s", prefix, isLast ? "`-- " : "|-- ", node->name);
    
    // 如果是目录，输出 "/"
    if (node->isDir) {
        printf("/");
    }
    
    // 换行
    printf("\n");
    
    // 如果没有孩子，返回
    if (!node->firstChild) {
        return;
    }
    
    // 遍历孩子链表，对每个孩子：
    FileNode *child = node->firstChild;
    int childCount = 0;
    FileNode *tmp = child;
    while (tmp) {
        childCount++;
        tmp = tmp->nextSibling;
    }
    
    int idx = 0;
    while (child) {
        int isLastChild = (++idx == childCount);
        
        // 计算新前缀 = prefix + (isLast ? "    " : "|   ")
        char newPrefix[4096];
        snprintf(newPrefix, sizeof(newPrefix), "%s%s", prefix, isLast ? "    " : "|   ");
        
        // 递归调用 printTree
        printTree(child, newPrefix, isLastChild);
        
        child = child->nextSibling;
    }
}

// 统计二叉树结点总数
int countNodes(FileNode *root) {
    // 递归：1 + 左子树结点数 + 右子树结点数
    if (!root) {
        return 0;
    }
    
    // 统计左子树（第一个孩子及其所有兄弟）
    int count = 1;  // 当前结点
    
    // 对于二叉树表示，需要统计firstChild和所有nextSibling
    FileNode *child = root->firstChild;
    while (child) {
        count += countNodes(child);
        child = child->nextSibling;
    }
    
    return count;
}

// 统计叶子结点数（firstChild == NULL 的结点）
int countLeaves(FileNode *root) {
    // 递归
    if (!root) {
        return 0;
    }
    
    // 叶子结点定义：firstChild == NULL
    if (!root->firstChild) {
        return 1;
    }
    
    // 否则统计所有孩子的叶子数
    int leaves = 0;
    FileNode *child = root->firstChild;
    while (child) {
        leaves += countLeaves(child);
        child = child->nextSibling;
    }
    
    return leaves;
}

// 计算二叉树高度（根深度为1，空树高度为0）
int treeHeight(FileNode *root) {
    // 递归
    if (!root) {
        return 0;
    }
    
    // 计算最深的孩子的高度
    int maxChildHeight = 0;
    FileNode *child = root->firstChild;
    while (child) {
        int childHeight = treeHeight(child);
        if (childHeight > maxChildHeight) {
            maxChildHeight = childHeight;
        }
        child = child->nextSibling;
    }
    
    return 1 + maxChildHeight;
}

// 统计目录数和文件数（遍历整棵树）
void countDirFile(FileNode *root, int *dirs, int *files) {
    // 递归
    if (!root) {
        return;
    }
    
    // 统计当前结点
    if (root->isDir) {
        (*dirs)++;
    } else {
        (*files)++;
    }
    
    // 递归统计所有孩子
    FileNode *child = root->firstChild;
    while (child) {
        countDirFile(child, dirs, files);
        child = child->nextSibling;
    }
}

// 释放整棵树的内存
void freeTree(FileNode *root) {
    // 递归释放左右子树，最后释放当前结点
    if (!root) {
        return;
    }
    
    // 递归释放所有孩子
    FileNode *child = root->firstChild;
    while (child) {
        FileNode *nextSibling = child->nextSibling;
        freeTree(child);
        child = nextSibling;
    }
    
    // 释放当前结点的 name 和结点本身
    free(root->name);
    free(root);
}

// 获取当前工作目录的"基本名称"（用于显示根结点名）
char* getBaseName(void) {
    // 调用 getcwd(NULL,0) 获取绝对路径
    char *cwd = getcwd(NULL, 0);
    if (!cwd) {
        perror("getcwd");
        return NULL;
    }
    
    // 提取最后一个 '/' 之后的部分
    const char *baseName = strrchr(cwd, '/');
    if (!baseName) {
        baseName = cwd;
    } else {
        baseName++;
    }
    
    // 创建返回字符串
    char *result = (char *)malloc(strlen(baseName) + 1);
    if (!result) {
        perror("malloc");
        free(cwd);
        return NULL;
    }
    
    strcpy(result, baseName);
    
    // 注意释放 getcwd 分配的内存
    free(cwd);
    
    return result;
}

int main(int argc, char *argv[]) {
    char targetPath[1024];
    if (argc >= 2) {
        strncpy(targetPath, argv[1], sizeof(targetPath)-1);
        targetPath[sizeof(targetPath)-1] = '\0';
    } else {
        if (getcwd(targetPath, sizeof(targetPath)) == NULL) {
            perror("getcwd");
            return 1;
        }
    }

    int len = strlen(targetPath);
    if (len > 0 && targetPath[len-1] == '/')
        targetPath[len-1] = '\0';

    struct stat st;
    if (stat(targetPath, &st) != 0) {
        perror("stat");
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "错误: %s 不是目录\n", targetPath);
        return 1;
    }

    FileNode *root = buildTree(targetPath);
    if (!root) {
        fprintf(stderr, "无法构建目录树\n");
        return 1;
    }

    // 输出根目录名
    char *displayName = NULL;
    if (argc >= 2) {
        displayName = root->name;
    } else {
        displayName = getBaseName();
    }
    printf("%s/\n", displayName);
    if (argc < 2) free(displayName);

    FileNode *child = root->firstChild;
    int childCount = 0;
    FileNode *tmp = child;
    while (tmp) { childCount++; tmp = tmp->nextSibling; }
    int idx = 0;
    while (child) {
        int isLast = (++idx == childCount);
        printTree(child, "", isLast);
        child = child->nextSibling;
    }

    int dirs = 0, files = 0;
    countDirFile(root, &dirs, &files);
    printf("\n%d 个目录, %d 个文件\n", dirs, files);
    printf("二叉树结点总数: %d\n", countNodes(root));
    printf("叶子结点数: %d\n", countLeaves(root));
    printf("树的高度: %d\n", treeHeight(root));

    freeTree(root);
    return 0;
}
