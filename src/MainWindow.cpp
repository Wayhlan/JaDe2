#include "MainWindow.h"
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QDir>
#include <QDateTime>
#include <algorithm>

using namespace QtCharts;

MainWindow::MainWindow(QWidget *parent) 
    : QMainWindow(parent),
      waveformChart(new QChart),
      resultsChart(new QChart),
      waveformSeries(new QLineSeries),
      thresholdSeries(new QLineSeries),
      peakSeries(new QScatterSeries),
      intervalSeries(new QScatterSeries)
{
    setupUI();
    QDir dir(".");
    wavFiles = dir.entryInfoList({"*.wav"}, QDir::Files, QDir::Name);
    if (!wavFiles.isEmpty()) {
        currentFileIndex = 0;
        loadWavFile(wavFiles[currentFileIndex].absoluteFilePath());
    }
}

void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    
    mainLayout->addWidget(createControlPanel(), 1);
    
    QVBoxLayout *plotsLayout = new QVBoxLayout();
    plotsLayout->addWidget(createWaveformPlot());
    plotsLayout->addWidget(createResultsPlot());
    mainLayout->addLayout(plotsLayout, 4);
    
    setCentralWidget(centralWidget);
    resize(1200, 800);
    setWindowTitle("JaDe Audio Processor");
    setStyleSheet("QMainWindow { background-color: #f0f0f0; }");
}

QWidget* MainWindow::createControlPanel() {
    QWidget *panel = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    panel->setStyleSheet("background-color: white; padding: 10px;");
    
    // Parameters Group
    QGroupBox *paramsGroup = new QGroupBox("Parameters");
    QVBoxLayout *paramsLayout = new QVBoxLayout(paramsGroup);
    
    threshInput = new QLineEdit("1.7");
    QHBoxLayout *threshLayout = new QHBoxLayout();
    threshLayout->addWidget(new QLabel("Threshold Multiplier:"));
    threshLayout->addWidget(threshInput);
    paramsLayout->addLayout(threshLayout);
    
    lengthInput = new QLineEdit("0.35");
    QHBoxLayout *lengthLayout = new QHBoxLayout();
    lengthLayout->addWidget(new QLabel("Minimum Length (s):"));
    lengthLayout->addWidget(lengthInput);
    paramsLayout->addLayout(lengthLayout);
    
    QPushButton *applyBtn = new QPushButton("Apply Parameters");
    connect(applyBtn, &QPushButton::clicked, this, &MainWindow::applyParameters);
    paramsLayout->addWidget(applyBtn);
    
    // Metadata Group
    QGroupBox *metaGroup = new QGroupBox("Metadata");
    QVBoxLayout *metaLayout = new QVBoxLayout(metaGroup);
    
    subjectInput = new QLineEdit("Subject");
    QHBoxLayout *subjectLayout = new QHBoxLayout();
    subjectLayout->addWidget(new QLabel("Subject Name:"));
    subjectLayout->addWidget(subjectInput);
    metaLayout->addLayout(subjectLayout);
    
    experimenterInput = new QLineEdit("Experimenter");
    QHBoxLayout *expLayout = new QHBoxLayout();
    expLayout->addWidget(new QLabel("Experimenter:"));
    expLayout->addWidget(experimenterInput);
    metaLayout->addLayout(expLayout);
    
    // Action Buttons
    QPushButton *saveBtn = new QPushButton("Save");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::saveResults);
    
    QPushButton *processAllBtn = new QPushButton("Process All Files");
    connect(processAllBtn, &QPushButton::clicked, this, &MainWindow::processAllFiles);
    
    // Navigation
    QHBoxLayout *navLayout = new QHBoxLayout();
    QPushButton *prevBtn = new QPushButton("Previous File");
    QPushButton *nextBtn = new QPushButton("Next File");
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::loadPreviousFile);
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::loadNextFile);
    navLayout->addWidget(prevBtn);
    navLayout->addWidget(nextBtn);
    
    statusLabel = new QLabel();
    statusLabel->setStyleSheet("color: #666;");
    
    // Assemble layout
    layout->addWidget(paramsGroup);
    layout->addWidget(metaGroup);
    layout->addWidget(saveBtn);
    layout->addWidget(processAllBtn);
    layout->addLayout(navLayout);
    layout->addWidget(statusLabel);
    layout->addStretch();
    
    return panel;
}

QWidget* MainWindow::createWaveformPlot() {
    QChartView *chartView = new QChartView(waveformChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    waveformChart->addSeries(waveformSeries);
    waveformChart->addSeries(thresholdSeries);
    waveformChart->addSeries(peakSeries);
    
    waveformChart->createDefaultAxes();
    waveformChart->axisX()->setTitleText("Time (s)");
    waveformChart->axisY()->setTitleText("Amplitude");
    waveformChart->legend()->hide();
    
    waveformSeries->setColor(QColor(0, 0, 255));
    thresholdSeries->setColor(QColor(255, 0, 0));
    peakSeries->setColor(QColor(255, 0, 0));
    peakSeries->setMarkerSize(8);
    
    return chartView;
}

QWidget* MainWindow::createResultsPlot() {
    QChartView *chartView = new QChartView(resultsChart);
    chartView->setRenderHint(QPainter::Antialiasing);
    
    resultsChart->addSeries(intervalSeries);
    resultsChart->createDefaultAxes();
    resultsChart->axisX()->setTitleText("Pulse Sequence Number");
    resultsChart->axisY()->setTitleText("Interval (ms)");
    resultsChart->legend()->hide();
    
    intervalSeries->setColor(QColor(0, 0, 255));
    intervalSeries->setMarkerSize(8);
    
    return chartView;
}

void MainWindow::loadWavFile(const QString& filePath) {
    try {
        audioData = AudioProcessor::processWavFile(
            filePath.toStdString(),
            threshInput->text().toDouble(),
            lengthInput->text().toDouble()
        );
        
        pulseTimes.clear();
        intervals.clear();
        
        for (auto peak : audioData.peaks) {
            pulseTimes.append(peak / audioData.sampleRate);
        }
        
        for (int i = 1; i < pulseTimes.size(); ++i) {
            intervals.append((pulseTimes[i] - pulseTimes[i-1]) * 1000);
        }
        
        updatePlots();
        statusLabel->setText("Loaded: " + QFileInfo(filePath).fileName());
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to load file: ") + e.what());
    }
}

void MainWindow::updatePlots() {
    // Update waveform plot
    waveformSeries->clear();
    thresholdSeries->clear();
    peakSeries->clear();
    
    QVector<QPointF> waveformPoints;
    for (size_t i = 0; i < audioData.samples.size(); ++i) {
        waveformPoints.append(QPointF(i / audioData.sampleRate, audioData.samples[i]));
    }
    waveformSeries->replace(waveformPoints);
    
    // Threshold line
    double xMax = audioData.samples.size() / audioData.sampleRate;
    thresholdSeries->append(QPointF(0, audioData.threshold));
    thresholdSeries->append(QPointF(xMax, audioData.threshold));
    
    // Peaks
    for (auto peak : audioData.peaks) {
        double time = peak / audioData.sampleRate;
        peakSeries->append(time, audioData.samples[peak]);
    }
    
    // Update results plot
    intervalSeries->clear();
    QVector<QPointF> intervalPoints;
    for (int i = 0; i < intervals.size(); ++i) {
        intervalPoints.append(QPointF(i, intervals[i]));
    }
    intervalSeries->replace(intervalPoints);
    
    // Adjust axes
    waveformChart->axisX()->setRange(0, audioData.samples.size() / audioData.sampleRate);
    waveformChart->axisY()->setRange(
        *std::min_element(audioData.samples.begin(), audioData.samples.end()),
        *std::max_element(audioData.samples.begin(), audioData.samples.end())
    );
    
    resultsChart->axisX()->setRange(0, intervals.size());
    resultsChart->axisY()->setRange(
        *std::min_element(intervals.begin(), intervals.end()) * 0.9,
        *std::max_element(intervals.begin(), intervals.end()) * 1.1
    );
}

void MainWindow::loadNextFile() {
    if (!wavFiles.isEmpty() && currentFileIndex < wavFiles.size() - 1) {
        currentFileIndex++;
        loadWavFile(wavFiles[currentFileIndex].absoluteFilePath());
    }
}

void MainWindow::loadPreviousFile() {
    if (!wavFiles.isEmpty() && currentFileIndex > 0) {
        currentFileIndex--;
        loadWavFile(wavFiles[currentFileIndex].absoluteFilePath());
    }
}

void MainWindow::applyParameters() {
    if (!wavFiles.isEmpty()) {
        loadWavFile(wavFiles[currentFileIndex].absoluteFilePath());
    }
}

void MainWindow::saveResults() {
    if (wavFiles.isEmpty() || intervals.isEmpty()) return;
    
    QFileInfo fileInfo(wavFiles[currentFileIndex]);
    QString baseName = fileInfo.baseName();
    
    // Save DAT file
    QFile datFile(baseName + ".dat");
    if (datFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&datFile);
        out << subjectInput->text() << "\n";
        out << "\t" << experimenterInput->text() << "/" << baseName 
            << " recorded with JaDe_V2 " 
            << QDateTime::currentDateTime().toString("dd/MM/yyyy   HH:mm") << "\n\n";
            
        for (double interval : intervals) {
            out << qRound(interval) << "\n";
        }
        datFile.close();
    }
    
    // Save JPG
    QPixmap pixmap = resultsChart->scene()->views().first()->grab();
    pixmap.save(baseName + ".jpg", "JPG");
    
    statusLabel->setText("Saved " + baseName + ".dat and .jpg");
}

void MainWindow::processAllFiles() {
    if (wavFiles.isEmpty()) return;
    
    int originalIndex = currentFileIndex;
    for (int i = 0; i < wavFiles.size(); ++i) {
        currentFileIndex = i;
        loadWavFile(wavFiles[i].absoluteFilePath());
        saveResults();
        QApplication::processEvents();
    }
    currentFileIndex = originalIndex;
    loadWavFile(wavFiles[originalIndex].absoluteFilePath());
    statusLabel->setText("Finished processing all files");
}