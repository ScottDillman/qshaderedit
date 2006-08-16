#include "parameterpanel.h"
#include "effect.h"

#include <QtGui/QHeaderView>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QLineEdit>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <QtGui/QFileDialog>
#include <QtGui/QApplication>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QColorDialog>


namespace {
	static QString s_lastPath = ".";
}

//
// FilePicker Widget.
//

FileEditor::FileEditor(QWidget * parent /*= 0*/) : QWidget(parent)
{
	setAutoFillBackground(true);

	QHBoxLayout * m_layout = new QHBoxLayout(this);
	m_layout->setMargin(0);
	m_layout->setSpacing(0);

	m_lineEdit = new QLineEdit(this);
	m_lineEdit->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding));
	m_lineEdit->setFrame(false);
	m_layout->addWidget(m_lineEdit);

	QToolButton * button = new QToolButton(this);
	button->setToolButtonStyle(Qt::ToolButtonTextOnly);
	button->setText("...");
	button->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));
	m_layout->addWidget(button);
	connect(button, SIGNAL(clicked()), this, SLOT(openFileDialog()));

	setFocusProxy(m_lineEdit);
}

void FileEditor::openFileDialog()
{
	emit activated();
	QString fileName = QFileDialog::getOpenFileName(this, "Choose file", s_lastPath, "Images (*.png *.jpg)");
	if( !fileName.isEmpty() ) {
		m_lineEdit->setText(fileName);
		s_lastPath = fileName;
	}
	emit done(this);
}

QString FileEditor::text() const
{
	return m_lineEdit->text();
}

void FileEditor::setText(const QString & str)
{
	m_lineEdit->setText(str);
}


//
// Color Editor
//

ColorEditor::ColorEditor(QColor color /*= Qt::black*/, QWidget* parent /*= 0*/):
	QWidget(parent), m_color(color)
{
	init();
}

ColorEditor::ColorEditor(QWidget* parent /*= 0*/): QWidget(parent)
{
	init();
}

void ColorEditor::init()
{
	m_colorLabel = new QLabel(this);
	m_colorLabel->setAutoFillBackground(true);
	updateLabel();

	QToolButton * button = new QToolButton(this);
	button->setToolButtonStyle(Qt::ToolButtonTextOnly);
	button->setText("...");
	connect(button, SIGNAL(clicked()), this, SLOT(openColorPicker()));

	QHBoxLayout * layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(m_colorLabel);
	layout->addWidget(button);
	 
	setFocusProxy(button);
}

QColor ColorEditor::color() const
{
	return m_color;
}

int ColorEditor::components() const
{
	return m_components;
}

void ColorEditor::setColor(QColor color, int components)
{
	m_color = color;
	m_components = components;
	updateLabel();
}

void ColorEditor::updateLabel()
{
	QString text = "[";
	text += QString().setNum(m_color.redF(), 'g', 3) + ", ";
	text += QString().setNum(m_color.greenF(), 'g', 3) + ", ";
	text += QString().setNum(m_color.blueF(), 'g', 3);
	if (m_components == 4) text += ", " + QString().setNum(m_color.alphaF(), 'g', 3);
	text += "]";

	m_colorLabel->setText(text);
}

void ColorEditor::openColorPicker()
{
	emit activated();
	QColor color = QColorDialog::getColor(m_color);
	if (color.isValid()) {
		m_color = color;
		updateLabel();
	}
	emit done(this);
}


//
// DoubleNumInput
//

DoubleNumInput::DoubleNumInput(QWidget * parent) : QWidget(parent)
{
	setAutoFillBackground(true);

	m_spinBox = new QDoubleSpinBox(this);
	m_spinBox->setRange(0.0, 1.0);
	m_spinBox->setDecimals(2);
	m_spinBox->setSingleStep(0.1);
	m_spinBox->setFocusProxy(this);
	connect(m_spinBox, SIGNAL(valueChanged(double)), this, SLOT(spinBoxValueChanged(double)));

	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setRange(0, 10);
	m_slider->setFocusProxy(this);
	connect(m_slider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));

	QHBoxLayout * layout = new QHBoxLayout(this);
	layout->setMargin(2);
	layout->addWidget(m_slider);
	layout->addWidget(m_spinBox);
}

double DoubleNumInput::value() const
{
	return m_spinBox->value();
}

void DoubleNumInput::setSingleStep(double step)
{
	m_spinBox->setSingleStep(step);
	m_slider->setRange(
		qRound(m_spinBox->minimum() / m_spinBox->singleStep()),
		qRound(m_spinBox->maximum() / m_spinBox->singleStep()));
}

void DoubleNumInput::setPageStep(double step)
{
	m_slider->setPageStep(qRound(step / m_spinBox->singleStep()));
}

void DoubleNumInput::setDecimals(int decimals)
{
	m_spinBox->setDecimals(decimals);
}

void DoubleNumInput::setRange(double min, double max)
{
	m_spinBox->setRange(min, max);
	m_slider->setRange(qRound(min / m_spinBox->singleStep()), qRound(max / m_spinBox->singleStep()));
}

void DoubleNumInput::setValue(double value)
{
	m_spinBox->setValue(value);
	m_slider->setValue(qRound(value / m_spinBox->singleStep()));
}

void DoubleNumInput::keyPressEvent(QKeyEvent* event)
{
	m_slider->event(event);
}

void DoubleNumInput::sliderValueChanged(int value)
{
	double dvalue = (double)value * m_spinBox->singleStep();
	m_spinBox->setValue(dvalue);
}

void DoubleNumInput::spinBoxValueChanged(double value)
{
	int ivalue = qRound(value / m_spinBox->singleStep());
	m_slider->setValue(ivalue);
	emit valueChanged(value);
}


//
// ParameterDelegate
//

QWidget * ParameterDelegate::createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	Q_UNUSED(option);
	Q_UNUSED(index);

	const ParameterTableModel * model = qobject_cast<const ParameterTableModel *>(index.model());
	Q_ASSERT(model != NULL);

	if( model->useNumericEditor(index)  ) {
		QVariant value = model->data(index, Qt::EditRole);

		if( value.type() == QVariant::Double ) {
			QDoubleSpinBox * editor = new QDoubleSpinBox(parent);
			editor->setRange(-1000, 1000);
			editor->setSingleStep( 0.1 );
			editor->setDecimals( 3 );
			editor->installEventFilter(const_cast<ParameterDelegate*>(this));
			connect(editor, SIGNAL(valueChanged(double)), this, SLOT(editorValueChanged()));
			return editor;
		}
	}
	else if( model->useColorEditor(index) ) {
		if (model->isComponent(index)) {
			DoubleNumInput * editor = new DoubleNumInput(parent);
			editor->setSingleStep(0.05);
			editor->setPageStep(0.1);
			editor->installEventFilter(const_cast<ParameterDelegate*>(this));
			connect(editor, SIGNAL(valueChanged(double)), this, SLOT(editorValueChanged()));
			return editor;
		}
		else {
			ColorEditor* editor = new ColorEditor(parent);
			editor->installEventFilter(const_cast<ParameterDelegate*>(this));
			connect(editor, SIGNAL(activated()), this, SLOT(editorOpened()));
			connect(editor, SIGNAL(done(QWidget*)), this, SIGNAL(commitData(QWidget*)));	// emit these in editorClosed
			connect(editor, SIGNAL(done(QWidget*)), this, SIGNAL(closeEditor(QWidget*)));
			connect(editor, SIGNAL(done(QWidget*)), this, SLOT(editorClosed(QWidget*)));
			return editor;
		}
	}
	else if( model->useFileEditor(index) ) {
		FileEditor * editor = new FileEditor(parent);
		editor->installEventFilter(const_cast<ParameterDelegate*>(this));
		connect(editor, SIGNAL(activated()), this, SLOT(editorOpened()));
		connect(editor, SIGNAL(done(QWidget*)), this, SIGNAL(commitData(QWidget*)));	// emit these in editorClosed
		connect(editor, SIGNAL(done(QWidget*)), this, SIGNAL(closeEditor(QWidget*)));
		connect(editor, SIGNAL(done(QWidget*)), this, SLOT(editorClosed(QWidget*)));
		return editor;
	}

	// default
	return QItemDelegate::createEditor(parent, option, index);
}

void ParameterDelegate::setEditorData(QWidget * editor, const QModelIndex & index) const
{
	const ParameterTableModel * model = qobject_cast<const ParameterTableModel *>(index.model());
	Q_ASSERT(model != NULL);

	if( model->useNumericEditor(index) ) {
		QVariant value = model->data(index, Qt::EditRole);

		if( value.type() == QVariant::Double ) {
			double value = model->data(index, Qt::DisplayRole).toDouble();
			QDoubleSpinBox * spinBox = static_cast<QDoubleSpinBox *>(editor);
			spinBox->setValue(value);
			return;
		}
	}
	else if( model->useColorEditor(index) ) {
		if (model->isComponent(index)) {
			double value = model->data(index, Qt::EditRole).toDouble();
			DoubleNumInput * input = static_cast<DoubleNumInput*>(editor);
			input->setValue(value);
		}
		else {
			QVariantList value = model->data(index, Qt::EditRole).toList();
			QColor color;

			if (value.size() == 3)
				color.setRgbF(value.at(0).toDouble(), value.at(1).toDouble(), value.at(2).toDouble());
			else
				color.setRgbF(value.at(0).toDouble(), value.at(1).toDouble(), value.at(2).toDouble(), value.at(3).toDouble());
			
			ColorEditor * colorEditor = static_cast<ColorEditor*>(editor);
			colorEditor->setColor(color, value.size());
		}
	}
	else if( model->useFileEditor(index) ) {
		QVariant value = model->data(index, Qt::EditRole);

		FileEditor * fileEditor = static_cast<FileEditor *>(editor);
		fileEditor->setText(value.toString());
		return;
	}
	else	// default
		QItemDelegate::setEditorData(editor, index);
}

void ParameterDelegate::setModelData(QWidget * editor, QAbstractItemModel * abstractModel, const QModelIndex & index) const
{
	ParameterTableModel * model = qobject_cast<ParameterTableModel *>(abstractModel);
	Q_ASSERT(model != NULL);

	if( model->useNumericEditor(index) ) {
		QVariant value = model->data(index, Qt::EditRole);

		if( value.type() == QVariant::Double ) {
			QDoubleSpinBox * spinBox = static_cast<QDoubleSpinBox *>(editor);
			spinBox->interpretText();
			model->setData(index, spinBox->value());
			return;
		}
	}
	else if( model->useColorEditor(index) ) {
		if (model->isComponent(index)) {
			DoubleNumInput * input = static_cast<DoubleNumInput*>(editor);
			model->setData(index, input->value());
		}
		else {
			ColorEditor * colorEditor = static_cast<ColorEditor*>(editor);
			QColor color = colorEditor->color();
			QVariantList value;
			value.append(color.redF());
			value.append(color.greenF());
			value.append(color.blueF());
			if (colorEditor->components() == 4)
				value.append(color.alphaF());
			model->setData(index, value);
		}
		return;
	}
	else if( model->useFileEditor(index) ) {
		FileEditor * fileEditor = static_cast<FileEditor *>(editor);
		model->setData(index, fileEditor->text());
		return;
	}

	// default
	QItemDelegate::setModelData(editor, model, index);
}

void ParameterDelegate::updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	Q_UNUSED(index);
	editor->setGeometry(option.rect);
}

QSize ParameterDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	return QItemDelegate::sizeHint(option, index) + QSize(4, 4);
}

bool ParameterDelegate::eventFilter(QObject* object, QEvent* event)
{
	QWidget* editor = qobject_cast<QWidget*>(object);
	if (!editor)
		return false;

	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = (QKeyEvent*)event;
		switch (keyEvent->key()) {
			case Qt::Key_Up:
				emit commitData(editor);
				emit closeEditor(editor, QAbstractItemDelegate::EditPreviousItem);
				return true;

			case Qt::Key_Down:
				emit commitData(editor);
				emit closeEditor(editor, QAbstractItemDelegate::EditNextItem);
				return true;
		}
	} 

	return QItemDelegate::eventFilter(object, event);
}

bool ParameterDelegate::isEditorActive() const
{
	return m_editorActive;
}

void ParameterDelegate::editorValueChanged()
{
	if (sender() && sender()->isWidgetType())
		emit commitData(static_cast<QWidget *>(sender()));
}

void ParameterDelegate::editorOpened()
{
	m_editorActive = true;
}

void ParameterDelegate::editorClosed(QWidget * editor)
{
	m_editorActive = false;
}

//
// ParameterTableModel
//

QModelIndex ParameterTableModel::index(int row, int column, const QModelIndex & parent) const
{
	if( !parent.isValid() ) {
		return createIndex(row, column, -1);
	}
	else {
		return createIndex(row, column, parent.row());
	}
	return QModelIndex();
}

QModelIndex ParameterTableModel::parent(const QModelIndex & index) const
{
	Q_UNUSED(index);
	if( !index.isValid() || isParameter(index) ) {
		return QModelIndex();
	}
	else {
		return createIndex(parameter(index), 0, -1);
	}
}

int ParameterTableModel::rowCount(const QModelIndex & parent) const
{
	if( m_effect != NULL ) {
		if( !parent.isValid() ) {
			return m_effect->getParameterNum();
		}
		else if( isParameter(parent) ) {
			// Return parameter components.
			QVariant value = m_effect->getParameterValue(parent.row());
			if( value.canConvert(QVariant::List) ) {
				QVariantList list = value.toList();
				return list.count();
			}
		}
	}
	return 0;
}

int ParameterTableModel::columnCount(const QModelIndex & parent) const
{
	Q_UNUSED(parent);
	return 2;
}

QVariant ParameterTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) {
		return QVariant();
	}

	if( isComponent(index) ) {
		// Parameter component.
		if (index.column() == 0) {
			if (role == Qt::DisplayRole || role == Qt::EditRole) {
				Effect::EditorType editorType = m_effect->getParameterEditor(parameter(index));
				if( editorType == Effect::EditorType_Vector ) {
					Q_ASSERT(component(index) < 4);
					return QString("xyzw"[component(index)]);
				}
				else if( editorType == Effect::EditorType_Color ) {
					Q_ASSERT(component(index) < 4);
					return QString("rgba"[component(index)]);
				}
				else if( editorType == Effect::EditorType_Matrix ) {
					const int rows = m_effect->getParameterColumns(parameter(index));
					const int idx = component(index);
					int x = idx / rows;
					int y = idx % rows;
					return "[" + QString::number(x) + ", " + QString::number(y) + "]";
				}
			}
		}
		else if (index.column() == 1) {
			if (role == Qt::DisplayRole || role == Qt::EditRole) {
				QVariantList value = m_effect->getParameterValue(parameter(index)).toList();
				Q_ASSERT( component(index) < value.count() );
				return value.at(component(index));
			}
		}
	}
	else {
		// Root parameter.
		if (index.column() == 0) {
			if (role == Qt::DisplayRole || role == Qt::EditRole) {
				return m_effect->getParameterName(parameter(index));
			}
		}
		else if (index.column() == 1) {
			if (role == Qt::DisplayRole) {
				Effect::EditorType editorType = m_effect->getParameterEditor(parameter(index));
				if( editorType == Effect::EditorType_Matrix ) {
					return "[...]";
				}
				else if (editorType == Effect::EditorType_Color) {
					QVariantList value = m_effect->getParameterValue(parameter(index)).toList();
					QString str = "[";
					str += QString().number(value.at(0).toDouble(), 'g', 3) + ", ";
					str += QString().number(value.at(1).toDouble(), 'g', 3) + ", ";
					str += QString().number(value.at(2).toDouble(), 'g', 3);
					if (value.size() == 4) str += ", " + QString().number(value.at(3).toDouble(), 'g', 3);
					str += "]";
					return str;
				}
				else {
					QVariant value = m_effect->getParameterValue(parameter(index));
					if( value.canConvert(QVariant::String) ) {
						return value;
					}
					else if( value.canConvert(QVariant::StringList) ) {
						return "[" + value.toStringList().join(", ") + "]";
					}
					else {
						/*if(value.isValid()) {
							printf("!! %s\n", value.typeName());
						}
						else {
							printf("!! invalid value returned by getParameterEditor %d\n", index);
						}*/
						printf("!!");
					}
				}
			}
			else if (role == Qt::EditRole) {
				return m_effect->getParameterValue(parameter(index));
			}
			else if (role == Qt::DecorationRole) {
				Effect::EditorType editorType = m_effect->getParameterEditor(parameter(index));
				if (editorType == Effect::EditorType_Color) {
					QVariantList value = m_effect->getParameterValue(parameter(index)).toList();
					QColor color;
					color.setRgbF(value.at(0).toDouble(), value.at(1).toDouble(), value.at(2).toDouble());
					QPixmap pm(12, 12);
					pm.fill(color);
					return pm;
				}
			}
		}
	}

	if (role == Qt::EditRole) {
		return QVariant();
	}

	return QVariant();
}

QVariant ParameterTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole) {
		if (orientation == Qt::Horizontal) {
			if( section == 0 ) return tr("Name");
			else if( section == 1 ) return tr("Value");
		}
	}

	return QVariant();
}

QModelIndex ParameterTableModel::buddy(const QModelIndex & index) const
{
	if (index.column() == 0) {
		return createIndex(index.row(), 1, index.internalId());
	}
	return index;
}

Qt::ItemFlags ParameterTableModel::flags(const QModelIndex &index) const
{
	Q_ASSERT(index.isValid());
	Qt::ItemFlags f = QAbstractItemModel::flags(index);
	if (index.column() == 1 && isEditable(index)) {
		return f | Qt::ItemIsEditable;
	}
	return f;
}

bool ParameterTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	Q_UNUSED(index);
	Q_UNUSED(value);
	Q_UNUSED(role);

	if (!index.isValid() || index.column() != 1)
	{
		return false;
	}
	if (value.isNull())
	{
		return false;
	}

	if (role == Qt::EditRole) {
		if( isComponent(index) ) {
			// Parameter component.
			QVariantList list = m_effect->getParameterValue(parameter(index)).toList();
			Q_ASSERT( component(index) < list.count() );
			list.replace(component(index), value);
			m_effect->setParameterValue(parameter(index), list);
			emit dataChanged(index, index);
			QModelIndex pindex = createIndex(parameter(index), 1, -1);
			emit dataChanged(pindex, pindex);
			return true;
		}
		else {
			// Root parameter.
			m_effect->setParameterValue(parameter(index), value);
			emit dataChanged(index, index);
			return true;
		}
	}

	return false;
}


void ParameterTableModel::clear()
{
	m_effect = NULL;
	reset();
}

void ParameterTableModel::setEffect(Effect * effect)
{
	Q_ASSERT(effect != NULL);
	m_effect = effect;
	reset();
}


bool ParameterTableModel::isEditable(const QModelIndex &index) const
{
	if( isParameter(index) ) {
		Effect::EditorType type = m_effect->getParameterEditor(parameter(index));
		return type == Effect::EditorType_Color || type == Effect::EditorType_Scalar || type == Effect::EditorType_File;
	}
	else {
		// components are always editable.
		return true;
	}
}

// static
bool ParameterTableModel::isParameter(const QModelIndex &index)
{
	return index.internalId() == -1;
}

// static
bool ParameterTableModel::isComponent(const QModelIndex &index)
{
	return index.internalId() != -1;
}

// static
int ParameterTableModel::parameter(const QModelIndex &index)
{
	if( isParameter(index) ) {
		return index.row();
	}
	else {
		return index.internalId();
	}
}

// static
int ParameterTableModel::component(const QModelIndex &index)
{
	Q_ASSERT(isComponent(index));
	return index.row();
}

bool ParameterTableModel::useColorEditor(const QModelIndex &index) const
{
	Q_ASSERT(m_effect != NULL);
	if (!index.isValid())
		return false;
	Effect::EditorType editor = m_effect->getParameterEditor(parameter(index));
	return editor == Effect::EditorType_Color;
}

bool ParameterTableModel::useNumericEditor(const QModelIndex &index) const
{
	Q_ASSERT(m_effect != NULL);
	if (!index.isValid())
		return false;
	Effect::EditorType editor = m_effect->getParameterEditor(parameter(index));
	return editor == Effect::EditorType_Scalar ||
		editor == Effect::EditorType_Vector ||
		editor == Effect::EditorType_Matrix;
}

bool ParameterTableModel::useFileEditor(const QModelIndex &index) const
{
	Q_ASSERT(m_effect != NULL);
	if (!index.isValid())
		return false;
	Effect::EditorType editor = m_effect->getParameterEditor(parameter(index));
	return editor == Effect::EditorType_File;
	//return false;
}



//
// ParameterPanel
//

ParameterPanel::ParameterPanel(const QString & title, QWidget * parent /*= 0*/, Qt::WFlags flags /*= 0*/) :
		QDockWidget(title, parent, flags), m_model(NULL), m_delegate(NULL), m_table(NULL)
{
	initWidget();
}

ParameterPanel::ParameterPanel(QWidget * parent /*= 0*/, Qt::WFlags flags /*= 0*/) :
		QDockWidget(parent, flags), m_model(NULL), m_delegate(NULL), m_table(NULL)
{
	initWidget();
}

ParameterPanel::~ParameterPanel()
{
	delete m_model;
	m_model = NULL;
	delete m_delegate;
	m_delegate = NULL;
	delete m_table;
	m_table = NULL;
}

QSize ParameterPanel::sizeHint() const
{
	return QSize(200, 200);
}

bool ParameterPanel::isEditorActive() const
{
	return m_delegate->isEditorActive();
}

void ParameterPanel::initWidget()
{
	m_model = new ParameterTableModel(this);
	connect(m_model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SIGNAL(parameterChanged()));

	m_delegate = new ParameterDelegate(this);

	m_table = new QTreeView(this);
	m_table->setModel(m_model);
	m_table->setItemDelegate(m_delegate);
	m_table->header()->setStretchLastSection(true);
	m_table->header()->setClickable(false);
	m_table->setAlternatingRowColors(true);
	m_table->setEditTriggers(QAbstractItemView::AllEditTriggers);

	//	m_table->setIndentation(0);	// @@ This would be nice if it didn't affect the roots.

	setWidget(m_table);
}

void ParameterPanel::clear()
{
	m_model->clear();
}

void ParameterPanel::setEffect(Effect * effect)
{
	m_model->setEffect(effect);
	m_table->resizeColumnToContents(0);
}

//static
const QString & ParameterPanel::lastPath()
{
	return s_lastPath;
}

//static
void ParameterPanel::setLastPath(const QString & lastPath)
{
	s_lastPath = lastPath;
}

