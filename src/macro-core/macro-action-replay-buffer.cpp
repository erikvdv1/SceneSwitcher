#include "macro-action-replay-buffer.hpp"
#include "utility.hpp"

namespace advss {

const std::string MacroActionReplayBuffer::id = "replay_buffer";

bool MacroActionReplayBuffer::_registered = MacroActionFactory::Register(
	MacroActionReplayBuffer::id,
	{MacroActionReplayBuffer::Create, MacroActionReplayBufferEdit::Create,
	 "AdvSceneSwitcher.action.replay"});

const static std::map<ReplayBufferAction, std::string> actionTypes = {
	{ReplayBufferAction::STOP, "AdvSceneSwitcher.action.replay.type.stop"},
	{ReplayBufferAction::START,
	 "AdvSceneSwitcher.action.replay.type.start"},
	{ReplayBufferAction::SAVE, "AdvSceneSwitcher.action.replay.type.save"},
};

bool MacroActionReplayBuffer::PerformAction()
{
	switch (_action) {
	case ReplayBufferAction::STOP:
		if (obs_frontend_replay_buffer_active()) {
			obs_frontend_replay_buffer_stop();
		}
		break;
	case ReplayBufferAction::START:
		if (!obs_frontend_replay_buffer_active()) {
			obs_frontend_replay_buffer_start();
		}
		break;
	case ReplayBufferAction::SAVE:
		if (obs_frontend_replay_buffer_active()) {
			obs_frontend_replay_buffer_save();
		}
		break;
	default:
		break;
	}
	return true;
}

void MacroActionReplayBuffer::LogAction() const
{
	auto it = actionTypes.find(_action);
	if (it != actionTypes.end()) {
		vblog(LOG_INFO, "performed action \"%s\"", it->second.c_str());
	} else {
		blog(LOG_WARNING, "ignored unknown replay buffer action %d",
		     static_cast<int>(_action));
	}
}

bool MacroActionReplayBuffer::Save(obs_data_t *obj) const
{
	MacroAction::Save(obj);
	obs_data_set_int(obj, "action", static_cast<int>(_action));
	return true;
}

bool MacroActionReplayBuffer::Load(obs_data_t *obj)
{
	MacroAction::Load(obj);
	_action = static_cast<ReplayBufferAction>(
		obs_data_get_int(obj, "action"));
	return true;
}

static inline void populateActionSelection(QComboBox *list)
{
	for (auto entry : actionTypes) {
		list->addItem(obs_module_text(entry.second.c_str()));
	}
}

MacroActionReplayBufferEdit::MacroActionReplayBufferEdit(
	QWidget *parent, std::shared_ptr<MacroActionReplayBuffer> entryData)
	: QWidget(parent),
	  _actions(new QComboBox()),
	  _saveWarning(new QLabel(
		  obs_module_text("AdvSceneSwitcher.action.replay.saveWarn")))
{
	populateActionSelection(_actions);

	QWidget::connect(_actions, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(ActionChanged(int)));

	QHBoxLayout *layout = new QHBoxLayout;
	std::unordered_map<std::string, QWidget *> widgetPlaceholders = {
		{"{{actions}}", _actions},
	};
	PlaceWidgets(obs_module_text("AdvSceneSwitcher.action.replay.entry"),
		     layout, widgetPlaceholders);
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(layout);
	mainLayout->addWidget(_saveWarning);
	setLayout(mainLayout);

	_entryData = entryData;
	UpdateEntryData();
	_loading = false;
}

void MacroActionReplayBufferEdit::UpdateEntryData()
{
	if (!_entryData) {
		return;
	}
	_actions->setCurrentIndex(static_cast<int>(_entryData->_action));
	_saveWarning->setVisible(_entryData->_action ==
				 ReplayBufferAction::SAVE);
}

void MacroActionReplayBufferEdit::ActionChanged(int value)
{
	if (_loading || !_entryData) {
		return;
	}

	auto lock = LockContext();
	_entryData->_action = static_cast<ReplayBufferAction>(value);
	_saveWarning->setVisible(_entryData->_action ==
				 ReplayBufferAction::SAVE);
	adjustSize();
}

} // namespace advss
