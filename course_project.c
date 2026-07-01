// UTF-8 encoded file
/**
 * 实训.c — 选课管理系统（哈希表 + 双向链表 双模式实现）
 *
 * 支持运行时切换数据结构，性能对比测试同时覆盖两种结构。
 * 数据文件：D:\file.csv
 * 性能报告：D:\perf_compare.csv
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TABLE_SIZE 10007
#define EXPIRY_DATE "2023-09-01"
#define DATA_FILE "D:\\file.csv"

/* ==================== 公共数据类型 ==================== */

typedef struct Record
{
    char studentId[13];
    char name[30];
    char college[50];
    char courseId[9];
    char courseName[50];
    float credit;
    char semester[8];
    char selectDate[11];
    int score;
} Record;

typedef struct Filter
{
    char courseName[50];
    char semester[8];
    char college[50];
    int scoreMin;
    int scoreMax;
    int useFuzzyCourse;
} Filter;

typedef struct SortRule
{
    int type;
    int order;
} SortRule;

/* ==================== 模式定义 ==================== */

typedef enum
{
    MODE_HASH,
    MODE_LIST
} DataMode;
DataMode currentMode = MODE_HASH;

const char *modeName(void)
{
    return currentMode == MODE_HASH ? "哈希表" : "双向链表";
}

/* ==================== 哈希表类型 ==================== */

typedef struct Node_HT
{
    Record data;
    struct Node_HT *next;
} Node_HT;

typedef struct
{
    Node_HT *table[TABLE_SIZE];
} HashTable;

HashTable ht;

/* ==================== 链表类型 ==================== */

typedef struct Node_LL
{
    Record data;
    struct Node_LL *prev;
    struct Node_LL *next;
} Node_LL;

Node_LL *head = NULL;
Node_LL *tail = NULL;

/* ==================== 公共工具函数 ==================== */

void safeScanStr(char *buf, int size)
{
    fgets(buf, size, stdin);
    buf[strcspn(buf, "\n")] = '\0';
}

int loadCSVtoArray(const char *filePath, Record **outArr)
{
    FILE *fp = fopen(filePath, "r");
    int capacity = 100000, count = 0;
    char line[300];
    if (!fp)
    {
        printf("[错误] 无法打开文件: %s\n", filePath);
        printf("       请先运行 output.c 生成测试数据！\n");
        *outArr = NULL;
        return 0;
    }
    Record *arr = (Record *)malloc(capacity * sizeof(Record));
    if (!arr)
    {
        fclose(fp);
        *outArr = NULL;
        return 0;
    }
    fgets(line, sizeof(line), fp); /* 跳过表头 */
    while (fgets(line, sizeof(line), fp))
    {
        if (count >= capacity)
        {
            capacity *= 2;
            Record *tmp = (Record *)realloc(arr, capacity * sizeof(Record));
            if (!tmp)
            {
                free(arr);
                fclose(fp);
                *outArr = NULL;
                return 0;
            }
            arr = tmp;
        }
        int result = sscanf(line,
                            "%12[^,],%29[^,],%49[^,],%8[^,],%49[^,],%f,%7[^,],%10[^,],%d",
                            arr[count].studentId, arr[count].name, arr[count].college,
                            arr[count].courseId, arr[count].courseName, &arr[count].credit,
                            arr[count].semester, arr[count].selectDate, &arr[count].score);
        if (result == 9)
            count++;
    }
    fclose(fp);
    *outArr = arr;
    return count;
}

/* ================================================================
 *  哈希表实现（ht_ 前缀）
 * ================================================================ */

unsigned int ht_hash(const char *studentId, const char *courseId)
{
    unsigned long hashValue = 0;
    while (*studentId)
    {
        hashValue = hashValue * 31 + (*studentId++);
    }
    while (*courseId)
    {
        hashValue = hashValue * 31 + (*courseId++);
    }
    return hashValue % TABLE_SIZE;
}

void ht_initTable(HashTable *t)
{
    int i;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        t->table[i] = NULL;
    }
}

Node_HT *ht_findRecord(HashTable *t, const char *studentId, const char *courseId)
{
    unsigned int index = ht_hash(studentId, courseId);
    Node_HT *p = t->table[index];
    while (p)
    {
        if (strcmp(p->data.studentId, studentId) == 0 &&
            strcmp(p->data.courseId, courseId) == 0)
            return p;
        p = p->next;
    }
    return NULL;
}

void ht_insertRecord(HashTable *t, Record r)
{
    if (ht_findRecord(t, r.studentId, r.courseId) != NULL)
    {
        printf("记录已存在！\n");
        return;
    }
    unsigned int index = ht_hash(r.studentId, r.courseId);
    Node_HT *newNode = (Node_HT *)malloc(sizeof(Node_HT));
    if (!newNode)
    {
        printf("内存分配失败\n");
        return;
    }
    newNode->data = r;
    newNode->next = t->table[index];
    t->table[index] = newNode;
}

int ht_deleteRecord(HashTable *t, const char *studentId, const char *courseId)
{
    unsigned int index = ht_hash(studentId, courseId);
    Node_HT *p = t->table[index], *prev = NULL;
    while (p)
    {
        if (strcmp(p->data.studentId, studentId) == 0 &&
            strcmp(p->data.courseId, courseId) == 0)
        {
            if (prev == NULL)
                t->table[index] = p->next;
            else
                prev->next = p->next;
            free(p);
            return 1;
        }
        prev = p;
        p = p->next;
    }
    return 0;
}

int ht_updateRecord(HashTable *t, const char *studentId, const char *courseId, int newScore)
{
    Node_HT *node = ht_findRecord(t, studentId, courseId);
    if (node)
    {
        node->data.score = newScore;
        return 1;
    }
    return 0;
}

void ht_destroyTable(HashTable *t)
{
    int i;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *p = t->table[i];
        while (p)
        {
            Node_HT *tmp = p;
            p = p->next;
            free(tmp);
        }
        t->table[i] = NULL;
    }
}

int ht_countAll(HashTable *t)
{
    int i, count = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *p = t->table[i];
        while (p)
        {
            count++;
            p = p->next;
        }
    }
    return count;
}

int ht_collectAll(HashTable *t, Record *arr, int maxSize)
{
    int i, idx = 0;
    for (i = 0; i < TABLE_SIZE && idx < maxSize; i++)
    {
        Node_HT *p = t->table[i];
        while (p && idx < maxSize)
        {
            arr[idx++] = p->data;
            p = p->next;
        }
    }
    return idx;
}

void ht_statChainLength(HashTable *t, int *maxLen, double *avgLen, int *emptyBuckets)
{
    int i;
    *maxLen = 0;
    double sum = 0;
    *emptyBuckets = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *p = t->table[i];
        int len = 0;
        while (p)
        {
            len++;
            p = p->next;
        }
        if (len > *maxLen)
            *maxLen = len;
        if (len == 0)
            (*emptyBuckets)++;
        sum += len;
    }
    *avgLen = sum / TABLE_SIZE;
}

/* ---- 哈希表查找 ---- */

void ht_searchByStudentId(HashTable *t, char studentId[])
{
    int i, flag = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            if (strcmp(cur->data.studentId, studentId) == 0)
            {
                printf("%s %s %s %d\n", cur->data.studentId,
                       cur->data.name, cur->data.courseName, cur->data.score);
                flag = 1;
            }
            cur = cur->next;
        }
    }
    if (!flag)
        printf("未找到记录\n");
}

void ht_searchByNameExact(HashTable *t, char name[])
{
    int i, flag = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            if (strcmp(cur->data.name, name) == 0)
            {
                printf("%s %s %s %d\n", cur->data.studentId,
                       cur->data.name, cur->data.courseName, cur->data.score);
                flag = 1;
            }
            cur = cur->next;
        }
    }
    if (!flag)
        printf("未找到记录\n");
}

void ht_searchByNameFuzzy(HashTable *t, char keyword[])
{
    int i, flag = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            if (strstr(cur->data.name, keyword))
            {
                printf("%s %s %s %d\n", cur->data.studentId,
                       cur->data.name, cur->data.courseName, cur->data.score);
                flag = 1;
            }
            cur = cur->next;
        }
    }
    if (!flag)
        printf("未找到记录\n");
}

void ht_searchByCourseExact(HashTable *t, char courseName[])
{
    int i, flag = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            if (strcmp(cur->data.courseName, courseName) == 0)
            {
                printf("%s %s %s %d\n", cur->data.studentId,
                       cur->data.name, cur->data.courseName, cur->data.score);
                flag = 1;
            }
            cur = cur->next;
        }
    }
    if (!flag)
        printf("未找到记录\n");
}

void ht_searchByCourseFuzzy(HashTable *t, char keyword[])
{
    int i, flag = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            if (strstr(cur->data.courseName, keyword))
            {
                printf("%s %s %s %d\n", cur->data.studentId,
                       cur->data.name, cur->data.courseName, cur->data.score);
                flag = 1;
            }
            cur = cur->next;
        }
    }
    if (!flag)
        printf("未找到记录\n");
}

/* ---- 哈希表持久化 ---- */

void ht_printAll(HashTable *t)
{
    int i;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            printf("%s %s %s %s %s %.1f %s %s %d\n",
                   cur->data.studentId, cur->data.name, cur->data.college,
                   cur->data.courseId, cur->data.courseName, cur->data.credit,
                   cur->data.semester, cur->data.selectDate, cur->data.score);
            cur = cur->next;
        }
    }
}

void ht_saveToFile(HashTable *t)
{
    FILE *fp = fopen(DATA_FILE, "w");
    int i;
    if (!fp)
    {
        printf("文件打开失败！\n");
        return;
    }
    fprintf(fp, "studentId,name,college,courseId,courseName,credit,semester,selectDate,score\n");
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *p = t->table[i];
        while (p)
        {
            fprintf(fp, "%s,%s,%s,%s,%s,%.1f,%s,%s,%d\n",
                    p->data.studentId, p->data.name, p->data.college,
                    p->data.courseId, p->data.courseName, p->data.credit,
                    p->data.semester, p->data.selectDate, p->data.score);
            p = p->next;
        }
    }
    fclose(fp);
    printf("数据已保存至 %s\n", DATA_FILE);
}

void ht_loadFromFile(HashTable *t)
{
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp)
    {
        printf("数据文件 %s 不存在，将创建空哈希表。\n", DATA_FILE);
        return;
    }
    char line[300];
    fgets(line, sizeof(line), fp); /* 跳过表头 */
    while (fgets(line, sizeof(line), fp))
    {
        Record r;
        int result = sscanf(line,
                            "%12[^,],%29[^,],%49[^,],%8[^,],%49[^,],%f,%7[^,],%10[^,],%d",
                            r.studentId, r.name, r.college, r.courseId, r.courseName,
                            &r.credit, r.semester, r.selectDate, &r.score);
        if (result == 9)
            ht_insertRecord(t, r);
    }
    fclose(fp);
    printf("数据加载完成（来源: %s）。\n", DATA_FILE);
}

/* ---- 哈希表筛选与排序 ---- */

int ht_matchFilter(Record *r, Filter f)
{
    if (strlen(f.courseName) > 0)
    {
        if (f.useFuzzyCourse)
        {
            if (!strstr(r->courseName, f.courseName))
                return 0;
        }
        else
        {
            if (strcmp(r->courseName, f.courseName) != 0)
                return 0;
        }
    }
    if (strlen(f.semester) > 0 && strcmp(r->semester, f.semester) != 0)
        return 0;
    if (strlen(f.college) > 0 && strcmp(r->college, f.college) != 0)
        return 0;
    if (f.scoreMin != -1 && r->score < f.scoreMin)
        return 0;
    if (f.scoreMax != -1 && r->score > f.scoreMax)
        return 0;
    return 1;
}

void ht_filterAndExport(HashTable *t, Filter f)
{
    FILE *fp = fopen("D:\\filter_result.csv", "w");
    int i, count = 0;
    if (!fp)
    {
        printf("文件打开失败\n");
        return;
    }
    fprintf(fp, "学号,姓名,学院,课程编号,课程名称,学分,学期,选课日期,成绩\n");
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            if (ht_matchFilter(&cur->data, f))
            {
                fprintf(fp, "%s,%s,%s,%s,%s,%.1f,%s,%s,%d\n",
                        cur->data.studentId, cur->data.name, cur->data.college,
                        cur->data.courseId, cur->data.courseName, cur->data.credit,
                        cur->data.semester, cur->data.selectDate, cur->data.score);
                count++;
            }
            cur = cur->next;
        }
    }
    fclose(fp);
    printf("筛选完成，共 %d 条记录，已导出至 D:\\filter_result.csv\n", count);
}

int ht_compareRecords(Record *a, Record *b, SortRule rules[], int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        int result = 0;
        if (rules[i].type == 0)
            result = a->score - b->score;
        else if (rules[i].type == 1)
            result = strcmp(a->studentId, b->studentId);
        else if (rules[i].type == 2)
            result = strcmp(a->courseName, b->courseName);
        if (result != 0)
            return rules[i].order ? result : -result;
    }
    return 0;
}

void ht_sortRecords(HashTable *t, SortRule rules[], int n)
{
    int i, total, swapped;
    total = ht_countAll(t);
    if (total == 0)
    {
        printf("暂无数据，无法排序！\n");
        return;
    }
    Record *arr = (Record *)malloc(total * sizeof(Record));
    if (!arr)
    {
        printf("内存分配失败\n");
        return;
    }
    ht_collectAll(t, arr, total);
    do
    {
        swapped = 0;
        for (i = 0; i < total - 1; i++)
        {
            if (ht_compareRecords(&arr[i], &arr[i + 1], rules, n) > 0)
            {
                Record temp = arr[i];
                arr[i] = arr[i + 1];
                arr[i + 1] = temp;
                swapped = 1;
            }
        }
    } while (swapped);
    printf("\n--- 排序结果（共 %d 条）---\n", total);
    for (i = 0; i < total; i++)
        printf("%s %s %s %s %d\n", arr[i].studentId, arr[i].name,
               arr[i].courseId, arr[i].courseName, arr[i].score);
    free(arr);
}

/* ---- 哈希表统计 ---- */

void ht_stat_course_count(Record arr[], int n)
{
    char courseId[200][9];
    int count[200] = {0};
    int i, j, m = 0;
    for (i = 0; i < n; i++)
    {
        int found = 0;
        for (j = 0; j < m; j++)
        {
            if (strcmp(courseId[j], arr[i].courseId) == 0)
            {
                count[j]++;
                found = 1;
                break;
            }
        }
        if (!found && m < 200)
        {
            strcpy(courseId[m], arr[i].courseId);
            count[m] = 1;
            m++;
        }
    }
    printf("\n--- 每门课程选课人数 ---\n");
    for (i = 0; i < m; i++)
        printf("课程 %s: %d 人\n", courseId[i], count[i]);
}

void ht_stat_student_credit(Record arr[], int n)
{
    char stuId[200][13];
    int courseCount[200] = {0};
    float creditSum[200] = {0};
    int i, j, m = 0;
    for (i = 0; i < n; i++)
    {
        int found = 0;
        for (j = 0; j < m; j++)
        {
            if (strcmp(stuId[j], arr[i].studentId) == 0)
            {
                courseCount[j]++;
                creditSum[j] += arr[i].credit;
                found = 1;
                break;
            }
        }
        if (!found && m < 200)
        {
            strcpy(stuId[m], arr[i].studentId);
            courseCount[m] = 1;
            creditSum[m] = arr[i].credit;
            m++;
        }
    }
    printf("\n--- 每位学生选课统计 ---\n");
    printf("%-13s %-8s %-8s\n", "学号", "课程数", "总学分");
    for (i = 0; i < m; i++)
        printf("%-13s %-8d %-8.1f\n", stuId[i], courseCount[i], creditSum[i]);
}

void ht_stat_college_count(Record arr[], int n)
{
    char college[100][50];
    int count[100] = {0};
    int i, j, m = 0;
    for (i = 0; i < n; i++)
    {
        int found = 0;
        for (j = 0; j < m; j++)
        {
            if (strcmp(college[j], arr[i].college) == 0)
            {
                count[j]++;
                found = 1;
                break;
            }
        }
        if (!found && m < 100)
        {
            strcpy(college[m], arr[i].college);
            count[m] = 1;
            m++;
        }
    }
    printf("\n--- 各学院选课人数分布 ---\n");
    for (i = 0; i < m; i++)
        printf("%s: %d 人\n", college[i], count[i]);
}

void ht_stat_semester_count(Record arr[], int n)
{
    char semesters[50][8];
    int totalCount[50] = {0};
    char courseSet[50][200][9];
    int courseNum[50] = {0};
    int i, j, k, m = 0;
    for (i = 0; i < n; i++)
    {
        int semIdx = -1;
        for (j = 0; j < m; j++)
        {
            if (strcmp(semesters[j], arr[i].semester) == 0)
            {
                semIdx = j;
                break;
            }
        }
        if (semIdx == -1 && m < 50)
        {
            semIdx = m;
            strcpy(semesters[m], arr[i].semester);
            m++;
        }
        if (semIdx == -1)
            continue;
        totalCount[semIdx]++;
        int courseFound = 0;
        for (k = 0; k < courseNum[semIdx]; k++)
        {
            if (strcmp(courseSet[semIdx][k], arr[i].courseId) == 0)
            {
                courseFound = 1;
                break;
            }
        }
        if (!courseFound && courseNum[semIdx] < 200)
        {
            strcpy(courseSet[semIdx][courseNum[semIdx]], arr[i].courseId);
            courseNum[semIdx]++;
        }
    }
    printf("\n--- 按学期统计 ---\n");
    printf("%-10s %-12s %-10s\n", "学期", "选课总人次", "课程数");
    for (i = 0; i < m; i++)
        printf("%-10s %-12d %-10d\n", semesters[i], totalCount[i], courseNum[i]);
}

void ht_stat_score_distribution(Record arr[], int n)
{
    int i, excellent = 0, good = 0, medium = 0, pass = 0, fail = 0;
    for (i = 0; i < n; i++)
    {
        int s = arr[i].score;
        if (s >= 90)
            excellent++;
        else if (s >= 80)
            good++;
        else if (s >= 70)
            medium++;
        else if (s >= 60)
            pass++;
        else
            fail++;
    }
    printf("\n--- 课程成绩分布统计 ---\n");
    printf("优秀 (90-100): %d 人 (%.1f%%)\n", excellent, n > 0 ? 100.0 * excellent / n : 0);
    printf("良好 (80-89) : %d 人 (%.1f%%)\n", good, n > 0 ? 100.0 * good / n : 0);
    printf("中等 (70-79) : %d 人 (%.1f%%)\n", medium, n > 0 ? 100.0 * medium / n : 0);
    printf("及格 (60-69) : %d 人 (%.1f%%)\n", pass, n > 0 ? 100.0 * pass / n : 0);
    printf("不及格 (<60) : %d 人 (%.1f%%)\n", fail, n > 0 ? 100.0 * fail / n : 0);
    printf("总计: %d 人\n", n);
}

void ht_runStatistics(HashTable *t)
{
    int n = ht_countAll(t);
    if (n == 0)
    {
        printf("暂无数据！\n");
        return;
    }
    Record *arr = (Record *)malloc(n * sizeof(Record));
    if (!arr)
    {
        printf("内存分配失败\n");
        return;
    }
    ht_collectAll(t, arr, n);
    ht_stat_course_count(arr, n);
    ht_stat_student_credit(arr, n);
    ht_stat_college_count(arr, n);
    ht_stat_semester_count(arr, n);
    ht_stat_score_distribution(arr, n);
    free(arr);
}

/* ---- 哈希表过期处理 ---- */

int ht_isExpired(char selectDate[]) { return strcmp(selectDate, EXPIRY_DATE) < 0; }

int ht_countExpiredRecords(HashTable *t)
{
    int i, count = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i];
        while (cur)
        {
            if (ht_isExpired(cur->data.selectDate))
                count++;
            cur = cur->next;
        }
    }
    return count;
}

int ht_batchDeleteExpired(HashTable *t)
{
    int i, deleted;
    int expiredCount = ht_countExpiredRecords(t);
    if (expiredCount == 0)
    {
        printf("\n没有发现过期记录（选课日期早于 %s）\n", EXPIRY_DATE);
        return 0;
    }
    printf("\n========================================\n");
    printf("  即将删除 %d 条过期选课记录\n", expiredCount);
    printf("  过期条件：选课日期早于 %s\n", EXPIRY_DATE);
    printf("========================================\n");
    printf("确认删除？(y/n)：");
    char confirm;
    scanf(" %c", &confirm);
    if (confirm != 'y' && confirm != 'Y')
    {
        printf("已取消删除操作。\n");
        return 0;
    }
    deleted = 0;
    for (i = 0; i < TABLE_SIZE; i++)
    {
        Node_HT *cur = t->table[i], *prev = NULL;
        while (cur)
        {
            if (ht_isExpired(cur->data.selectDate))
            {
                Node_HT *toDelete = cur;
                if (!prev)
                {
                    t->table[i] = cur->next;
                    cur = t->table[i];
                }
                else
                {
                    prev->next = cur->next;
                    cur = cur->next;
                }
                free(toDelete);
                deleted++;
            }
            else
            {
                prev = cur;
                cur = cur->next;
            }
        }
    }
    ht_saveToFile(t);
    printf("\n删除完成！共删除 %d 条过期记录。\n", deleted);
    return deleted;
}

/* ================================================================
 *  双向链表实现（ll_ 前缀）
 * ================================================================ */

void ll_insertRecord(Record r)
{
    Node_LL *newNode = (Node_LL *)malloc(sizeof(Node_LL));
    newNode->data = r;
    newNode->next = NULL;
    if (head == NULL)
    {
        newNode->prev = NULL;
        head = tail = newNode;
    }
    else
    {
        newNode->prev = tail;
        tail->next = newNode;
        tail = newNode;
    }
}

Node_LL *ll_findRecord(const char *studentId, const char *courseId)
{
    Node_LL *cur = head;
    while (cur)
    {
        if (strcmp(cur->data.studentId, studentId) == 0 &&
            strcmp(cur->data.courseId, courseId) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

int ll_deleteRecord(const char *studentId, const char *courseId)
{
    Node_LL *p = ll_findRecord(studentId, courseId);
    if (p == NULL)
        return 0;
    if (p == head)
        head = head->next;
    if (p == tail)
        tail = p->prev;
    if (p->prev)
        p->prev->next = p->next;
    if (p->next)
        p->next->prev = p->prev;
    free(p);
    return 1;
}

int ll_updateRecord(const char *studentId, const char *courseId, int newScore)
{
    Node_LL *p = ll_findRecord(studentId, courseId);
    if (p == NULL)
        return 0;
    p->data.score = newScore;
    return 1;
}

int ll_listLength(void)
{
    int n = 0;
    Node_LL *cur = head;
    while (cur)
    {
        n++;
        cur = cur->next;
    }
    return n;
}

void ll_clearList(void)
{
    Node_LL *cur = head;
    while (cur)
    {
        Node_LL *t = cur;
        cur = cur->next;
        free(t);
    }
    head = tail = NULL;
}

/* ---- 链表查找 ---- */

void ll_searchByStudentId(const char studentId[])
{
    Node_LL *cur = head;
    int flag = 0;
    while (cur)
    {
        if (strcmp(cur->data.studentId, studentId) == 0)
        {
            printf("%s %s %s %d\n", cur->data.studentId,
                   cur->data.name, cur->data.courseName, cur->data.score);
            flag = 1;
        }
        cur = cur->next;
    }
    if (!flag)
        printf("未找到记录\n");
}

void ll_searchByNameExact(const char name[])
{
    Node_LL *cur = head;
    int flag = 0;
    while (cur)
    {
        if (strcmp(cur->data.name, name) == 0)
        {
            printf("%s %s %s %d\n", cur->data.studentId,
                   cur->data.name, cur->data.courseName, cur->data.score);
            flag = 1;
        }
        cur = cur->next;
    }
    if (!flag)
        printf("未找到记录\n");
}

void ll_searchByNameFuzzy(const char keyword[])
{
    Node_LL *cur = head;
    int flag = 0;
    while (cur)
    {
        if (strstr(cur->data.name, keyword))
        {
            printf("%s %s %s %d\n", cur->data.studentId,
                   cur->data.name, cur->data.courseName, cur->data.score);
            flag = 1;
        }
        cur = cur->next;
    }
    if (!flag)
        printf("未找到记录\n");
}

void ll_searchByCourseExact(const char courseName[])
{
    Node_LL *cur = head;
    int flag = 0;
    while (cur)
    {
        if (strcmp(cur->data.courseName, courseName) == 0)
        {
            printf("%s %s %s %d\n", cur->data.studentId,
                   cur->data.name, cur->data.courseName, cur->data.score);
            flag = 1;
        }
        cur = cur->next;
    }
    if (!flag)
        printf("未找到记录\n");
}

void ll_searchByCourseFuzzy(const char keyword[])
{
    Node_LL *cur = head;
    int flag = 0;
    while (cur)
    {
        if (strstr(cur->data.courseName, keyword))
        {
            printf("%s %s %s %d\n", cur->data.studentId,
                   cur->data.name, cur->data.courseName, cur->data.score);
            flag = 1;
        }
        cur = cur->next;
    }
    if (!flag)
        printf("未找到记录\n");
}

/* ---- 链表持久化 ---- */

void ll_printAll(void)
{
    Node_LL *cur = head;
    while (cur)
    {
        printf("%s %s %s %s %s %.1f %s %s %d\n",
               cur->data.studentId, cur->data.name, cur->data.college,
               cur->data.courseId, cur->data.courseName, cur->data.credit,
               cur->data.semester, cur->data.selectDate, cur->data.score);
        cur = cur->next;
    }
}

void ll_saveToFile(void)
{
    FILE *fp = fopen(DATA_FILE, "w");
    if (!fp)
    {
        printf("文件打开失败\n");
        return;
    }
    fprintf(fp, "studentId,name,college,courseId,courseName,credit,semester,selectDate,score\n");
    Node_LL *cur = head;
    while (cur)
    {
        fprintf(fp, "%s,%s,%s,%s,%s,%.1f,%s,%s,%d\n",
                cur->data.studentId, cur->data.name, cur->data.college,
                cur->data.courseId, cur->data.courseName, cur->data.credit,
                cur->data.semester, cur->data.selectDate, cur->data.score);
        cur = cur->next;
    }
    fclose(fp);
}

void ll_loadFromFile(void)
{
    FILE *fp = fopen(DATA_FILE, "r");
    if (!fp)
    {
        printf("数据文件 %s 不存在，创建空链表\n", DATA_FILE);
        return;
    }
    char line[300];
    fgets(line, sizeof(line), fp); /* 跳过表头 */
    while (fgets(line, sizeof(line), fp))
    {
        Record r;
        int result = sscanf(line,
                            "%12[^,],%29[^,],%49[^,],%8[^,],%49[^,],%f,%7[^,],%10[^,],%d",
                            r.studentId, r.name, r.college, r.courseId, r.courseName,
                            &r.credit, r.semester, r.selectDate, &r.score);
        if (result == 9)
            ll_insertRecord(r);
    }
    fclose(fp);
    printf("数据加载完成（来源: %s）。\n", DATA_FILE);
}

/* ---- 链表筛选与排序 ---- */

int ll_matchFilter(Node_LL *cur, Filter f)
{
    if (strlen(f.courseName) > 0)
    {
        if (f.useFuzzyCourse)
        {
            if (!strstr(cur->data.courseName, f.courseName))
                return 0;
        }
        else
        {
            if (strcmp(cur->data.courseName, f.courseName) != 0)
                return 0;
        }
    }
    if (strlen(f.semester) > 0 && strcmp(cur->data.semester, f.semester) != 0)
        return 0;
    if (strlen(f.college) > 0 && strcmp(cur->data.college, f.college) != 0)
        return 0;
    if (f.scoreMin != -1 && cur->data.score < f.scoreMin)
        return 0;
    if (f.scoreMax != -1 && cur->data.score > f.scoreMax)
        return 0;
    return 1;
}

void ll_filterAndExport(Filter f)
{
    FILE *fp = fopen("D:\\filter_result.csv", "w");
    Node_LL *cur = head;
    int count = 0;
    if (!fp)
    {
        printf("文件打开失败\n");
        return;
    }
    fprintf(fp, "学号,姓名,学院,课程编号,课程名称,学分,学期,选课日期,成绩\n");
    while (cur)
    {
        if (ll_matchFilter(cur, f))
        {
            fprintf(fp, "%s,%s,%s,%s,%s,%.1f,%s,%s,%d\n",
                    cur->data.studentId, cur->data.name, cur->data.college,
                    cur->data.courseId, cur->data.courseName, cur->data.credit,
                    cur->data.semester, cur->data.selectDate, cur->data.score);
            count++;
        }
        cur = cur->next;
    }
    fclose(fp);
    printf("筛选完成，共 %d 条记录，已导出至 D:\\filter_result.csv\n", count);
}

int ll_compare(Node_LL *a, Node_LL *b, SortRule rules[], int n)
{
    int i;
    for (i = 0; i < n; i++)
    {
        int result = 0;
        if (rules[i].type == 0)
            result = a->data.score - b->data.score;
        else if (rules[i].type == 1)
            result = strcmp(a->data.studentId, b->data.studentId);
        else if (rules[i].type == 2)
            result = strcmp(a->data.courseName, b->data.courseName);
        if (result != 0)
            return rules[i].order ? result : -result;
    }
    return 0;
}

void ll_sortRecords(SortRule rules[], int n)
{
    Node_LL *ptr;
    int swapped;
    if (!head)
    {
        printf("暂无数据，无法排序！\n");
        return;
    }
    do
    {
        swapped = 0;
        ptr = head;
        while (ptr->next)
        {
            if (ll_compare(ptr, ptr->next, rules, n) > 0)
            {
                Record temp = ptr->data;
                ptr->data = ptr->next->data;
                ptr->next->data = temp;
                swapped = 1;
            }
            ptr = ptr->next;
        }
    } while (swapped);
    printf("\n--- 排序结果（共 %d 条）---\n", ll_listLength());
    ptr = head;
    while (ptr)
    {
        printf("%s %s %s %s %d\n", ptr->data.studentId, ptr->data.name,
               ptr->data.courseId, ptr->data.courseName, ptr->data.score);
        ptr = ptr->next;
    }
}

/* ---- 链表统计 ---- */

void ll_stat_course_count(Record arr[], int n)
{
    char courseId[200][9];
    int count[200] = {0};
    int i, j, m = 0;
    for (i = 0; i < n; i++)
    {
        int found = 0;
        for (j = 0; j < m; j++)
        {
            if (strcmp(courseId[j], arr[i].courseId) == 0)
            {
                count[j]++;
                found = 1;
                break;
            }
        }
        if (!found && m < 200)
        {
            strcpy(courseId[m], arr[i].courseId);
            count[m] = 1;
            m++;
        }
    }
    printf("\n--- 每门课程选课人数 ---\n");
    for (i = 0; i < m; i++)
        printf("课程 %s: %d 人\n", courseId[i], count[i]);
}

void ll_stat_student_credit(Record arr[], int n)
{
    char stuId[200][13];
    int courseCount[200] = {0};
    float creditSum[200] = {0};
    int i, j, m = 0;
    for (i = 0; i < n; i++)
    {
        int found = 0;
        for (j = 0; j < m; j++)
        {
            if (strcmp(stuId[j], arr[i].studentId) == 0)
            {
                courseCount[j]++;
                creditSum[j] += arr[i].credit;
                found = 1;
                break;
            }
        }
        if (!found && m < 200)
        {
            strcpy(stuId[m], arr[i].studentId);
            courseCount[m] = 1;
            creditSum[m] = arr[i].credit;
            m++;
        }
    }
    printf("\n--- 每位学生选课统计 ---\n");
    printf("%-13s %-8s %-8s\n", "学号", "课程数", "总学分");
    for (i = 0; i < m; i++)
        printf("%-13s %-8d %-8.1f\n", stuId[i], courseCount[i], creditSum[i]);
}

void ll_stat_college_count(Record arr[], int n)
{
    char college[100][50];
    int count[100] = {0};
    int i, j, m = 0;
    for (i = 0; i < n; i++)
    {
        int found = 0;
        for (j = 0; j < m; j++)
        {
            if (strcmp(college[j], arr[i].college) == 0)
            {
                count[j]++;
                found = 1;
                break;
            }
        }
        if (!found && m < 100)
        {
            strcpy(college[m], arr[i].college);
            count[m] = 1;
            m++;
        }
    }
    printf("\n--- 各学院选课人数分布 ---\n");
    for (i = 0; i < m; i++)
        printf("%s: %d 人\n", college[i], count[i]);
}

void ll_stat_semester_distribution(Record arr[], int n)
{
    char semesters[100][8];
    int totalCount[100] = {0};
    char courseSet[100][100][9];
    int courseNum[100] = {0};
    int i, j, k, m = 0;
    for (i = 0; i < n; i++)
    {
        int semIdx = -1;
        for (j = 0; j < m; j++)
        {
            if (strcmp(semesters[j], arr[i].semester) == 0)
            {
                semIdx = j;
                break;
            }
        }
        if (semIdx == -1 && m < 100)
        {
            semIdx = m;
            strcpy(semesters[m], arr[i].semester);
            m++;
        }
        if (semIdx == -1)
            continue;
        totalCount[semIdx]++;
        int courseExists = 0;
        for (k = 0; k < courseNum[semIdx]; k++)
        {
            if (strcmp(courseSet[semIdx][k], arr[i].courseId) == 0)
            {
                courseExists = 1;
                break;
            }
        }
        if (!courseExists && courseNum[semIdx] < 100)
        {
            strcpy(courseSet[semIdx][courseNum[semIdx]], arr[i].courseId);
            courseNum[semIdx]++;
        }
    }
    printf("\n--- 按学期选课统计 ---\n");
    for (i = 0; i < m; i++)
        printf("学期 %s: 选课总人次=%d, 课程数=%d\n",
               semesters[i], totalCount[i], courseNum[i]);
}

void ll_stat_score_distribution(Record arr[], int n)
{
    int i, excellent = 0, good = 0, medium = 0, pass = 0, fail = 0;
    for (i = 0; i < n; i++)
    {
        int s = arr[i].score;
        if (s >= 90)
            excellent++;
        else if (s >= 80)
            good++;
        else if (s >= 70)
            medium++;
        else if (s >= 60)
            pass++;
        else
            fail++;
    }
    printf("\n--- 课程成绩分布统计 ---\n");
    printf("优秀 (90-100): %d 人 (%.1f%%)\n", excellent, n > 0 ? 100.0 * excellent / n : 0);
    printf("良好 (80-89) : %d 人 (%.1f%%)\n", good, n > 0 ? 100.0 * good / n : 0);
    printf("中等 (70-79) : %d 人 (%.1f%%)\n", medium, n > 0 ? 100.0 * medium / n : 0);
    printf("及格 (60-69) : %d 人 (%.1f%%)\n", pass, n > 0 ? 100.0 * pass / n : 0);
    printf("不及格 (<60) : %d 人 (%.1f%%)\n", fail, n > 0 ? 100.0 * fail / n : 0);
    printf("总计: %d 人\n", n);
}

void ll_runStatistics(void)
{
    int n = ll_listLength(), i;
    if (n == 0)
    {
        printf("暂无数据！\n");
        return;
    }
    Record *arr = (Record *)malloc(n * sizeof(Record));
    Node_LL *cur = head;
    i = 0;
    while (cur)
    {
        arr[i++] = cur->data;
        cur = cur->next;
    }
    ll_stat_course_count(arr, n);
    ll_stat_student_credit(arr, n);
    ll_stat_college_count(arr, n);
    ll_stat_semester_distribution(arr, n);
    ll_stat_score_distribution(arr, n);
    free(arr);
}

/* ---- 链表过期处理 ---- */

int ll_isExpired(char selectDate[]) { return strcmp(selectDate, EXPIRY_DATE) < 0; }

int ll_countExpiredRecords(void)
{
    int count = 0;
    Node_LL *cur = head;
    while (cur)
    {
        if (ll_isExpired(cur->data.selectDate))
            count++;
        cur = cur->next;
    }
    return count;
}

int ll_batchDeleteExpired(void)
{
    int expiredCount = ll_countExpiredRecords();
    int deleted;
    if (expiredCount == 0)
    {
        printf("\n没有发现过期记录（选课日期早于 %s）\n", EXPIRY_DATE);
        return 0;
    }
    printf("\n========================================\n");
    printf("  即将删除 %d 条过期选课记录\n", expiredCount);
    printf("  过期条件：选课日期早于 %s\n", EXPIRY_DATE);
    printf("========================================\n");
    printf("确认删除？(y/n)：");
    char confirm;
    scanf(" %c", &confirm);
    if (confirm != 'y' && confirm != 'Y')
    {
        printf("已取消删除操作。\n");
        return 0;
    }
    deleted = 0;
    {
        Node_LL *cur = head;
        while (cur)
        {
            Node_LL *next = cur->next;
            if (ll_isExpired(cur->data.selectDate))
            {
                if (cur->prev)
                    cur->prev->next = cur->next;
                else
                    head = cur->next;
                if (cur->next)
                    cur->next->prev = cur->prev;
                else
                    tail = cur->prev;
                free(cur);
                deleted++;
            }
            cur = next;
        }
    }
    ll_saveToFile();
    printf("\n删除完成！共删除 %d 条过期记录。\n", deleted);
    return deleted;
}

/* ================================================================
 *  调度层 —— 根据 currentMode 分发到哈希表或链表实现
 * ================================================================ */

void insertRecord(Record r)
{
    if (currentMode == MODE_HASH)
        ht_insertRecord(&ht, r);
    else
        ll_insertRecord(r);
}

int deleteRecord(const char *sid, const char *cid)
{
    if (currentMode == MODE_HASH)
        return ht_deleteRecord(&ht, sid, cid);
    else
        return ll_deleteRecord(sid, cid);
}

int updateRecord(const char *sid, const char *cid, int newScore)
{
    if (currentMode == MODE_HASH)
        return ht_updateRecord(&ht, sid, cid, newScore);
    else
        return ll_updateRecord(sid, cid, newScore);
}

void searchByStudentId(const char *sid)
{
    if (currentMode == MODE_HASH)
        ht_searchByStudentId(&ht, (char *)sid);
    else
        ll_searchByStudentId(sid);
}

void searchByNameExact(const char *name)
{
    if (currentMode == MODE_HASH)
        ht_searchByNameExact(&ht, (char *)name);
    else
        ll_searchByNameExact(name);
}

void searchByNameFuzzy(const char *kw)
{
    if (currentMode == MODE_HASH)
        ht_searchByNameFuzzy(&ht, (char *)kw);
    else
        ll_searchByNameFuzzy(kw);
}

void searchByCourseExact(const char *cn)
{
    if (currentMode == MODE_HASH)
        ht_searchByCourseExact(&ht, (char *)cn);
    else
        ll_searchByCourseExact(cn);
}

void searchByCourseFuzzy(const char *kw)
{
    if (currentMode == MODE_HASH)
        ht_searchByCourseFuzzy(&ht, (char *)kw);
    else
        ll_searchByCourseFuzzy(kw);
}

void printAll(void)
{
    if (currentMode == MODE_HASH)
        ht_printAll(&ht);
    else
        ll_printAll();
}

int getTotalCount(void)
{
    if (currentMode == MODE_HASH)
        return ht_countAll(&ht);
    else
        return ll_listLength();
}

void saveToFile(void)
{
    if (currentMode == MODE_HASH)
        ht_saveToFile(&ht);
    else
        ll_saveToFile();
}

void loadFromFile(void)
{
    if (currentMode == MODE_HASH)
        ht_loadFromFile(&ht);
    else
        ll_loadFromFile();
}

void filterAndExport(Filter f)
{
    if (currentMode == MODE_HASH)
        ht_filterAndExport(&ht, f);
    else
        ll_filterAndExport(f);
}

void sortRecords(SortRule rules[], int n)
{
    if (currentMode == MODE_HASH)
        ht_sortRecords(&ht, rules, n);
    else
        ll_sortRecords(rules, n);
}

void runStatistics(void)
{
    if (currentMode == MODE_HASH)
        ht_runStatistics(&ht);
    else
        ll_runStatistics();
}

int batchDeleteExpired(void)
{
    if (currentMode == MODE_HASH)
        return ht_batchDeleteExpired(&ht);
    else
        return ll_batchDeleteExpired();
}

void destroyCurrentStructure(void)
{
    if (currentMode == MODE_HASH)
        ht_destroyTable(&ht);
    else
        ll_clearList();
}

/* ================================================================
 *  数据结构切换
 * ================================================================ */

void switchDataMode(void)
{
    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║              切换数据结构                    ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║  当前：%-36s ║\n", modeName());
    printf("╚══════════════════════════════════════════════╝\n");

    /* 1. 保存当前数据 */
    printf("[1/3] 正在保存当前数据到 %s ...\n", DATA_FILE);
    saveToFile();

    /* 2. 清空当前结构 */
    printf("[2/3] 正在清空当前 %s ...\n", modeName());
    destroyCurrentStructure();

    /* 3. 切换模式并重新加载 */
    currentMode = (currentMode == MODE_HASH) ? MODE_LIST : MODE_HASH;
    printf("[3/3] 正在加载数据到 %s ...\n", modeName());
    loadFromFile();

    printf("\n✓ 已切换为：%s，共 %d 条记录。\n", modeName(), getTotalCount());
}

/* ================================================================
 *  性能对比测试 —— 哈希表 vs 双向链表
 * ================================================================ */

void benchmarkCompare(void)
{
    int i, s, totalRecords, numScales;
    int scales[] = {5000, 10000, 25000, 50000, 75000, 100000};
    int validScales[7];

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  性能对比测试 —— 哈希表 vs 双向链表                      ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    /* ---- 步骤1: 加载全部数据到数组 ---- */
    printf("[1/4] 正在从 %s 加载数据...\n", DATA_FILE);
    Record *allRecords = NULL;
    totalRecords = loadCSVtoArray(DATA_FILE, &allRecords);
    if (!allRecords || totalRecords == 0)
    {
        printf("[失败] 未加载到任何数据，请先运行 output.c 生成 %s\n", DATA_FILE);
        return;
    }
    printf("      加载完成，共 %d 条记录。\n", totalRecords);

    /* ---- 步骤2: 确定测试规模点 ---- */
    numScales = 0;
    for (i = 0; i < 6; i++)
    {
        if (scales[i] <= totalRecords)
            validScales[numScales++] = scales[i];
    }
    if (numScales > 0 && validScales[numScales - 1] < totalRecords)
        validScales[numScales++] = totalRecords;
    else if (numScales == 0)
        validScales[numScales++] = totalRecords;

    printf("\n[2/4] 测试规模点: ");
    for (i = 0; i < numScales; i++)
        printf("%d ", validScales[i]);
    printf("\n");

    /* ---- 步骤3: 逐规模执行两种结构的性能测试 ---- */
    printf("\n[3/4] 开始性能测试...\n");

    /* 哈希表结果 */
    double htInsertTime[10] = {0};
    double htSearchTime[10] = {0};
    double htDeleteTime[10] = {0};
    double htInsTP[10] = {0};
    double htSeaTP[10] = {0};
    double htDelTP[10] = {0};
    int htChainMax[10] = {0};
    double htChainAvg[10] = {0};
    int htEmpty[10] = {0};

    /* 链表结果 */
    double llInsertTime[10] = {0};
    double llSearchTime[10] = {0};
    double llDeleteTime[10] = {0};
    double llInsTP[10] = {0};
    double llSeaTP[10] = {0};
    double llDelTP[10] = {0};

    for (s = 0; s < numScales; s++)
    {
        int scale = validScales[s];
        int samples = (scale / 10 > 1000) ? (scale / 10) : 1000;
        printf("\n  ═══ 规模 %d 条记录 ═══\n", scale);

        /* ============ 哈希表测试 ============ */
        {
            HashTable testHT;
            ht_initTable(&testHT);

            /* 插入 */
            clock_t start = clock();
            for (i = 0; i < scale; i++)
            {
                Record r = allRecords[i];
                if (ht_findRecord(&testHT, r.studentId, r.courseId) != NULL)
                    continue;
                unsigned int idx = ht_hash(r.studentId, r.courseId);
                Node_HT *nd = (Node_HT *)malloc(sizeof(Node_HT));
                nd->data = r;
                nd->next = testHT.table[idx];
                testHT.table[idx] = nd;
            }
            clock_t end = clock();
            htInsertTime[s] = (double)(end - start) / CLOCKS_PER_SEC;
            htInsTP[s] = scale / htInsertTime[s];
            ht_statChainLength(&testHT, &htChainMax[s], &htChainAvg[s], &htEmpty[s]);
            printf("  [哈希表] 插入:%.4fs (%.0f条/s)  ", htInsertTime[s], htInsTP[s]);

            /* 查找 */
            {
                int *sampleIdx = (int *)malloc(samples * sizeof(int));
                int foundCount = 0;
                for (i = 0; i < samples; i++)
                    sampleIdx[i] = rand() % scale;
                clock_t start2 = clock();
                for (i = 0; i < samples; i++)
                {
                    Record *rec = &allRecords[sampleIdx[i]];
                    if (ht_findRecord(&testHT, rec->studentId, rec->courseId))
                        foundCount++;
                }
                clock_t end2 = clock();
                htSearchTime[s] = (double)(end2 - start2) / CLOCKS_PER_SEC;
                htSeaTP[s] = samples / htSearchTime[s];
                printf("查找:%.4fs (%.0f次/s)  ", htSearchTime[s], htSeaTP[s]);
                free(sampleIdx);
            }

            /* 删除 */
            {
                int delCount = samples < scale ? samples : scale / 2;
                int *delIdx = (int *)malloc(delCount * sizeof(int));
                for (i = 0; i < delCount; i++)
                    delIdx[i] = rand() % scale;
                clock_t start3 = clock();
                for (i = 0; i < delCount; i++)
                {
                    Record *rec = &allRecords[delIdx[i]];
                    unsigned int idx = ht_hash(rec->studentId, rec->courseId);
                    Node_HT *p = testHT.table[idx], *prev = NULL;
                    while (p)
                    {
                        if (strcmp(p->data.studentId, rec->studentId) == 0 &&
                            strcmp(p->data.courseId, rec->courseId) == 0)
                        {
                            if (!prev)
                                testHT.table[idx] = p->next;
                            else
                                prev->next = p->next;
                            free(p);
                            break;
                        }
                        prev = p;
                        p = p->next;
                    }
                }
                clock_t end3 = clock();
                htDeleteTime[s] = (double)(end3 - start3) / CLOCKS_PER_SEC;
                htDelTP[s] = delCount / htDeleteTime[s];
                printf("删除:%.4fs (%.0f条/s)", htDeleteTime[s], htDelTP[s]);
                free(delIdx);
            }
            printf("\n");

            ht_destroyTable(&testHT);
        }

        /* ============ 链表测试 ============ */
        {
            Node_LL *testHead = NULL, *testTail = NULL;

            /* 插入 */
            clock_t start = clock();
            for (i = 0; i < scale; i++)
            {
                Record r = allRecords[i];
                Node_LL *nd = (Node_LL *)malloc(sizeof(Node_LL));
                nd->data = r;
                nd->next = NULL;
                if (testHead == NULL)
                {
                    nd->prev = NULL;
                    testHead = testTail = nd;
                }
                else
                {
                    nd->prev = testTail;
                    testTail->next = nd;
                    testTail = nd;
                }
            }
            clock_t end = clock();
            llInsertTime[s] = (double)(end - start) / CLOCKS_PER_SEC;
            llInsTP[s] = scale / llInsertTime[s];
            printf("  [链表]   插入:%.4fs (%.0f条/s)  ", llInsertTime[s], llInsTP[s]);

            /* 查找 */
            {
                int *sampleIdx = (int *)malloc(samples * sizeof(int));
                int foundCount = 0;
                for (i = 0; i < samples; i++)
                    sampleIdx[i] = rand() % scale;
                clock_t start2 = clock();
                for (i = 0; i < samples; i++)
                {
                    Record *rec = &allRecords[sampleIdx[i]];
                    Node_LL *cur = testHead;
                    while (cur)
                    {
                        if (strcmp(cur->data.studentId, rec->studentId) == 0 &&
                            strcmp(cur->data.courseId, rec->courseId) == 0)
                        {
                            foundCount++;
                            break;
                        }
                        cur = cur->next;
                    }
                }
                clock_t end2 = clock();
                llSearchTime[s] = (double)(end2 - start2) / CLOCKS_PER_SEC;
                llSeaTP[s] = samples / llSearchTime[s];
                printf("查找:%.4fs (%.0f次/s)  ", llSearchTime[s], llSeaTP[s]);
                free(sampleIdx);
            }

            /* 删除 */
            {
                int delCount = samples < scale ? samples : scale / 2;
                int *delIdx = (int *)malloc(delCount * sizeof(int));
                for (i = 0; i < delCount; i++)
                    delIdx[i] = rand() % scale;
                clock_t start3 = clock();
                for (i = 0; i < delCount; i++)
                {
                    Record *rec = &allRecords[delIdx[i]];
                    Node_LL *p = testHead;
                    while (p)
                    {
                        if (strcmp(p->data.studentId, rec->studentId) == 0 &&
                            strcmp(p->data.courseId, rec->courseId) == 0)
                        {
                            if (p == testHead)
                                testHead = testHead->next;
                            if (p == testTail)
                                testTail = p->prev;
                            if (p->prev)
                                p->prev->next = p->next;
                            if (p->next)
                                p->next->prev = p->prev;
                            free(p);
                            break;
                        }
                        p = p->next;
                    }
                }
                clock_t end3 = clock();
                llDeleteTime[s] = (double)(end3 - start3) / CLOCKS_PER_SEC;
                llDelTP[s] = delCount / llDeleteTime[s];
                printf("删除:%.4fs (%.0f条/s)", llDeleteTime[s], llDelTP[s]);
                free(delIdx);
            }
            printf("\n");

            /* 清理测试链表 */
            {
                Node_LL *cur = testHead;
                while (cur)
                {
                    Node_LL *t = cur;
                    cur = cur->next;
                    free(t);
                }
            }
        }
    }

    /* ---- 步骤4: 输出对比报告 ---- */

    /* 4a. CSV */
    printf("\n[4/4] 写入性能对比报告至 D:\\perf_compare.csv ...\n");
    {
        FILE *fp = fopen("D:\\perf_compare.csv", "w");
        if (!fp)
        {
            printf("[错误] 无法创建 D:\\perf_compare.csv\n");
            free(allRecords);
            return;
        }
        fprintf(fp, "规模(条),"
                    "哈希-插入耗时(s),哈希-插入吞吐(条/s),"
                    "链表-插入耗时(s),链表-插入吞吐(条/s),"
                    "哈希-查找耗时(s),哈希-查找吞吐(条/s),"
                    "链表-查找耗时(s),链表-查找吞吐(条/s),"
                    "哈希-删除耗时(s),哈希-删除吞吐(条/s),"
                    "链表-删除耗时(s),链表-删除吞吐(条/s),"
                    "哈希-最大链长,哈希-平均链长,哈希-空桶数,哈希-装载因子\n");
        for (s = 0; s < numScales; s++)
        {
            double loadFactor = (double)validScales[s] / TABLE_SIZE;
            fprintf(fp, "%d,%.4f,%.0f,%.4f,%.0f,%.4f,%.0f,%.4f,%.0f,%.4f,%.0f,%.4f,%.0f,%d,%.2f,%d,%.2f\n",
                    validScales[s],
                    htInsertTime[s], htInsTP[s],
                    llInsertTime[s], llInsTP[s],
                    htSearchTime[s], htSeaTP[s],
                    llSearchTime[s], llSeaTP[s],
                    htDeleteTime[s], htDelTP[s],
                    llDeleteTime[s], llDelTP[s],
                    htChainMax[s], htChainAvg[s],
                    htEmpty[s], loadFactor);
        }
        fclose(fp);
    }

    /* 4b. 控制台对比表格 */
    printf("\n╔══════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              哈希表 vs 双向链表 — 性能对比汇总                                    ║\n");
    printf("╠════════╦══════════════╦══════════════╦══════════════╦══════════════╦══════════════╦══════════════╣\n");
    printf("║ 规模   ║ 哈希插入(s)  ║ 链表插入(s)  ║ 哈希查找(s)  ║ 链表查找(s)  ║ 哈希删除(s)  ║ 链表删除(s)  ║\n");
    printf("╠════════╬══════════════╬══════════════╬══════════════╬══════════════╬══════════════╬══════════════╣\n");
    for (s = 0; s < numScales; s++)
    {
        printf("║ %-6d ║ %-12.4f ║ %-12.4f ║ %-12.4f ║ %-12.4f ║ %-12.4f ║ %-12.4f ║\n",
               validScales[s],
               htInsertTime[s], llInsertTime[s],
               htSearchTime[s], llSearchTime[s],
               htDeleteTime[s], llDeleteTime[s]);
    }
    printf("╚════════╩══════════════╩══════════════╩══════════════╩══════════════╩══════════════╩══════════════╝\n");

    /* 4c. 吞吐量对比表格 */
    printf("\n╔══════════════════════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                              吞吐量对比 (条/秒)                                                   ║\n");
    printf("╠════════╦══════════════╦══════════════╦══════════════╦══════════════╦══════════════╦══════════════╣\n");
    printf("║ 规模   ║ 哈希插入     ║ 链表插入     ║ 哈希查找     ║ 链表查找     ║ 哈希删除     ║ 链表删除     ║\n");
    printf("╠════════╬══════════════╬══════════════╬══════════════╬══════════════╬══════════════╬══════════════╣\n");
    for (s = 0; s < numScales; s++)
    {
        printf("║ %-6d ║ %-12.0f ║ %-12.0f ║ %-12.0f ║ %-12.0f ║ %-12.0f ║ %-12.0f ║\n",
               validScales[s],
               htInsTP[s], llInsTP[s],
               htSeaTP[s], llSeaTP[s],
               htDelTP[s], llDelTP[s]);
    }
    printf("╚════════╩══════════════╩══════════════╩══════════════╩══════════════╩══════════════╩══════════════╝\n");

    /* 4d. 性能对比分析 */
    printf("\n╔══════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                           性能对比分析                                       ║\n");
    printf("╠════════╦═════════════════╦═════════════════╦═════════════════╗\n");
    printf("║ 规模   ║ 查找:链表/哈希  ║ 插入:链表/哈希  ║ 删除:链表/哈希  ║\n");
    printf("╠════════╬═════════════════╬═════════════════╬═════════════════╣\n");
    for (s = 0; s < numScales; s++)
    {
        printf("║ %-6d ║ %-15.1f ║ %-15.1f ║ %-15.1f ║\n",
               validScales[s],
               htSearchTime[s] > 0 ? llSearchTime[s] / htSearchTime[s] : 0,
               htInsertTime[s] > 0 ? llInsertTime[s] / htInsertTime[s] : 0,
               htDeleteTime[s] > 0 ? llDeleteTime[s] / htDeleteTime[s] : 0);
    }
    printf("╚════════╩═════════════════╩═════════════════╩═════════════════╝\n");
    printf("  (比值 > 1 表示链表更慢，即哈希表更快；比值越大，哈希表优势越明显)\n");

    /* 4e. 综合结论 */
    printf("\n--- 综合结论 ---\n");
    printf("  哈希表:\n");
    printf("    插入: O(1) 期望 ~ O(链长) 最坏\n");
    printf("    查找: O(1) 期望，大规模下优势显著\n");
    printf("    删除: O(1) 期望\n");
    printf("    空间: 预分配 %d 个桶 + 节点开销\n", TABLE_SIZE);
    printf("    适用: 大规模随机键精确查找\n\n");
    printf("  双向链表:\n");
    printf("    插入: O(1) 尾插\n");
    printf("    查找: O(n) 线性扫描，随数据量线性退化\n");
    printf("    删除: O(n)\n");
    printf("    空间: 仅节点开销，无需预分配\n");
    printf("    适用: 小数据量、需要频繁遍历/排序的场景\n");

    printf("\n 性能对比报告已保存至: D:\\perf_compare.csv\n");
    printf("   可在 Excel 中打开并绘制双系列折线图进行可视化分析。\n");

    free(allRecords);
}

/* ================================================================
 *  主菜单
 * ================================================================ */

int main(void)
{
    int choice;

    /* 初始化 */
    ht_initTable(&ht);
    head = tail = NULL;
    loadFromFile(); /* 默认以哈希表模式加载 */

    printf("\n当前使用数据结构：%s\n", modeName());

    do
    {
        printf("\n===== 选课管理系统（当前：%s）=====\n", modeName());
        printf("1.  插入记录\n");
        printf("2.  删除记录\n");
        printf("3.  修改成绩\n");
        printf("4.  按学号查找\n");
        printf("5.  按姓名精确查找\n");
        printf("6.  按姓名模糊查找\n");
        printf("7.  按课程名精确查找\n");
        printf("8.  按课程名模糊查找\n");
        printf("9.  显示全部记录\n");
        printf("10. 多条件筛选并导出\n");
        printf("11. 多关键字排序\n");
        printf("12. 数据统计分析\n");
        printf("13. 批量删除过期记录\n");
        printf("14. [性能对比] 哈希表 vs 双向链表\n");
        printf("15. 切换数据结构（当前：%s）\n", modeName());
        printf("0.  保存并退出\n");
        printf("请选择：");
        scanf("%d", &choice);
        getchar();

        switch (choice)
        {
        case 1:
        {
            Record r;
            printf("学号：");
            scanf("%s", r.studentId);
            printf("姓名：");
            scanf("%s", r.name);
            printf("学院：");
            scanf("%s", r.college);
            printf("课程编号：");
            scanf("%s", r.courseId);
            printf("课程名称：");
            scanf("%s", r.courseName);
            printf("学分：");
            scanf("%f", &r.credit);
            printf("学期(如2024-1)：");
            scanf("%s", r.semester);
            printf("选课日期(如2024-09-01)：");
            scanf("%s", r.selectDate);
            printf("成绩：");
            scanf("%d", &r.score);
            insertRecord(r);
            printf("插入操作已执行。\n");
            break;
        }
        case 2:
        {
            char sid[13], cid[9];
            printf("学号：");
            scanf("%s", sid);
            printf("课程编号：");
            scanf("%s", cid);
            printf(deleteRecord(sid, cid) ? "删除成功！\n" : "未找到记录！\n");
            break;
        }
        case 3:
        {
            char sid[13], cid[9];
            int sc;
            printf("学号：");
            scanf("%s", sid);
            printf("课程编号：");
            scanf("%s", cid);
            printf("新成绩：");
            scanf("%d", &sc);
            printf(updateRecord(sid, cid, sc) ? "修改成功！\n" : "未找到记录！\n");
            break;
        }
        case 4:
        {
            char sid[13];
            printf("学号：");
            scanf("%s", sid);
            searchByStudentId(sid);
            break;
        }
        case 5:
        {
            char nm[30];
            printf("姓名：");
            scanf("%s", nm);
            searchByNameExact(nm);
            break;
        }
        case 6:
        {
            char kw[30];
            printf("关键字：");
            scanf("%s", kw);
            searchByNameFuzzy(kw);
            break;
        }
        case 7:
        {
            char cn[50];
            printf("课程名称：");
            scanf("%s", cn);
            searchByCourseExact(cn);
            break;
        }
        case 8:
        {
            char kw[50];
            printf("关键字：");
            scanf("%s", kw);
            searchByCourseFuzzy(kw);
            break;
        }
        case 9:
        {
            printf("\n--- 全部选课记录 ---\n");
            printAll();
            printf("共 %d 条记录。\n", getTotalCount());
            break;
        }
        case 10:
        {
            Filter f;
            strcpy(f.courseName, "");
            strcpy(f.semester, "");
            strcpy(f.college, "");
            f.scoreMin = -1;
            f.scoreMax = -1;
            f.useFuzzyCourse = 0;
            printf("课程名(直接回车=不限制)：");
            safeScanStr(f.courseName, 50);
            if (strlen(f.courseName) > 0)
            {
                printf("课程名是否模糊匹配？(1=模糊 0=精确)：");
                scanf("%d", &f.useFuzzyCourse);
                getchar();
            }
            printf("学期(直接回车=不限制)：");
            safeScanStr(f.semester, 8);
            printf("学院(直接回车=不限制)：");
            safeScanStr(f.college, 50);
            printf("最低分(-1=不限制)：");
            scanf("%d", &f.scoreMin);
            printf("最高分(-1=不限制)：");
            scanf("%d", &f.scoreMax);
            getchar();
            filterAndExport(f);
            break;
        }
        case 11:
        {
            SortRule rules[3];
            int n, i;
            printf("排序规则数量(1~3)：");
            scanf("%d", &n);
            if (n < 1)
                n = 1;
            if (n > 3)
                n = 3;
            for (i = 0; i < n; i++)
            {
                printf("\n第 %d 个排序关键字：\n", i + 1);
                printf("  类型 (0=成绩 1=学号 2=课程名)：");
                scanf("%d", &rules[i].type);
                printf("  方向 (1=升序 0=降序)：");
                scanf("%d", &rules[i].order);
            }
            getchar();
            sortRecords(rules, n);
            break;
        }
        case 12:
            runStatistics();
            break;
        case 13:
            batchDeleteExpired();
            break;
        case 14:
            benchmarkCompare();
            break;
        case 15:
            switchDataMode();
            break;
        case 0:
            saveToFile();
            printf("数据已保存，系统退出。\n");
            break;
        default:
            printf("输入错误！请重新选择。\n");
        }
    } while (choice != 0);

    destroyCurrentStructure();
    return 0;
}
