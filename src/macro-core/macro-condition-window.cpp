#include "macro-condition-window.hpp"
#include "switcher-data.hpp"
#include "platform-funcs.hpp"
#include "utility.hpp"

#include <regex>

namespace advss {

const std::string MacroConditionWindow::id = "window";

bool MacroConditionWindow::_registered = MacroConditionFactory::Register(
	MacroConditionWindow::id,
	{MacroConditionWindow::Create, MacroConditionWindowEdit::Create,
	 "AdvSceneSwitcher.condition.window"});

bool MacroConditionWindow::CheckWindowTitleSwitchDirect(
	const std::string &currentWindowTitle)
{
	bool focus = (!_focus || _window == currentWindowTitle);
	bool fullscreen = (!_fullscreen || IsFullscreen(_window));
	bool max = (!_maximized || IsMaximized(_window));

	return focus && fullscreen && max;
}

bool MacroConditionWindow::CheckWindowTitleSwitchRegex(
	const std::string &currentWindowTitle,
	const std::vector<std::string> &windowList)
{
	bool match = false;
	for (auto &window : windowList) {
		try {
			std::regex expr(_window);
			if (!std::regex_match(window, expr)) {
				continue;
			}
		} catch (const std::regex_error &) {
		}

		bool focus = (!_focus || window == currentWindowTitle);
		bool fullscreen = (!_fullscreen || IsFullscreen(window));
		bool max = (!_maximized || IsMaximized(window));

		if (focus && fullscreen && max) {
			match = true;
			break;
		}
	}

	return match;
}

static bool foregroundWindowChanged()
{
	return switcher->currentTitle != switcher->lastTitle;
}

bool MacroConditionWindow::CheckCondition()
{
	const std::string &currentWindowTitle = switcher->currentTitle;
	SetVariableValue(currentWindowTitle);

	std::vector<std::string> windowList;
	GetWindowList(windowList);

	bool match = false;
	if (std::find(windowList.begin(), windowList.end(), _window) !=
	    windowList.end()) {
		match = CheckWindowTitleSwitchDirect(currentWindowTitle);
	} else {
		match = CheckWindowTitleSwitchRegex(currentWindowTitle,
						    windowList);
	}
	match = match && (!_windowFocusChanged || foregroundWindowChanged());
	return match;
}

bool MacroConditionWindow::Save(obs_data_t *obj) const
{
	MacroCondition::Save(obj);
	obs_data_set_string(obj, "window", _window.c_str());
	obs_data_set_bool(obj, "fullscreen", _fullscreen);
	obs_data_set_bool(obj, "maximized", _maximized);
	obs_data_set_bool(obj, "focus", _focus);
	obs_data_set_bool(obj, "windowFocusChanged", _windowFocusChanged);
	return true;
}

bool MacroConditionWindow::Load(obs_data_t *obj)
{
	MacroCondition::Load(obj);
	_window = obs_data_get_string(obj, "window");
	_fullscreen = obs_data_get_bool(obj, "fullscreen");
	_maximized = obs_data_get_bool(obj, "maximized");
	_focus = obs_data_get_bool(obj, "focus");
	_windowFocusChanged = obs_data_get_bool(obj, "windowFocusChanged");
	return true;
}

std::string MacroConditionWindow::GetShortDesc() const
{
	return _window;
}

MacroConditionWindowEdit::MacroConditionWindowEdit(
	QWidget *parent, std::shared_ptr<MacroConditionWindow> entryData)
	: QWidget(parent),
	  _windowSelection(new QComboBox()),
	  _fullscreen(new QCheckBox()),
	  _maximized(new QCheckBox()),
	  _focused(new QCheckBox()),
	  _windowFocusChanged(new QCheckBox()),
	  _focusWindow(new QLabel()),
	  _focusLayout(new QHBoxLayout())
{
	_windowSelection->setEditable(true);
	_windowSelection->setMaxVisibleItems(20);

	QWidget::connect(_windowSelection,
			 SIGNAL(currentTextChanged(const QString &)), this,
			 SLOT(WindowChanged(const QString &)));
	QWidget::connect(_fullscreen, SIGNAL(stateChanged(int)), this,
			 SLOT(FullscreenChanged(int)));
	QWidget::connect(_maximized, SIGNAL(stateChanged(int)), this,
			 SLOT(MaximizedChanged(int)));
	QWidget::connect(_focused, SIGNAL(stateChanged(int)), this,
			 SLOT(FocusedChanged(int)));
	QWidget::connect(_windowFocusChanged, SIGNAL(stateChanged(int)), this,
			 SLOT(WindowFocusChanged(int)));
	QWidget::connect(&_timer, SIGNAL(timeout()), this,
			 SLOT(UpdateFocusWindow()));

	PopulateWindowSelection(_windowSelection);

	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{windows}}", _windowSelection},
		{"{{fullscreen}}", _fullscreen},
		{"{{maximized}}", _maximized},
		{"{{focused}}", _focused},
		{"{{windowFocusChanged}}", _windowFocusChanged},
		{"{{focusWindow}}", _focusWindow},
	};

	auto *line1Layout = new QHBoxLayout;
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.window.entry.line1"),
		     line1Layout, widgetPlaceholders);
	auto *line2Layout = new QHBoxLayout;
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.window.entry.line2"),
		     line2Layout, widgetPlaceholders);
	PlaceWidgets(obs_module_text(
			     "AdvSceneSwitcher.condition.window.entry.line3"),
		     _focusLayout, widgetPlaceholders);
	auto *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(line1Layout);
	mainLayout->addLayout(line2Layout);
	mainLayout->addLayout(_focusLayout);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;

	_timer.start(1000);
}

void MacroConditionWindowEdit::WindowChanged(const QString &text)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_window = text.toStdString();
	emit HeaderInfoChanged(
		QString::fromStdString(_entryData->GetShortDesc()));
}

void MacroConditionWindowEdit::FullscreenChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_fullscreen = state;
}

void MacroConditionWindowEdit::MaximizedChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_maximized = state;
}

void MacroConditionWindowEdit::FocusedChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_focus = state;
	SetWidgetVisibility();
}

void MacroConditionWindowEdit::WindowFocusChanged(int state)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_windowFocusChanged = state;
	SetWidgetVisibility();
}

void MacroConditionWindowEdit::UpdateFocusWindow()
{
	_focusWindow->setText(QString::fromStdString(switcher->currentTitle));
}

void MacroConditionWindowEdit::SetWidgetVisibility()
{
	if (!_entryData) {
		return;
	}
	SetLayoutVisible(_focusLayout,
			 _entryData->_focus || _entryData->_windowFocusChanged);
	adjustSize();
}

void MacroConditionWindowEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}

	_windowSelection->setCurrentText(_entryData->_window.c_str());
	_fullscreen->setChecked(_entryData->_fullscreen);
	_maximized->setChecked(_entryData->_maximized);
	_focused->setChecked(_entryData->_focus);
	_windowFocusChanged->setChecked(_entryData->_windowFocusChanged);
	SetWidgetVisibility();
}

} // namespace advss
