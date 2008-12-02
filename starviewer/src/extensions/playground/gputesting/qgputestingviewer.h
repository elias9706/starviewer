#ifndef UDGQGPUTESTINGVIEWER_H
#define UDGQGPUTESTINGVIEWER_H


#include <QGLWidget>


namespace udg {


class Volume;


/**
 * Visualitzador amb GPU. Fa tota la feina necessària per visualitzar un volum donat amb la GPU.
 */
class QGpuTestingViewer : public QGLWidget {

    Q_OBJECT

public:

    QGpuTestingViewer( QWidget *parent = 0 );
    ~QGpuTestingViewer();

    /// Assigna el volum a visualitzar. Un cop assignat un volum no es pot canviar.
    void setVolume( Volume *volume );

protected:

    /// Fa les inicialitzacions necessàries abans de començar a visualitzar amb OpenGL.
    virtual void initializeGL();
    /// Ajusta el viewport i la projecció quan canvia la mida del visualitzador.
    virtual void resizeGL( int width, int height );
    /// Fa la visualització amb OpenGL.
    virtual void paintGL();

private:

    void createVolumeTexture();
    void createFramebufferObject();
    void loadShaders();
    void drawCube();



    void enableRenderbuffers();
    void disableRenderbuffers();
    void renderBackface();
    // this method is used to draw the front and backside of the volume
    void drawQuads( float x, float y, float z );
    void vertex( float x, float y, float z );
    void raycastingPass();
    void renderBufferToScreen();





    Volume *m_volume;

    GLuint m_volumeTexture;

    GLhandleARB m_shaderProgram;
    GLint m_texUniform;
    GLint m_volumeTexUniform;
    GLint m_stepSizeUniform;

    GLuint m_framebufferObject;
    GLuint m_backfaceBuffer;

    GLuint m_framebuffer;
    GLuint m_finalImage;
    GLuint m_renderbuffer;

};


}


#endif
