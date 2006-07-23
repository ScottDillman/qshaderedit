
#include "qshaderedit.h"
#include "qglview.h"
#include "effect.h"
#include "messagepanel.h"
#include "parameterpanel.h"
#include "editor.h"
#include "newdialog.h"

#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
//#include <QtCore/QtDebug>
#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QMenuBar>
#include <QtGui/QTextEdit>
#include <QtGui/QTabWidget>
#include <QtGui/QStatusBar>
#include <QtGui/QDockWidget>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QCloseEvent>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>


/// Ctor.
QShaderEdit::QShaderEdit() :
	m_editor(NULL),
	m_fileToolBar(NULL),
	m_techniqueToolBar(NULL),
	m_sceneViewDock(NULL),
	m_paramViewDock(NULL),
	m_logViewDock(NULL),
	m_positionLabel(NULL),
	m_sceneView(NULL),
	m_openAction(NULL),
	m_saveAction(NULL),
	m_techniqueCombo(NULL),
	m_timer(NULL),
	m_animationTimer(NULL),
	m_file(NULL),
	m_effectFactory(NULL),
	m_effect(NULL),
	m_modified(false),
	m_autoCompile(true)	// @@ Save settings!!
{
	// Create central widget.
	m_editor = new Editor(this);
	setCentralWidget(m_editor);
	m_editor->setFocus();
	connect(m_editor, SIGNAL(textChanged()), this, SLOT(shaderTextChanged()));
	connect(m_editor, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
	
	createActions();
	createToolbars();
	createDockWindows();
	createMenus();
	createStatusbar();

    loadSettings();

	// Start timers.
	m_timer = new QTimer(this);
	connect(m_timer, SIGNAL(timeout()), this, SLOT(keyTimeout()));
	
	m_animationTimer = new QTimer(this);
	connect(m_animationTimer, SIGNAL(timeout()), m_sceneView, SLOT(updateGL()));
	
	show();
	
	// Make sure the main window is shown before the new file dialog.
	QApplication::processEvents();
	
	newFile();
}

QShaderEdit::~QShaderEdit()
{
	closeEffect();
}

QSize QShaderEdit::sizeHint() const
{
	return QSize(600, 400);
}

void QShaderEdit::createActions()
{
	m_newAction = new QAction(tr("&New"), this);
	m_newAction->setShortcut(tr("Ctrl+N"));
	m_newAction->setStatusTip(tr("Create a new effect"));
	connect(m_newAction, SIGNAL(triggered()), this, SLOT(newFile()));
	
	m_openAction = new QAction(tr("&Open"), this);
	m_openAction->setShortcut(tr("Ctrl+O"));
	m_openAction->setStatusTip(tr("Open an effect"));
	connect(m_openAction, SIGNAL(triggered()), this, SLOT(open()));

	m_saveAction = new QAction(tr("&Save"), this);
	m_saveAction->setEnabled(false);
	m_saveAction->setShortcut(tr("Ctrl+S"));
	m_saveAction->setStatusTip(tr("Save this effect"));
	connect(m_saveAction, SIGNAL(triggered()), this, SLOT(save()));	
	
	m_saveAsAction = new QAction(tr("Save &As..."), this);
	m_saveAsAction->setEnabled(false);
	connect(m_saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs()));
}

void QShaderEdit::createMenus()
{
	QAction * action = NULL;
	
	QMenu * fileMenu = menuBar()->addMenu(tr("&File"));
	
	fileMenu->addAction(m_newAction);
	fileMenu->addAction(m_openAction);
	fileMenu->addAction(m_saveAction);
	fileMenu->addAction(m_saveAsAction);
	
	action = new QAction(tr("E&xit"), this);
	action->setShortcut(tr("Ctrl+Q"));
	action->setStatusTip(tr("Exit the application"));
	connect(action, SIGNAL(triggered()), this, SLOT(close()));
	fileMenu->addAction(action);
	
	QMenu * editMenu = menuBar()->addMenu(tr("&Edit"));

	action = new QAction(tr("&Undo"), this);
	action->setShortcut(tr("Ctrl+Z"));
	action->setEnabled(false);
	connect(action, SIGNAL(triggered()), m_editor, SLOT(undo()));
	connect(m_editor, SIGNAL(undoAvailable(bool)), action, SLOT(setEnabled(bool)));
	editMenu->addAction(action);

	action = new QAction(tr("&Redo"), this);
	action->setShortcut(tr("Ctrl+Shift+Z"));
	action->setEnabled(false);
	connect(action, SIGNAL(triggered()), m_editor, SLOT(redo()));
	connect(m_editor, SIGNAL(redoAvailable(bool)), action, SLOT(setEnabled(bool)));
	editMenu->addAction(action);

	editMenu->addSeparator();

	action = new QAction(tr("C&ut"), this);
	action->setShortcut(tr("Ctrl+X"));
	action->setEnabled(false);
	connect(action, SIGNAL(triggered()), m_editor, SLOT(cut()));
	connect(m_editor, SIGNAL(copyAvailable(bool)), action, SLOT(setEnabled(bool)));
	editMenu->addAction(action);

	action = new QAction(tr("&Copy"), this);
	action->setShortcut(tr("Ctrl+C"));
	action->setEnabled(false);
	connect(action, SIGNAL(triggered()), m_editor, SLOT(copy()));
	connect(m_editor, SIGNAL(copyAvailable(bool)), action, SLOT(setEnabled(bool)));
	editMenu->addAction(action);

	action = new QAction(tr("&Paste"), this);
	action->setShortcut(tr("Ctrl+V"));
	action->setEnabled(false);
	connect(action, SIGNAL(triggered()), m_editor, SLOT(paste()));
	connect(m_editor, SIGNAL(pasteAvailable(bool)), action, SLOT(setEnabled(bool)));
	editMenu->addAction(action);
	
	QMenu * viewMenu = menuBar()->addMenu(tr("&View"));
	viewMenu->addAction(m_sceneViewDock->toggleViewAction());
	viewMenu->addAction(m_paramViewDock->toggleViewAction());
	viewMenu->addAction(m_logViewDock->toggleViewAction());
	viewMenu->addSeparator();
	viewMenu->addAction(m_fileToolBar->toggleViewAction());
	viewMenu->addAction(m_techniqueToolBar->toggleViewAction());
	QMenu * helpMenu = menuBar()->addMenu(tr("&Help"));
	
	action = new QAction(tr("&About"), this);
	action->setStatusTip(tr("Show the application's about box"));
	connect(action, SIGNAL(triggered()), this, SLOT(about()));
	helpMenu->addAction(action);

	action = new QAction(tr("About &Qt"), this);
	action->setStatusTip(tr("Show the Qt library's about box"));
	connect(action, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	helpMenu->addAction(action);
}

void QShaderEdit::createToolbars()
{
	m_fileToolBar = new QToolBar(tr("File Toolbar"), this);
	this->addToolBar(m_fileToolBar);
	
	m_fileToolBar->addAction(m_newAction);
	m_fileToolBar->addAction(m_openAction);
	m_fileToolBar->addAction(m_saveAction);
	
	m_techniqueToolBar = new QToolBar(tr("Technique Toolbar"), this);
	this->addToolBar(m_techniqueToolBar);
	
	m_techniqueCombo = new QComboBox();
	m_techniqueCombo->setEditable(false);
	m_techniqueCombo->setInsertPolicy(QComboBox::InsertAtBottom);
	m_techniqueCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_techniqueCombo->setEnabled(false);
	connect(m_techniqueCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(techniqueChanged(int)));
	
	QLabel * techniqueLabel = new QLabel(tr("Technique: "), m_techniqueToolBar);
	techniqueLabel->setBuddy(m_techniqueCombo);
	m_techniqueToolBar->addWidget(techniqueLabel);
	m_techniqueToolBar->addWidget(m_techniqueCombo);
}

void QShaderEdit::createStatusbar()
{
	statusBar()->showMessage(tr("Ready"));
	//m_positionLabel = new QLabel(statusBar());
	//statusBar()->addWidget(m_positionLabel);
	//m_positionLabel->setText("Test!");
}

void QShaderEdit::createDockWindows()
{
	m_logViewDock = new MessagePanel(tr("Messages"), this);
	m_logViewDock->setAllowedAreas(Qt::BottomDockWidgetArea);
	m_logViewDock->setVisible(false);
	addDockWidget(Qt::BottomDockWidgetArea, m_logViewDock);
	connect(m_logViewDock, SIGNAL(messageClicked(int, int, int)), m_editor, SLOT(gotoLine(int, int, int)));
	
	m_sceneViewDock = new QDockWidget(tr("Scene"), this);
	m_sceneViewDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	
	if( QGLFormat::hasOpenGL() )
	{
		QGLFormat format;
		//format.setRgba(true);
		//format.setAlpha(false);
		format.setDepth(true);
		//format.setStencil(false);	// not for now...
		format.setDoubleBuffer(true);	
		m_sceneView = new QGLView(format, m_sceneViewDock);
        connect( m_sceneView, SIGNAL(fileDropped(const QString&)),
                 this, SLOT(load(const QString&)) );
		
		if( !m_sceneView->init(m_logViewDock) ) {
			QMessageBox::critical(this, tr("Error"), tr("OpenGL initialization failed."));
			m_logViewDock->setVisible(true);
		}
		m_sceneViewDock->setWidget(m_sceneView);
	}
	addDockWidget(Qt::RightDockWidgetArea, m_sceneViewDock);
	
	m_paramViewDock = new ParameterPanel(tr("Parameters"), this);
	m_paramViewDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	addDockWidget(Qt::RightDockWidgetArea, m_paramViewDock);
	connect(m_paramViewDock, SIGNAL(parameterChanged()), this, SLOT(setModified()));
}

void QShaderEdit::loadSettings()
{
    QSettings pref( "Castano Inc", "QShaderEdit" );

    pref.beginGroup("MainWindow");
    resize(pref.value("size", QSize(640,480)).toSize());
    pref.endGroup();

    m_autoCompile = pref.value("autoCompile", true).toBool();
    m_openDir     = pref.value("openDir", ".").toString();
}

void QShaderEdit::saveSettings()
{
    QSettings pref( "Castano Inc", "QShaderEdit" );

    pref.beginGroup("MainWindow");
    pref.setValue("size", size());
    pref.endGroup();

    pref.setValue("autoCompile", m_autoCompile);
    pref.setValue("openDir", m_openDir);
}

void QShaderEdit::closeEffect()
{
	// Delete effect.
	m_sceneView->resetEffect();
	delete m_effect;
	m_effect = NULL;
	
	m_paramViewDock->clear();

	updateEditor();
	updateTechniques();
	
	// Delete effect file.
	delete m_file;
	m_file = NULL;
}

void QShaderEdit::updateWindowTitle()
{
	QString title;
	
	if( m_file == NULL ) {
		title = tr("Untitled");
	}
	else {
		QFileInfo info(*m_file);
		title = info.fileName();
	}
	
	if( m_modified ) {
		title.append(" ");
		title.append(tr("[modified]"));
	}
	
	title.append(" - ");
	
	title.append("QShaderEditor"); //QApplication::instance()->applicationName();
	
	
	this->setWindowTitle(title);
}

void QShaderEdit::updateActions()
{
	if(m_effectFactory == NULL) {
		return;
	}
	
	//m_saveAction->setVisible(m_file != NULL);
	m_saveAction->setEnabled(m_modified);
	m_saveAsAction->setEnabled(true);
	
	QString fileName;
	if( m_file == NULL ) {
		QString extension = m_effectFactory->extension();
		fileName = tr("untitled") + "." + extension;
	}
	else {
		QFileInfo info(*m_file);
		fileName = info.fileName();
	}
	
	m_saveAction->setText(tr("Save %1").arg(fileName));
}

void QShaderEdit::updateEditor()
{
	if( m_effect == NULL ) {
		// Remove all the tabs from the editor.
		while(m_editor->count() > 0) {
			m_editor->removeTab(0);
		}
	}
	else {
		// Open all the inputs in tabs.
		int inputNum = m_effect->getInputNum();
		for(int i = 0; i < inputNum; i++) {
			QTextEdit * textEdit = m_editor->addEditor(m_effect->getInputName(i));
			textEdit->setPlainText(m_effect->getInput(i));
			textEdit->document()->setModified(false);

			Highlighter* hl = new Highlighter(textEdit->document());
			//TODO why does m_effect->factory() return NULL?
			hl->setRules(m_effectFactory->highlightingRules());
			hl->setMultiLineCommentStart(m_effectFactory->multiLineCommentStart());
			hl->setMultiLineCommentEnd(m_effectFactory->multiLineCommentEnd());
		}
	}
	
	m_modified = false;
}

void QShaderEdit::updateTechniques()
{
	m_techniqueCombo->clear();
	if(m_effect == NULL) {
		m_techniqueCombo->setEnabled(false);
	}
	else {
		int count = m_effect->getTechniqueNum();
		for(int i = 0; i < count; i++) {
			m_techniqueCombo->addItem(m_effect->getTechniqueName(i));
		}
		m_techniqueCombo->setEnabled(count > 1);
	}
}

bool QShaderEdit::isModified()
{
	Q_ASSERT(m_effect != NULL);
	Q_ASSERT(m_editor != NULL);
	
	bool modified = false;
	
	const int num = m_editor->count();
	for(int i = 0; i < num; i++) {
		QTextEdit * editor = qobject_cast<QTextEdit *>(m_editor->widget(i));
		
		if( editor != NULL && editor->document()->isModified() ) {
			modified = true;
		}
	}
	
	return modified;
}

void QShaderEdit::setModified(bool modified)
{
	Q_ASSERT(m_effect != NULL);
	Q_ASSERT(m_editor != NULL);
	
	const int num = m_editor->count();
	for(int i = 0; i < num; i++) {
		QTextEdit * editor = qobject_cast<QTextEdit *>(m_editor->widget(i));
		
		if( editor != NULL  ) {
			editor->document()->setModified(modified);
		}
	}
	
	m_modified = modified;
}

void QShaderEdit::techniqueChanged(int index)
{
	if( index >= 0 ) {
		Q_ASSERT(m_effect != NULL);
		m_effect->selectTechnique(index);
		m_sceneView->updateGL();
	}
}

void QShaderEdit::cursorPositionChanged()
{
	//m_positionLabel->setText(QString("%1,%2").arg(23).arg(1));
}

void QShaderEdit::closeEvent(QCloseEvent * event)
{
	Q_ASSERT(event != NULL);
	
	if( m_effectFactory != NULL && m_modified ) {
		QString fileName;
		if( m_file != NULL ) {
			QFileInfo info(*m_file);
			fileName = info.fileName();
		}
		else
		{
			QString extension = m_effectFactory->extension();
			fileName = tr("untitled") + "." + extension;
		}
		
		while(true) {
			int answer = QMessageBox::question(this, tr("Save modified files"), tr("Do you want to save '%1' before closing?").arg(fileName), QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
			if( answer == QMessageBox::Yes ) {
				if( save() ) {
					break;
				}
			}
			else if( answer == QMessageBox::Cancel ) {
				event->ignore();
				return;
			}
			else {
				break;
			}
		}
	}
    saveSettings();
	event->accept();
}

void QShaderEdit::keyPressEvent(QKeyEvent * event)
{
	if( event->key() == Qt::Key_Escape ) {
		m_logViewDock->close();
		if( m_editor->currentWidget() ) {
			m_editor->currentWidget()->setFocus();
		}
		event->accept();
	}
	else if( event->key() == Qt::Key_F7 ) {
		event->ignore();
		// @@ Trigger build when auto build is disabled.
	}
	else {
		event->ignore();
	}
}


void QShaderEdit::newEffect(const EffectFactory * effectFactory)
{
	Q_ASSERT(effectFactory != NULL);
	
	closeEffect();
	
	m_effectFactory = effectFactory;
	m_effect = m_effectFactory->createEffect();
	Q_ASSERT(m_effect != NULL);
	
	updateEditor();
	updateWindowTitle();
	updateActions();
	updateTechniques();
	
	// Init scene view.
	m_sceneView->setEffect(m_effect);
	build(true);
}

void QShaderEdit::newFile()
{
	// Count effect file types.
	const QList<const EffectFactory *> & effectFactoryList = EffectFactory::factoryList();
	
	int count = 0;
	foreach(const EffectFactory * ef, effectFactoryList) {
		if(ef->isSupported()) {
			++count;
		}
	}
	
	const EffectFactory * effectFactory = NULL;
	if(count == 0) {
		// Display error.
		QMessageBox::critical(this, tr("Error"), tr("No effect files supported"), QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
	}
	else if(count == 1) {
		// Use the first supported effect type.
		foreach(const EffectFactory * ef, effectFactoryList) {
			if(ef->isSupported()) {
				effectFactory = ef;
				break;
			}
		}
	}
	else {
		// Let the user choose the effect type.
		NewDialog newDialog(this);
		
		int result = newDialog.exec();
		if( result == QDialog::Accepted ) {
			QString selection = newDialog.shaderType();
			
			foreach(const EffectFactory * ef, effectFactoryList) {
				if(ef->isSupported() && ef->name() == selection) {
					effectFactory = ef;
					break;
				}
			}
		}
	}
	
	if( effectFactory != NULL ) {
		newEffect(effectFactory);
	}
}

void QShaderEdit::open()
{
	// Find supported effect types.
	QStringList effectTypes;
	QStringList effectExtensions;
	foreach(const EffectFactory * factory, EffectFactory::factoryList()) {
		if( factory->isSupported() ) {
			QString effectType = QString("%1 (*.%2)").arg(factory->namePlural()).arg(factory->extension());
			effectTypes.append(effectType);
			
			QString effectExtension = factory->extension();
			effectExtensions.append("*." + effectExtension);
		}
	}
	
	if(effectTypes.isEmpty()) {
		QMessageBox::critical(this, tr("Error"), tr("No effect files supported"), QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		return;
	}
	
	//QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), tr("."), effectTypes.join(";"));
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
            m_openDir, QString(tr("Effect Files (%1)")).arg(effectExtensions.join(" ")));
	
	if( ! fileName.isEmpty() ) {
        m_openDir = fileName;
        load( fileName );
	}
}

void QShaderEdit::load( const QString& fileName )
{
	closeEffect();
	
	m_file = new QFile(fileName);
	m_file->open(QIODevice::ReadOnly);
	
	int idx = fileName.lastIndexOf('.');
	QString fileExtension = fileName.mid(idx+1);
	
	m_effectFactory = EffectFactory::factoryForExtension(fileExtension);
	if( m_effectFactory ) {
		m_effect = m_effectFactory->createEffect();
		m_effect->load(m_file);
	}
	
	m_file->close();
	
	updateEditor();
	updateWindowTitle();
	updateActions();
	updateTechniques();
	
	// Init scene view.
	if( m_effect ) {
		m_sceneView->setEffect(m_effect);
		build(false);
	}
	else
	{
		m_sceneView->resetEffect();
	}
}

bool QShaderEdit::save()
{
	if( m_file == NULL ) {
		// Choose file dialog.
		QString fileExtension = m_effectFactory->extension();
		QString effectType = QString("%1 (*.%2)").arg(m_effectFactory->namePlural()).arg(fileExtension);
		QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), tr("untitled") + "." + fileExtension, effectType);
		
		if( fileName.isEmpty() ) {
			return false;
		}
		
		m_file = new QFile(fileName);
	}
	
	m_file->open(QIODevice::WriteOnly);
	m_effect->save(m_file);
	m_file->close();
	
	setModified(false);
	
	updateWindowTitle();
	updateActions();
	
	return true;
}

void QShaderEdit::saveAs()
{
	Q_ASSERT(m_effectFactory != NULL);
	
	QString fileExtension = m_effectFactory->extension();
	QString effectType = QString("%1 (*.%2)").arg(m_effectFactory->namePlural()).arg(fileExtension);
	
	QString fileName; 
	if( m_file != NULL ) {
		QFileInfo info(*m_file);
		fileName = info.fileName();
	}
	else
	{
		fileName = tr("untitled") + "." + fileExtension;
	}
	
	// Choose file dialog.	
	fileName = QFileDialog::getSaveFileName(this, tr("Save File"), fileName, effectType);
	
	if( fileName.isEmpty() ) {
		return;
	}
	
	m_file = new QFile(fileName);
	
	m_file->open(QIODevice::WriteOnly);
	m_effect->save(m_file);
	m_file->close();
	
	setModified(false);
	
	updateWindowTitle();
	updateActions();
}

void QShaderEdit::about()
{
	QMessageBox::about(this, tr("About QShaderEdit"),
			tr("<b>QShaderEdit</b> is a simple shader editor"));
}

void QShaderEdit::setAutoCompile(bool enable)
{
	m_autoCompile = enable;
	if( m_autoCompile ) {
		m_timer->stop();
	}
}

void QShaderEdit::shaderTextChanged()
{
	bool modified = isModified();
	
	// if modified changed.
	if( m_modified != modified ) {
		m_modified = modified;
		updateWindowTitle();
		updateActions();
	}
	
	if( m_autoCompile )
	{
		// Compile after 1.5 seconds of inactivity.
		m_timer->start(1500);
	}
}

void QShaderEdit::keyTimeout()
{
	// Stop the timer.
	m_timer->stop();
	
	// @@ Set status bar message!
	statusBar()->showMessage(tr("Compiling..."));
	
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	
	
	// Update the effect.
	int inputNum = m_effect->getInputNum();
	for(int i = 0; i < inputNum; i++) {
		QTextEdit * textEdit = qobject_cast<QTextEdit *>(m_editor->widget(i));
		if( textEdit != NULL ) {
			m_effect->setInput(i, textEdit->toPlainText().toLatin1());
		}
	}
	
	// Compile the effect.
	build(false);

	QApplication::restoreOverrideCursor();
}


void QShaderEdit::build(bool silent)
{
	Q_ASSERT(m_effect != NULL);
	
	m_paramViewDock->clear();
	
	if( silent ) {
		m_effect->build(NULL);
	}
	else {
		if( m_effect->build(m_logViewDock) ) {
			statusBar()->showMessage(tr("Compilation succeed."), 2000);
			m_logViewDock->info(tr("Compilation succeed."));
		}
		else {
			statusBar()->showMessage(tr("Compilation failed."), 2000);
			m_logViewDock->setVisible(true);
			m_logViewDock->error(tr("Compilation failed."));
		}
	}
	
	updateTechniques();
	m_paramViewDock->setEffect(m_effect);
	
	if( m_effect->isAnimated() ) {
		m_animationTimer->start(30);
	}
	else {
		m_animationTimer->stop();
	}
	
	m_sceneView->updateGL();
}

void QShaderEdit::setModified()
{
	m_modified = true;
	updateActions();
	updateWindowTitle();
	m_sceneView->updateGL();
}
