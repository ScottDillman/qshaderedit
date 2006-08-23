
#include <QtCore/QVariant>


class Parameter
{
public:
	virtual ~Parameter() { }
	
	QString name() const { return m_name; }
	QString description() const { return m_description; }
	
	void setDescription(const QString& desc) { m_description = desc; }
	
	QVariant value() const { return m_value; }
	virtual void setValue(const QVariant& value);
	
	int type() const { return m_value.userType(); }
	
	virtual QString displayValue() const;	
	virtual QVariant decoration() const;

	virtual bool isEditable() const;
	
	bool hasRange() const;
	QVariant minValue() const { return m_minValue; } 
	QVariant maxValue() const { return m_maxValue; }
	void setRange(const QVariant& min, const QVariant& max);
	void clearRange();

	virtual int componentCount() const;
	virtual bool componentsAreEditable() const;
	virtual QString componentName(int idx) const;	
	virtual QVariant componentValue(int idx) const;
	virtual QString  componentDisplayValue(int idx) const;
	virtual void setComponentValue(int idx, QVariant value);
	
	virtual int componentType() const; 	// the same for all components
	virtual QVariant componentMinValue() const;
	virtual QVariant componentMaxValue() const;
	 
protected:
	void setName(const QString& name) { m_name = name; }
	
private:
	QString m_name;
	QString m_description;
	
	QVariant m_value;
	QVariant m_minValue, m_maxValue;
};