#include "qglview.h"
#include "effect.h"
#include "messagepanel.h"

#include <QtCore/QUrl>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

#if HAVE_NVIDIA_CG
#include <Cg/cg.h>
#include <Cg/cgGL.h>
#endif

extern void drawTeapot();


QGLView::QGLView(const QGLFormat & format, QWidget *parent) : QGLWidget(format, parent),
	m_effect(NULL), m_dlist(0)
{
    setAcceptDrops(true);
}

void QGLView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list"))
        event->acceptProposedAction();
}

void QGLView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    event->acceptProposedAction();
    if( urls.size() )
        emit fileDropped( urls[0].toLocalFile() );
}

QSize QGLView::sizeHint() const
{
	return QSize(200, 200);
}
QSize QGLView::minimumSizeHint() const
{
	return QSize(100, 100);
}


void QGLView::setEffect(Effect * effect)
{
	m_effect = effect;
	if( m_effect != NULL ) {
		makeCurrent();
	}
}

void QGLView::resetEffect()
{
	m_effect = NULL;
}

bool QGLView::init(MessagePanel * output)
{
	Q_ASSERT(output != NULL);
	
	makeCurrent();
	setAutoBufferSwap(false);
	
	// @@ Move to initializeGL
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		//fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		return false;
	}
	
	const char * vendor = (const char *)glGetString(GL_VENDOR);
	output->info(QString("OpenGL vendor: ").append(vendor));
	
	const char * renderer = (const char *)glGetString(GL_RENDERER);
	output->info(QString("OpenGL renderer: ").append(renderer));
		
	const char * version = (const char *)glGetString(GL_VERSION);
	output->info(QString("OpenGL version: ").append(version));
	
	output->info(QString("OpenGL extensions:"));
	
	if( GLEW_ARB_vertex_shader ) output->info("- ARB_vertex_shader FOUND");
	else output->error("- ARB_vertex_shader NOT FOUND");
	
	if( GLEW_ARB_fragment_shader ) output->info("- ARB_fragment_shader FOUND");
	else output->error("- ARB_fragment_shader NOT FOUND");
	
	if( GLEW_ARB_shader_objects ) output->info("- ARB_shader_objects FOUND");
	else output->error("- ARB_shader_objects NOT FOUND");
	
	if( GLEW_ARB_shading_language_100 ) output->info("- ARB_shading_language_100 FOUND");
	else output->error("- ARB_shading_language_100 NOT FOUND");
	
	if( GLEW_VERSION_2_0 ) {
		const char * glsl_version = (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
		output->info(QString("GLSL version: ").append(glsl_version));
	}
	
	return true;
}

void QGLView::initializeGL()
{
	glClearColor (0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	
	m_alpha = 0.0f;
	m_beta = 0.0f;
	m_x = 0.0f;
	m_y = 0.0f;
	m_z = 5.0f;	
	
	m_dlist = glGenLists(1);
	glNewList(m_dlist, GL_COMPILE);
	drawTeapot();
	glEndList();
}

void QGLView::resetGL()
{
	if( m_dlist != 0 ) {
		glDeleteLists(m_dlist, 1);
	}
}


void QGLView::resizeGL(int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	updateMatrices();
}

void QGLView::paintGL()
{
	if( !isVisible() ) {
		// @@ updateGL should not be called when the window is hidden!
		return;
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	
	if( m_effect != NULL && m_effect->isValid() ) {
		
		m_effect->begin();
		
		for(int i = 0; i < m_effect->getPassNum(); i++) {
			m_effect->beginPass(i);
			
			glCallList(m_dlist);
			
			m_effect->endPass();
		}
		
		m_effect->end();
		
	}
	else {
	//	glCallList(m_dlist);
	}
	
	swapBuffers();	
}


void QGLView::updateMatrices()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(30, float(width())/float(height()), 0.5, 50);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// World transform:
	gluLookAt(m_x, m_y, m_z, m_x, m_y, m_z-1, 0, 1, 0);
	glRotatef(m_beta, 1, 0, 0);
	glRotatef(m_alpha, 0, 1, 0);
	
	// Object transform:
	glRotated(270.0, 1.0, 0.0, 0.0);
	glScaled(0.5, 0.5, 0.5);
	glTranslated(0.0, 0.0, -1.5);	
}


void QGLView::mousePressEvent(QMouseEvent *event)
{
	m_pos = event->pos();
	m_button = event->button();
}

void QGLView::mouseMoveEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();

	if(m_button == Qt::LeftButton) {
		m_alpha += (240.0f * (pos - m_pos).x()) / height();
		m_beta += (240.0f * (pos - m_pos).y()) / height();
	}
	else if(m_button == Qt::RightButton) {
		m_x -= (4.0f * (pos - m_pos).x()) / height();
		m_y += (4.0f * (pos - m_pos).y()) / height();
	}
	else if(m_button == Qt::MidButton) {
		m_z -= (5.0f * (pos - m_pos).y()) / height();
	}

	m_pos = pos;

	updateMatrices();
	updateGL();
}

void QGLView::mouseReleaseEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	m_button = Qt::NoButton;
}

void QGLView::wheelEvent(QWheelEvent *e)
{
	m_z += (e->delta()/120.0)/10.0;
	updateMatrices();
	updateGL();
}
