// 引入 glad：负责加载 OpenGL 函数指针（否则很多 glXXX 用不了）
#include <glad/glad.h>

// 引入 GLFW：负责创建窗口、处理输入
#include <GLFW/glfw3.h>

// C++标准输出（调试用）
#include <iostream>

// 函数声明：告诉编译器这个函数后面会定义
void processInput(GLFWwindow *window);

int main()
{
    // ================== 1. 初始化 GLFW ==================
    glfwInit();  
    // 初始化 GLFW 库（必须第一步，否则后面函数不能用）

    // ================== 2. 设置 OpenGL版本 ==================
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); 
    // 使用 OpenGL 主版本 3

    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); 
    // 使用 OpenGL 次版本 3 → 合起来就是 3.3

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // 使用核心模式（Core Profile），现代 OpenGL

    // ================== 3. 创建窗口 ==================
    GLFWwindow* window = glfwCreateWindow(800, 600, "Hello OpenGL", NULL, NULL);
    // 创建一个 800x600 的窗口，标题叫 Hello OpenGL

    glfwMakeContextCurrent(window);
    // 把这个窗口设置为当前 OpenGL 上下文（后面所有操作都作用于它）

    // ================== 4. 加载 OpenGL函数 ==================
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    // 让 glad 获取所有 OpenGL 函数地址（否则 gl函数是空的）

    // ================== 5. 定义顶点数据 ==================
    float vertices[] = {
        0.0f,  0.5f, 0.0f,   // 顶点1（上）
       -0.5f, -0.5f, 0.0f,   // 顶点2（左下）
        0.5f, -0.5f, 0.0f    // 顶点3（右下）
    };
    // 三个点 → 一个三角形

    // ================== 6. 创建 VBO / VAO ==================
    unsigned int VBO, VAO;

    glGenVertexArrays(1, &VAO);
    // 生成一个 VAO（记录“如何解释顶点数据”）

    glGenBuffers(1, &VBO);
    // 生成一个 VBO（存储顶点数据）

    glBindVertexArray(VAO);
    // 绑定 VAO（后面所有配置都记录到这个 VAO）

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // 把 VBO 绑定到“顶点缓冲区”

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // 把 vertices 数据复制到 GPU

    // 告诉 OpenGL：如何解析这些数据
    glVertexAttribPointer(
        0,                  // 属性位置 = 0（对应 shader里的 layout location）
        3,                  // 每个顶点有3个float（x,y,z）
        GL_FLOAT,           // 数据类型是 float
        GL_FALSE,           // 不需要归一化
        3 * sizeof(float),  // 步长（每个顶点占多少字节）
        (void*)0            // 偏移量（从哪里开始读）
    );

    glEnableVertexAttribArray(0);
    // 启用第0号顶点属性（否则不会生效）

    // ================== 7. 写 Shader ==================

    // 顶点着色器源码
    const char* vertexShaderSource = R"(
    #version 330 core

    layout (location = 0) in vec3 aPos;
    // 输入：从 VAO 里拿位置数据

    void main()
    {
        gl_Position = vec4(aPos, 1.0);
        // 输出：告诉GPU这个点在屏幕的位置
    })";

    // 片段着色器源码
    const char* fragmentShaderSource = R"(
    #version 330 core

    out vec4 FragColor;
    // 输出颜色

    void main()
    {
        FragColor = vec4(1.0, 0.5, 0.2, 1.0);
        // 固定颜色（橙色）
    })";

    // ================== 编译 Shader ==================

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    // 创建“顶点着色器对象”

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    // 把源码绑定到这个 shader

    glCompileShader(vertexShader);
    // 编译顶点着色器

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    // 创建片段着色器

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    // 绑定源码

    glCompileShader(fragmentShader);
    // 编译

    // ================== 创建 Shader 程序 ==================
    unsigned int shaderProgram = glCreateProgram();
    // 创建程序（把多个 shader 组合）

    glAttachShader(shaderProgram, vertexShader);
    // 附加顶点着色器

    glAttachShader(shaderProgram, fragmentShader);
    // 附加片段着色器

    glLinkProgram(shaderProgram);
    // 链接 → 让两个 shader 能一起工作

    glDeleteShader(vertexShader);
    // 删除单独的 shader（已经链接进程序了）

    glDeleteShader(fragmentShader);

    // ================== 8. 渲染循环 ==================
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        // 处理键盘输入（比如 ESC 退出）

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        // 设置背景颜色

        glClear(GL_COLOR_BUFFER_BIT);
        // 清空屏幕

        glUseProgram(shaderProgram);
        // 使用我们刚刚创建的 shader

        glBindVertexArray(VAO);
        // 绑定 VAO（告诉GPU用哪组顶点数据）

        glDrawArrays(GL_TRIANGLES, 0, 3);
        // 画图：用3个点画一个三角形

        glfwSwapBuffers(window);
        // 双缓冲：把画好的图显示出来

        glfwPollEvents();
        // 处理窗口事件（键盘、鼠标等）
    }

    glfwTerminate();
    // 释放 GLFW 资源

    return 0;
}

// ================== 输入函数 ==================
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        // 如果按下 ESC 键

        glfwSetWindowShouldClose(window, true);
        // 关闭窗口
}
