#pragma once
#include "source-selection.hpp"

namespace advss {

class FilterSelection {
public:
	void Save(obs_data_t *obj, const char *name = "filter") const;
	void Load(obs_data_t *obj, const SourceSelection &source,
		  const char *name = "filter");

	enum class Type {
		SOURCE,
		VARIABLE,
	};

	Type GetType() const { return _type; }
	OBSWeakSource GetFilter(const SourceSelection &source) const;
	std::string ToString(bool resolve = false) const;

private:
	// TODO: Remove in future version
	// Used for backwards compatability to older settings versions
	void LoadFallback(obs_data_t *obj, const SourceSelection &source,
			  const char *name);

	OBSWeakSource _filter;
	// Storing the name separately as depending on the source selection
	// the filter source might not be available at the moment.
	std::string _filterName = "";
	std::weak_ptr<Variable> _variable;
	Type _type = Type::SOURCE;
	friend class FilterSelectionWidget;
};

class FilterSelectionWidget : public QComboBox {
	Q_OBJECT

public:
	FilterSelectionWidget(QWidget *parent, SourceSelectionWidget *sources,
			      bool addVariables = true);
	void SetFilter(const SourceSelection &, const FilterSelection &);

signals:
	void FilterChanged(const FilterSelection &);

public slots:
	void SourceChanged(const SourceSelection &);
private slots:
	void SelectionChanged(const QString &name);
	void ItemAdd(const QString &name);
	void ItemRemove(const QString &name);
	void ItemRename(const QString &oldName, const QString &newName);

private:
	void Reset();
	FilterSelection CurrentSelection();
	void PopulateSelection();
	bool NameUsed(const QString &name);

	bool _addVariables;
	FilterSelection _currentSelection;
	SourceSelection _source;

	// Order of entries
	// 1. "select entry" entry
	// 2. Variables
	// 3. Regular filters
	const int _selectIdx = 0;
	int _variablesEndIdx = -1;
	int _filterEndIdx = -1;
};

} // namespace advss
