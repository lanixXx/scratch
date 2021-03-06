#ifndef DEMOCUBE_H
#define DEMOCUBE_H

#include <QImage>
#include "qdecviewportitem.h"

class DemoTexture : public QDecViewportItem
{
    Q_OBJECT
public:
    explicit DemoTexture(QDeclarativeItem *parent = 0);

private:
    void initViewport();
    void drawViewport();
    GLuint loadShader(GLenum type, const char *shaderSrc);

    // opengl stuff
    GLuint m_gl_hdl_prog;               // handle to program
    GLuint m_gl_hdl_vbo_pos;            // handle to vertex buffer for position
    GLuint m_gl_hdl_vbo_tex;
    GLuint m_gl_hdl_ibo;
    GLuint m_gl_hdl_tex;

    GLint m_gl_idx_attrib0;             // index for generic vertex attribute 0
    GLint m_gl_idx_attrib1;             // index for generic vertex attribute 1
    GLint m_gl_loc_xf_mvp;              // location of uniform xf_mvp within program
    GLint m_gl_loc_texsampler;          //

    GLfloat * m_gl_vpos;                // vertex position data
    GLfloat * m_gl_vcolor;              // vertex color data
};

#endif
