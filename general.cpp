#include "advanced-scene-switcher.hpp"
#include <obs-module.h>

void SceneSwitcher::on_close_clicked()
{
	done(0);
}

void SceneSwitcher::on_startAtLaunch_toggled(bool value)
{
	if (loading)
		return;

	lock_guard<mutex> lock(switcher->m);
	switcher->startAtLaunch = value;
}

void SceneSwitcher::UpdateNonMatchingScene(const QString& name)
{
	obs_source_t* scene = obs_get_source_by_name(name.toUtf8().constData());
	obs_weak_source_t* ws = obs_source_get_weak_source(scene);

	switcher->nonMatchingScene = ws;

	obs_weak_source_release(ws);
	obs_source_release(scene);
}

void SceneSwitcher::on_noMatchDontSwitch_clicked()
{
	if (loading)
		return;

	lock_guard<mutex> lock(switcher->m);
	switcher->switchIfNotMatching = false;
}

void SceneSwitcher::on_noMatchSwitch_clicked()
{
	if (loading)
		return;

	lock_guard<mutex> lock(switcher->m);
	switcher->switchIfNotMatching = true;
	UpdateNonMatchingScene(ui->noMatchSwitchScene->currentText());
}

void SceneSwitcher::on_noMatchSwitchScene_currentTextChanged(const QString& text)
{
	if (loading)
		return;

	lock_guard<mutex> lock(switcher->m);
	UpdateNonMatchingScene(text);
}

void SceneSwitcher::on_checkInterval_valueChanged(int value)
{
	if (loading)
		return;

	lock_guard<mutex> lock(switcher->m);
	switcher->interval = value;
}

void SceneSwitcher::SetStarted()
{
	ui->toggleStartButton->setText(obs_module_text("Stop"));
	ui->pluginRunningText->setText(obs_module_text("Active"));
}

void SceneSwitcher::SetStopped()
{
	ui->toggleStartButton->setText(obs_module_text("Start"));
	ui->pluginRunningText->setText(obs_module_text("Inactive"));
}

void SceneSwitcher::on_toggleStartButton_clicked()
{
	if (switcher->th.joinable())
	{
		switcher->Stop();
		SetStopped();
	}
	else
	{
		switcher->Start();
		SetStarted();
	}
}

void SceneSwitcher::closeEvent(QCloseEvent*)
{
	obs_frontend_save();
}
