// #ifndef UYVYRENDER_H
// #define UYVYRENDER_H

// #include <QByteArray>
// #include <QOpenGLFunctions_ES2>
// #include <QOpenGLShaderProgram>
// #include <QOpenGLTexture>
// #include <QOpenGLWidget>

// // 继承 QOpenGLFunctions_ES2 以确保 GLES 2.0 函数可用
// class UYVYRenderer : public QOpenGLWidget, protected QOpenGLFunctions_ES2
// {
//     Q_OBJECT

// public:
//     explicit UYVYRenderer(QWidget *parent = nullptr);
//     ~UYVYRenderer() override;

//     // 外部调用：设置 UYVY 数据（width 必须为偶数，UYVY 格式为 2 字节/像素）
//     void setUYVYData(const unsigned char *data, int width, int height);

// protected:
//     // OpenGL 核心生命周期函数
//     void initializeGL() override;
//     void resizeGL(int w, int h) override;
//     void paintGL() override;

// private:
//     // 辅助函数：初始化着色器
//     bool initShaders();

// private:
//     QOpenGLShaderProgram *m_program; // 着色器程序
//     QOpenGLTexture *m_texture;       // UYVY 纹理对象
//     GLuint m_vbo;                    // 顶点缓冲对象（VBO）
//     QByteArray m_uyvyBuffer;         // 存储 UYVY 数据（避免野指针风险）
//     int m_width;                     // 视频宽度（原分辨率）
//     int m_height;                    // 视频高度（原分辨率）
//     int m_texSizeLoc;                // 着色器中 "texSize" uniform 的位置
// };
// #endif // UYVYRENDER_H
