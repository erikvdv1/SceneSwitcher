#include "macro-action-projector.hpp"
#include "utility.hpp"

#include <QGuiApplication>
#include <QScreen>

namespace advss {

const std::string MacroActionProjector::id = "projector";

bool MacroActionProjector::_registered = MacroActionFactory::Register(
	MacroActionProjector::id,
	{MacroActionProjector::Create, MacroActionProjectorEdit::Create,
	 "AdvSceneSwitcher.action.projector"});

const static std::map<MacroActionProjector::Type, std::string> selectionTypes = {
	{MacroActionProjector::Type::SOURCE,
	 "AdvSceneSwitcher.action.projector.type.source"},
	{MacroActionProjector::Type::SCENE,
	 "AdvSceneSwitcher.action.projector.type.scene"},
	{MacroActionProjector::Type::PREVIEW,
	 "AdvSceneSwitcher.action.projector.type.preview"},
	{MacroActionProjector::Type::PROGRAM,
	 "AdvSceneSwitcher.action.projector.type.program"},
	{MacroActionProjector::Type::MULTIVIEW,
	 "AdvSceneSwitcher.action.projector.type.multiview"},
};

bool MacroActionProjector::PerformAction()
{
	std::string name = "";
	const char *type = "";

	switch (_type) {
	case MacroActionProjector::Type::SOURCE:
		type = "Source";
		name = GetWeakSourceName(_source.GetSource());
		if (name.empty()) {
			return true;
		}
		break;
	case MacroActionProjector::Type::SCENE:
		type = "Scene";
		name = GetWeakSourceName(_scene.GetScene());
		if (name.empty()) {
			return true;
		}
		break;
	case MacroActionProjector::Type::PREVIEW:
		type = "Preview";
		break;
	case MacroActionProjector::Type::PROGRAM:
		type = "StudioProgram";
		break;
	case MacroActionProjector::Type::MULTIVIEW:
		type = "Multiview";
		break;
	}

	obs_frontend_open_projector(type, _fullscreen ? _monitor : -1, "",
				    name.c_str());

	return true;
}

void MacroActionProjector::LogAction() const
{
	auto it = selectionTypes.find(_type);
	if (it != selectionTypes.end()) {
		vblog(LOG_INFO,
		      "performed projector action \"%s\" with"
		      "source \"%s\","
		      "scene \"%s\","
		      "monitor %d",
		      it->second.c_str(), _source.ToString(true).c_str(),
		      _scene.ToString(true).c_str(), _monitor);
	} else {
		blog(LOG_WARNING, "ignored unknown projector action %d",
		     static_cast<int>(_type));
	}
}

bool MacroActionProjector::Save(obs_data_t *obj) const
{
	MacroAction::Save(obj);
	obs_data_set_int(obj, "type", static_cast<int>(_type));
	obs_data_set_int(obj, "monitor", _monitor);
	obs_data_set_bool(obj, "fullscreen", _fullscreen);
	_scene.Save(obj);
	_source.Save(obj);
	return true;
}

bool MacroActionProjector::Load(obs_data_t *obj)
{
	MacroAction::Load(obj);
	_type = static_cast<Type>(obs_data_get_int(obj, "type"));
	_monitor = obs_data_get_int(obj, "monitor");
	_fullscreen = obs_data_get_bool(obj, "fullscreen");
	_scene.Load(obj);
	_source.Load(obj);
	return true;
}

static inline void populateSelectionTypes(QComboBox *list)
{
	for (auto entry : selectionTypes) {
		list->addItem(obs_module_text(entry.second.c_str()));
	}
}

static inline void populateWindowTypes(QComboBox *list)
{
	list->addItem(
		obs_module_text("AdvSceneSwitcher.action.projector.windowed"));
	list->addItem(obs_module_text(
		"AdvSceneSwitcher.action.projector.fullscreen"));
}

static QStringList getMonitorNames()
{
	QStringList monitorNames;
	QList<QScreen *> screens = QGuiApplication::screens();
	for (int i = 0; i < screens.size(); i++) {
		QScreen *screen = screens[i];
		QRect screenGeometry = screen->geometry();
		qreal ratio = screen->devicePixelRatio();
		QString name = "";
#if defined(__APPLE__) || defined(_WIN32)
		name = screen->name();
#else
		name = screen->model().simplified();
		if (name.length() > 1 && name.endsWith("-")) {
			name.chop(1);
		}
#endif
		name = name.simplified();

		if (name.length() == 0) {
			name = QString("%1 %2")
				       .arg(obs_module_text(
					       "AdvSceneSwitcher.action.projector.display"))
				       .arg(QString::number(i + 1));
		}
		QString str =
			QString("%1: %2x%3 @ %4,%5")
				.arg(name,
				     QString::number(screenGeometry.width() *
						     ratio),
				     QString::number(screenGeometry.height() *
						     ratio),
				     QString::number(screenGeometry.x()),
				     QString::number(screenGeometry.y()));

		monitorNames << str;
	}
	return monitorNames;
}

MacroActionProjectorEdit::MacroActionProjectorEdit(
	QWidget *parent, std::shared_ptr<MacroActionProjector> entryData)
	: QWidget(parent),
	  _windowTypes(new QComboBox()),
	  _types(new QComboBox()),
	  _scenes(new SceneSelectionWidget(window(), true, false, true, true,
					   true)),
	  _sources(new SourceSelectionWidget(window(), QStringList(), true)),
	  _monitorSelection(new QHBoxLayout()),
	  _monitors(new QComboBox())
{
	populateWindowTypes(_windowTypes);
	populateSelectionTypes(_types);
	auto sources = GetSourceNames();
	sources.sort();
	_sources->SetSourceNameList(sources);
	_monitors->addItems(getMonitorNames());

	QWidget::connect(_windowTypes, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(WindowTypeChanged(int)));
	QWidget::connect(_types, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(TypeChanged(int)));
	QWidget::connect(_scenes, SIGNAL(SceneChanged(const SceneSelection &)),
			 this, SLOT(SceneChanged(const SceneSelection &)));
	QWidget::connect(_sources,
			 SIGNAL(SourceChanged(const SourceSelection &)), this,
			 SLOT(SourceChanged(const SourceSelection &)));
	QWidget::connect(_monitors, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(MonitorChanged(int)));

	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{windowTypes}}", _windowTypes}, {"{{types}}", _types},
		{"{{scenes}}", _scenes},           {"{{sources}}", _sources},
		{"{{monitors}}", _monitors},
	};

	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.action.projector.entry.monitor"),
		     _monitorSelection, widgetPlaceholders);

	QHBoxLayout *mainLayout = new QHBoxLayout;
	PlaceWidgets(obs_module_text("AdvSceneSwitcher.action.projector.entry"),
		     mainLayout, widgetPlaceholders);
	mainLayout->insertLayout(mainLayout->count() - 1, _monitorSelection);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroActionProjectorEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}
	_windowTypes->setCurrentIndex(_entryData->_fullscreen ? 1 : 0);
	_types->setCurrentIndex(static_cast<int>(_entryData->_type));
	_scenes->SetScene(_entryData->_scene);
	_sources->SetSource(_entryData->_source);
	_monitors->setCurrentIndex(_entryData->_monitor);
	SetWidgetVisibility();
}

void MacroActionProjectorEdit::SceneChanged(const SceneSelection &s)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_scene = s;
}

void MacroActionProjectorEdit::SourceChanged(const SourceSelection &source)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_source = source;
}

void MacroActionProjectorEdit::MonitorChanged(int value)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_monitor = value;
}

void MacroActionProjectorEdit::WindowTypeChanged(int)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_fullscreen =
		_windowTypes->currentText() ==
		obs_module_text("AdvSceneSwitcher.action.projector.fullscreen");
	SetWidgetVisibility();
}

void MacroActionProjectorEdit::TypeChanged(int value)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_type = static_cast<MacroActionProjector::Type>(value);
	SetWidgetVisibility();
}

void MacroActionProjectorEdit::SetWidgetVisibility()
{
	if (!_entryData) {
		return;
	}

	_scenes->setVisible(_entryData->_type ==
			    MacroActionProjector::Type::SCENE);
	_sources->setVisible(_entryData->_type ==
			     MacroActionProjector::Type::SOURCE);
	SetLayoutVisible(_monitorSelection, _entryData->_fullscreen);

	adjustSize();
	updateGeometry();
}

} // namespace advss
