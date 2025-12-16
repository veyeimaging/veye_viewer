// #include "UYVYRender.h"
// #include <QDebug>

// UYVYRenderer::UYVYRenderer(QWidget *parent)
//     : QOpenGLWidget(parent)
//     , m_program(nullptr)
//     , m_texture(nullptr)
//     , m_vbo(0)
//     , m_width(0)
//     , m_height(0)
//     , m_texSizeLoc(-1)
// {
//     // 强制配置 GLES 2.0 上下文（嵌入式设备核心配置）
//     QSurfaceFormat format;
//     format.setRenderableType(QSurfaceFormat::OpenGLES);   // 明确使用 OpenGL ES
//     format.setVersion(2, 0);                              // 指定 GLES 2.0 版本
//     format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); // 双缓冲避免画面闪烁
//     format.setSamples(0);                                 // 禁用抗锯齿（GLES 2.0 兼容性优先）
//     setFormat(format);

//     // 禁止自动填充背景（避免与 OpenGL 渲染冲突）
//     setAutoFillBackground(false);
// }

// UYVYRenderer::~UYVYRenderer()
// {
//     // 确保在 OpenGL 上下文环境中释放资源
//     makeCurrent();

//     // 释放着色器程序
//     if (m_program) {
//         m_program->release();
//         delete m_program;
//         m_program = nullptr;
//     }

//     // 释放纹理对象
//     if (m_texture) {
//         m_texture->release();
//         delete m_texture;
//         m_texture = nullptr;
//     }

//     // 释放 VBO（避免显存泄漏）
//     if (m_vbo != 0) {
//         glDeleteBuffers(1, &m_vbo);
//         m_vbo = 0;
//     }

//     doneCurrent();
// }

// void UYVYRenderer::setUYVYData(const unsigned char *data, int width, int height)
// {
//     // 输入合法性校验（UYVY 宽度必须为偶数，因每 2 像素共用 1 组 U/V）
//     if (!data || width <= 0 || height <= 0 || width % 2 != 0) {
//         qWarning() << "[UYVYRenderer] Invalid data: width=" << width << ", height=" << height
//                    << " (width must be even)";
//         m_uyvyBuffer.clear(); // 清空无效数据
//         return;
//     }

//     // 复制数据到内部缓冲区（避免外部数据释放后出现野指针）
//     const int dataSize = width * height * 2; // UYVY 格式：2 字节/像素
//     m_uyvyBuffer.resize(dataSize);
//     memcpy(m_uyvyBuffer.data(), data, dataSize);

//     // 更新分辨率
//     m_width = width;
//     m_height = height;

//     // 若纹理已创建，更新纹理数据（避免重复创建纹理消耗资源）
//     if (m_texture) {
//         makeCurrent();
//         m_texture->bind();
//         // 纹理宽度 = 原宽度 / 2（UYVY 4 分量合并为 RGBA 1 像素，对应原 2 像素）
//         m_texture->setSize(m_width / 2, m_height);
//         // 传递 UYVY 数据（格式：RGBA，数据类型：无符号字节）
//         m_texture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, m_uyvyBuffer.data());
//         m_texture->release();
//         doneCurrent();
//     }

//     // 触发重绘
//     update();
// }

// void UYVYRenderer::initializeGL()
// {
//     // 初始化 GLES 2.0 函数（必须调用，否则无法使用 gl* 函数）
//     if (!initializeOpenGLFunctions()) {
//         qCritical() << "[UYVYRenderer] Failed to initialize GLES 2.0 functions";
//         return;
//     }

//     // 设置清空画布颜色（黑色）
//     glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

//     // 初始化着色器（失败则终止初始化）
//     if (!initShaders()) {
//         qCritical() << "[UYVYRenderer] Shader initialization failed";
//         return;
//     }

//     // 创建 UYVY 纹理（GLES 2.0 兼容配置）
//     m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
//     m_texture->create();
//     m_texture->setFormat(QOpenGLTexture::RGBA8_UNorm);         // 8 位 RGBA 格式（匹配 UYVY 4 分量）
//     m_texture->setMinificationFilter(QOpenGLTexture::Linear);  // 缩小过滤：线性（平滑）
//     m_texture->setMagnificationFilter(QOpenGLTexture::Linear); // 放大过滤：线性（平滑）
//     m_texture->setWrapMode(QOpenGLTexture::ClampToEdge); // 纹理边缘处理： clamping（避免拉伸异常）
//     m_texture->release();                                // 释放纹理绑定

//     // 创建 VBO（存储顶点坐标 + 纹理坐标）
//     // 顶点数据：4 个顶点（覆盖整个窗口），每个顶点包含（x,y, s,t）4 个 float
//     float vertices[] = {
//         // 位置          纹理坐标
//         -1.0f,
//         1.0f,
//         0.0f,
//         1.0f, // 左上角
//         -1.0f,
//         -1.0f,
//         0.0f,
//         0.0f, // 左下角
//         1.0f,
//         -1.0f,
//         1.0f,
//         0.0f, // 右下角
//         1.0f,
//         1.0f,
//         1.0f,
//         1.0f // 右上角
//     };

//     // 生成 VBO 并绑定数据
//     glGenBuffers(1, &m_vbo);
//     glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
//     // 传递顶点数据（静态数据：GL_STATIC_DRAW）
//     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//     glBindBuffer(GL_ARRAY_BUFFER, 0); // 解绑 VBO（避免后续操作污染）

//     qInfo() << "[UYVYRenderer] GLES 2.0 initialization completed successfully";
// }

// void UYVYRenderer::resizeGL(int w, int h)
// {
//     // 设置视口（匹配窗口大小，确保渲染内容充满窗口）
//     glViewport(0, 0, w, h);
// }

// void UYVYRenderer::paintGL()
// {
//     // 合法性校验（资源未就绪则跳过渲染）
//     if (!m_program || !m_texture || m_uyvyBuffer.isEmpty()) {
//         glClear(GL_COLOR_BUFFER_BIT); // 清空画布为黑色
//         return;
//     }

//     // 1. 清空画布
//     glClear(GL_COLOR_BUFFER_BIT);

//     // 2. 绑定着色器程序和纹理
//     m_program->bind();
//     m_texture->bind(0); // 绑定到纹理单元 0

//     // 3. 设置着色器 uniform 变量
//     m_program->setUniformValue("uyvyTexture", 0);                    // 告诉着色器：纹理在单元 0
//     m_program->setUniformValue(m_texSizeLoc, m_width / 2, m_height); // 传递纹理尺寸

//     // 4. 绑定 VBO 并配置顶点属性（GLES 2.0 无 VAO，需手动配置）
//     glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

//     // 顶点位置属性（location = 0）：2 个 float，无归一化，步长 4*float，偏移 0
//     glEnableVertexAttribArray(0);
//     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);

//     // 纹理坐标属性（location = 1）：2 个 float，无归一化，步长 4*float，偏移 2*float
//     glEnableVertexAttribArray(1);
//     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));

//     // 5. 绘制四边形（GL_TRIANGLE_FAN：4 个顶点绘制 2 个三角形，覆盖整个窗口）
//     glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

//     // 6. 清理资源（避免属性污染后续渲染）
//     glDisableVertexAttribArray(0);
//     glDisableVertexAttribArray(1);
//     glBindBuffer(GL_ARRAY_BUFFER, 0);
//     m_texture->release();
//     m_program->release();
// }

// bool UYVYRenderer::initShaders()
// {
//     // 创建着色器程序
//     m_program = new QOpenGLShaderProgram(this);

//     // -------------------------- 顶点着色器（GLES 2.0 兼容）--------------------------
//     const char *vertexShaderSrc = R"(
//         #version 100          // GLES 2.0 必须使用 #version 100
//         attribute vec2 aPos;  // 顶点位置输入（attribute 关键字，对应 location 0）
//         attribute vec2 aTexCoord; // 纹理坐标输入（对应 location 1）

//         varying vec2 TexCoord; // 传递纹理坐标到片段着色器（varying 关键字）

//         void main()
//         {
//             gl_Position = vec4(aPos, 0.0, 1.0); // 顶点最终位置（齐次坐标）
//             TexCoord = aTexCoord;               // 传递纹理坐标
//         }
//     )";

//     // 添加并编译顶点着色器
//     if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSrc)) {
//         qCritical() << "[UYVYRenderer] Vertex shader error:\n" << m_program->log();
//         return false;
//     }

//     // -------------------------- 片段着色器（GLES 2.0 兼容）--------------------------
//     const char *fragmentShaderSrc = R"(
//         #version 100                  // GLES 2.0 必须使用 #version 100
//         precision mediump float;      // GLES 2.0 必须指定浮点数精度（mediump：中等精度）

//         varying vec2 TexCoord;        // 从顶点着色器接收的纹理坐标
//         uniform sampler2D uyvyTexture;// 纹理采样器（对应纹理单元 0）
//         uniform vec2 texSize;         // 纹理尺寸（width: 原宽度/2, height: 原高度）

//         void main()
//         {
//             // 1. 采样 UYVY 纹理（texture2D：GLES 2.0 纹理采样函数）
//             vec4 uyvy = texture2D(uyvyTexture, TexCoord);

//             // 2. 提取 UYVY 分量（R=U, G=Y0, B=V, A=Y1）
//             float U = uyvy.r - 0.5;   // U 分量归一化（-0.5 ~ 0.5）
//             float Y0 = uyvy.g;        // 第一个像素的 Y 分量（亮度）
//             float V = uyvy.b - 0.5;   // V 分量归一化（-0.5 ~ 0.5）
//             float Y1 = uyvy.a;        // 第二个像素的 Y 分量（亮度）

//             // 3. 判断当前像素对应原 UYVY 的 Y0 还是 Y1（基于纹理坐标的 x 分量）
//             float texCol = TexCoord.x * texSize.x; // 纹理坐标对应的像素列号
//             float Y = fract(texCol) < 0.5 ? Y0 : Y1; // 左半像素取 Y0，右半取 Y1

//             // 4. YUV 转 RGB（BT.601 标准，适用于大多数视频场景）
//             float R = Y + 1.402 * V;  // 红色通道
//             float G = Y - 0.344136 * U - 0.714136 * V; // 绿色通道
//             float B = Y + 1.772 * U;  // 蓝色通道

//             // 5. 输出最终颜色（clamp 确保颜色值在 0~1 范围内，避免溢出）
//             gl_FragColor = vec4(clamp(R, 0.0, 1.0),
//                                 clamp(G, 0.0, 1.0),
//                                 clamp(B, 0.0, 1.0),
//                                 1.0);
//         }
//     )";

//     // 添加并编译片段着色器
//     if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSrc)) {
//         qCritical() << "[UYVYRenderer] Fragment shader error:\n" << m_program->log();
//         return false;
//     }

//     // -------------------------- 链接着色器程序 --------------------------
//     if (!m_program->link()) {
//         qCritical() << "[UYVYRenderer] Shader link error:\n" << m_program->log();
//         return false;
//     }

//     // 保存 "texSize" uniform 的位置（避免每次渲染时重复查询，提升性能）
//     m_texSizeLoc = m_program->uniformLocation("texSize");
//     if (m_texSizeLoc == -1) {
//         qWarning() << "[UYVYRenderer] Uniform 'texSize' not found in shader";
//         return false;
//     }

//     qInfo() << "[UYVYRenderer] Shaders initialized successfully";
//     return true;
// }
