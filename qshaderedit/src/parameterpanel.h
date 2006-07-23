#ifndef PARAMETERPANEL_H
#define PARAMETERPANEL_H

#include <QtGui/QDockWidget>
#include <QtGui/QItemDelegate>
#include <QtGui/QTreeView>
#include <QtCore/QAbstractTableModel>

class QTableView;
class Effect;



class FileEditor : public QWidget 
{
	Q_OBJECT
public:
	FileEditor(QWidget * parent = 0);
	
	QString text() const;
	void setText(const QString & str);
	
	QLineEdit * lineEditor() { return m_lineEdit; }
	
private slots:
	void openFileDialog();

signals:
	void done(QWidget *);
	
private:
	QLineEdit * m_lineEdit;
};


class ParameterDelegate : public QItemDelegate
{
	Q_OBJECT
public:
	ParameterDelegate(QObject *parent = 0) : 
		QItemDelegate(parent) {}
	
//	virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const;
//	virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	
	virtual QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const;
	virtual void setEditorData ( QWidget * editor, const QModelIndex & index ) const;
	virtual void setModelData ( QWidget * editor, QAbstractItemModel * model, const QModelIndex & index ) const;
	virtual void updateEditorGeometry(QWidget * editor, const QStyleOptionViewItem & option, const QModelIndex & index) const;
};


class ParameterTableModel : public QAbstractItemModel
{
	Q_OBJECT
public:
	ParameterTableModel(QObject *parent = 0) : 
		QAbstractItemModel(parent), m_effect(NULL) {}

	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &child) const;
	
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;

	virtual QModelIndex buddy(const QModelIndex & index) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole); 	

public slots:
	void clear();	
	void setEffect(Effect * effect);
	
private:
	// Let the delegate access the private methods.
	friend class ParameterDelegate;
	
	// Helper methods.
	bool isEditable(const QModelIndex &index) const;
	static bool isParameter(const QModelIndex &index);
	static bool isComponent(const QModelIndex &index);
	static int parameter(const QModelIndex &index);
	static int component(const QModelIndex &index);
	
	bool useColorEditor(const QModelIndex &index) const;
	bool useNumericEditor(const QModelIndex &index) const;
	bool useFileEditor(const QModelIndex &index) const;
	
	
private:
	
	Effect * m_effect;
	
};


class ParameterPanel : public QDockWidget
{
	Q_OBJECT
public:
	
	ParameterPanel(const QString & title, QWidget * parent = 0, Qt::WFlags flags = 0);
	ParameterPanel(QWidget * parent = 0, Qt::WFlags flags = 0);
	
	virtual QSize sizeHint() const;
	
signals:
	void parameterChanged();
	
public slots:
	void clear();
	void setEffect(Effect * effect);

private:
	void initWidget();
	
private:
	
	ParameterTableModel * m_model;
	ParameterDelegate * m_delegate;	// @@ Not used.
	QTreeView * m_table;
	
};



#endif // PARAMETERPANEL_H