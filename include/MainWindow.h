#pragma once

#include <QMainWindow>
#include <QVector>
#include <QFileInfoList>
#include "AudioProcessor.h"

QT_BEGIN_NAMESPACE
class QLineEdit;
class QLabel;
class QChartView;
class QValueAxis;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void loadNextFile();
    void loadPreviousFile();
    void applyParameters();
    void saveResults();
    void processAllFiles();
    void updateThreshold(double value);

private:
    void setupUI();
    void loadWavFile(const QString& filePath);
    void updatePlots();
    QWidget* createControlPanel();
    QWidget* createWaveformPlot();
    QWidget* createResultsPlot();

    QFileInfoList wavFiles;
    int currentFileIndex = -1;
    
    // Audio data
    AudioProcessor::AudioData audioData;
    QVector<double> pulseTimes;
    QVector<double> intervals;
    
    // UI Components
    QLineEdit *threshInput, *lengthInput;
    QLineEdit *subjectInput, *experimenterInput;
    QLabel *statusLabel;
    QChartView *waveformChartView;
    QChartView *resultsChartView;
    QValueAxis *waveformXAxis, *waveformYAxis;
    QValueAxis *resultsXAxis, *resultsYAxis;
};