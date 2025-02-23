#pragma once
#include "opencv-helpers.hpp"
#include "area-selection.hpp"
#include "preview-dialog.hpp"
#include "paramerter-wrappers.hpp"

#include <macro.hpp>
#include <file-selection.hpp>
#include <screenshot-helper.hpp>
#include <slider-spinbox.hpp>
#include <variable-text-edit.hpp>

#include <QWidget>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QRect>

namespace advss {

class PreviewDialog;

class MacroConditionVideo : public MacroCondition {
public:
	MacroConditionVideo(Macro *m) : MacroCondition(m, true){};
	bool CheckCondition();
	bool Save(obs_data_t *obj) const;
	bool Load(obs_data_t *obj);
	std::string GetShortDesc() const;
	std::string GetId() const { return id; };
	static std::shared_ptr<MacroCondition> Create(Macro *m)
	{
		return std::make_shared<MacroConditionVideo>(m);
	}
	QImage GetMatchImage() const { return _matchImage; };
	void GetScreenshot(bool blocking = false);
	bool LoadImageFromFile();
	bool LoadModelData(std::string &path);
	std::string GetModelDataPath() const;
	void ResetLastMatch() { _lastMatchResult = false; }
	double GetCurrentBrightness() const { return _currentBrightness; }
	void SetPageSegMode(tesseract::PageSegMode);

	VideoInput _video;
	VideoCondition _condition = VideoCondition::MATCH;
	std::string _file = obs_module_text("AdvSceneSwitcher.enterPath");
	// Enabling this will reduce matching latency, but slow down the
	// the condition checks of all macros overall.
	//
	// If not set the screenshot will be gathered in one interval and
	// checked in the next one.
	// If set both operations will happen in the same interval.
	bool _blockUntilScreenshotDone = false;
	NumberVariable<double> _brightnessThreshold = 0.5;
	PatternMatchParameters _patternMatchParameters;
	ObjDetectParameters _objMatchParameters;
	OCRParameters _ocrParamters;
	AreaParamters _areaParameters;
	bool _throttleEnabled = false;
	int _throttleCount = 3;

private:
	bool OutputChanged();
	bool ScreenshotContainsPattern();
	bool ScreenshotContainsObject();
	bool CheckBrightnessThreshold();
	bool CheckOCR();
	bool Compare();
	bool CheckShouldBeSkipped();

	bool _getNextScreenshot = true;
	ScreenshotHelper _screenshotData;
	QImage _matchImage;
	PatternImageData _patternImageData;

	bool _lastMatchResult = false;
	int _runCount = 0;

	double _currentBrightness = 0.;

	static bool _registered;
	static const std::string id;
};

class MacroConditionVideoEdit : public QWidget {
	Q_OBJECT

public:
	MacroConditionVideoEdit(
		QWidget *parent,
		std::shared_ptr<MacroConditionVideo> cond = nullptr);
	void UpdateEntryData();
	static QWidget *Create(QWidget *parent,
			       std::shared_ptr<MacroCondition> cond)
	{
		return new MacroConditionVideoEdit(
			parent,
			std::dynamic_pointer_cast<MacroConditionVideo>(cond));
	}

	void UpdatePreviewTooltip();

private slots:
	void VideoInputTypeChanged(int);
	void SourceChanged(const SourceSelection &);
	void SceneChanged(const SceneSelection &);
	void ConditionChanged(int cond);
	void ReduceLatencyChanged(int value);

	void ImagePathChanged(const QString &text);
	void ImageBrowseButtonClicked();

	void UsePatternForChangedCheckChanged(int value);
	void PatternThresholdChanged(const NumberVariable<double> &);
	void UseAlphaAsMaskChanged(int value);
	void PatternMatchModeChanged(int value);

	void BrightnessThresholdChanged(const NumberVariable<double> &);

	void ModelPathChanged(const QString &text);
	void ObjectScaleThresholdChanged(const NumberVariable<double> &);
	void MinNeighborsChanged(int value);
	void MinSizeChanged(Size value);
	void MaxSizeChanged(Size value);

	void SelectColorClicked();
	void MatchTextChanged();
	void RegexChanged(RegexConfig conf);
	void PageSegModeChanged(int);

	void CheckAreaEnableChanged(int value);
	void CheckAreaChanged(Area);
	void CheckAreaChanged(QRect area);
	void SelectAreaClicked();

	void ThrottleEnableChanged(int value);
	void ThrottleCountChanged(int value);
	void ShowMatchClicked();

	void UpdateCurrentBrightness();
signals:
	void VideoSelectionChanged(const VideoInput &);
	void HeaderInfoChanged(const QString &);

private:
	void SetWidgetVisibility();
	void HandleVideoInputUpdate();
	void SetupPreviewDialogParams();
	void SetupColorLabel(const QColor &);

	QComboBox *_videoInputTypes;
	SceneSelectionWidget *_scenes;
	SourceSelectionWidget *_sources;
	QComboBox *_condition;

	QCheckBox *_reduceLatency;

	QCheckBox *_usePatternForChangedCheck;
	FileSelection *_imagePath;

	SliderSpinBox *_patternThreshold;
	QCheckBox *_useAlphaAsMask;
	QHBoxLayout *_patternMatchModeLayout;
	QComboBox *_patternMatchMode;

	SliderSpinBox *_brightnessThreshold;
	QLabel *_currentBrightness;

	QVBoxLayout *_ocrLayout;
	VariableTextEdit *_matchText;
	RegexConfigWidget *_regex;
	QLabel *_textColor;
	QPushButton *_selectColor;
	QComboBox *_pageSegMode;

	FileSelection *_modelDataPath;
	QHBoxLayout *_modelPathLayout;
	SliderSpinBox *_objectScaleThreshold;
	QHBoxLayout *_neighborsControlLayout;
	QSpinBox *_minNeighbors;
	QLabel *_minNeighborsDescription;
	QHBoxLayout *_sizeLayout;
	SizeSelection *_minSize;
	SizeSelection *_maxSize;

	QHBoxLayout *_checkAreaControlLayout;
	QCheckBox *_checkAreaEnable;
	AreaSelection *_checkArea;
	QPushButton *_selectArea;

	QHBoxLayout *_throttleControlLayout;
	QCheckBox *_throttleEnable;
	QSpinBox *_throttleCount;

	QPushButton *_showMatch;
	PreviewDialog _previewDialog;

	QTimer _updateBrightnessTimer;

	std::shared_ptr<MacroConditionVideo> _entryData;
	bool _loading = true;
};

} // namespace advss
