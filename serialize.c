// 序列化和反序列化

include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct TreeNode {
	    int val;
	        struct TreeNode *left;
		    struct TreeNode *right;
};

#define MAX_LEN 100000
// 将root中的元素转化为字符存储到一个字符数组中
void serializeDFS(struct TreeNode *root, char *buf, int *pos) {
	    if (root == NULL) {
		            int len = sprintf(buf + *pos, "null ");
			            *pos += len;
				            return;
					        }
	        int len = sprintf(buf + *pos, "%d ", root->val);
		    *pos += len;
		        serializeDFS(root->left, buf, pos);
			    serializeDFS(root->right, buf, pos);
}

// 输出那个字符数组
char *serialize(struct TreeNode *root) {
	    char *buf = (char *)malloc(sizeof(char) * MAX_LEN);
	        if (buf == NULL) {
			        return NULL;
				    }
		    int pos = 0;
		        serializeDFS(root, buf, &pos);
			    buf[pos] = '\0';
			        return buf;
}


// 创建数值为 val 的树节点
struct TreeNode *create(int val) {
	    struct TreeNode *root = malloc(sizeof(struct TreeNode));
	        if (root == NULL) {
			        return NULL;
				    }
		    root->val = val;
		        root->left = NULL;
			    root->right = NULL;
			        return root;
}


// 将字符串数组中的字符串转成树的节点
struct TreeNode *deserializeDFS(char **tokens, int *index) {
	    char *token = tokens[*index];
	        (*index)++;
		    if (strcmp(token, "null") == 0) {
			            return NULL;
				        }
		        struct TreeNode *node = create(atoi(token));
			    node->left = deserializeDFS(tokens, index);
			        node->right = deserializeDFS(tokens, index);
				    return node;
}
// 先将字符串利用 strtok 转化为字符串数组，再利用上面的函数转化为树节点，构造树，输出树
struct TreeNode *deserialize(char *data) {
	    if (data == NULL || strlen(data) == 0) {
		            return NULL;
			        }
	        char **tokens = (char **)malloc(sizeof(char *) * 100000);
		    int count = 0;
		        char *token = strtok(data, " ");
			    while (token != NULL) {
				            tokens[count++] = token;
					            token = strtok(NULL, " ");
						        }
			        int index = 0;
				    struct TreeNode *root = deserializeDFS(tokens, &index);
				        free(tokens);
					    return root;
}
// 释放二叉树的空间
void destroy(struct TreeNode *root) {
	    if (root == NULL) {
		            return;
			        }
	        destroy(root->left);
		    destroy(root->right);
		        free(root);
}
