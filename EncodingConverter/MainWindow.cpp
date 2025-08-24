#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../common/FileConverter.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>
#include <QDirIterator>
#include <QApplication>
#include <QLabel>
#include <QSet>
#include <QDebug>

// Helper to split a QString
std::vector<std::string> qstringToVector(const QString& str) {
    std::vector<std::string> vec;
    for (const QString& part : str.split(",", Qt::SkipEmptyParts)) {
        vec.push_back(part.trimmed().toStdString());
    }
    return vec;
}

// A worker thread to run the conversion process without blocking the UI
class ConverterWorker : public QObject {
    Q_OBJECT

public:
    ConverterWorker(const std::vector<std::string>& dirs, 
                    const std::vector<std::string>& exts, 
                    const std::string& targetEnc,
                    bool backup,
                    const std::string& backupSuffix)
        : m_dirs(dirs), m_exts(exts), m_targetEnc(targetEnc), 
          m_backup(backup), m_backupSuffix(backupSuffix) {}

public slots:
    void doWork() {
        emit logMessage("Starting conversion...");

        int totalFiles = 0;
        QSet<QString> fileList;

        for (const auto& dir : m_dirs) {
            QDirIterator it(QString::fromStdString(dir), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString filePath = it.next();
                if (!filePath.isEmpty()) {
                    bool extensionMatch = false;
                    for (const auto& ext : m_exts) {
                        if (filePath.endsWith(QString::fromStdString(ext))) {
                            extensionMatch = true;
                            break;
                        }
                    }
                    if (extensionMatch) {
                        fileList.insert(filePath);
                        totalFiles++;
                    }
                }
            }
        }
        
        emit logMessage(QString("Found %1 files to process.").arg(totalFiles));
        if (totalFiles == 0) {
            emit conversionFinished();
            return;
        }

        int processedCount = 0;
        for (const auto& file : fileList) {
            try {
                // Use FileConverter to convert file using the new API
                std::string filePath = file.toStdString();
                FileConverter::ConversionResult result = FileConverter::convertFile(
                    filePath,
                    m_targetEnc,
                    m_backup,
                    m_backupSuffix);
                
                // Handle the result
                switch (result)
                {
                    case FileConverter::ConversionResult::Success:
                    {
                        // Detect source encoding for logging
                        std::string sourceEncoding = FileConverter::detectFileEncoding(filePath);
                        emit logMessage(QString("Successfully converted: '%1' (%2 -> %3)")
                                      .arg(file)
                                      .arg(QString::fromStdString(sourceEncoding))
                                      .arg(QString::fromStdString(m_targetEnc)));
                        break;
                    }
                    case FileConverter::ConversionResult::EmptyFile:
                        emit logMessage(QString("Skipping file '%1': File is empty").arg(file));
                        break;
                    case FileConverter::ConversionResult::AlreadyTargetEncoding:
                        emit logMessage(QString("Skipping file '%1': Already in target encoding").arg(file));
                        break;
                    case FileConverter::ConversionResult::CannotDetectEncoding:
                        emit logMessage(QString("Skipping file '%1': Cannot detect encoding").arg(file));
                        break;
                    case FileConverter::ConversionResult::BackupFailed:
                        emit logMessage(QString("Backup failed: '%1'").arg(file));
                        break;
                    case FileConverter::ConversionResult::ConversionFailed:
                        emit logMessage(QString("Conversion failed: '%1'").arg(file));
                        break;
                }

            } catch (const std::exception& e) {
                emit logMessage(QString("Error processing '%1': %2").arg(file).arg(e.what()));
            }

            processedCount++;
            emit conversionProgress(static_cast<int>((static_cast<float>(processedCount) / totalFiles) * 100));
        }

        emit logMessage("Conversion completed.");
        emit conversionFinished();
    }

signals:
    void conversionProgress(int progress);
    void logMessage(const QString &message);
    void conversionFinished();

private:
    std::vector<std::string> m_dirs;
    std::vector<std::string> m_exts;
    std::string m_targetEnc;
    bool m_backup;
    std::string m_backupSuffix;
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Connect signals and slots
    connect(this, &MainWindow::conversionProgress, ui->progressBar, &QProgressBar::setValue);
    connect(this, &MainWindow::logMessage, ui->logTextEdit, &QTextEdit::append);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_selectDirectoryButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory", QDir::currentPath());
    if (!dir.isEmpty()) {
        QString currentText = ui->directoryLineEdit->text();
        if (!currentText.isEmpty()) {
            currentText += ",";
        }
        currentText += dir;
        ui->directoryLineEdit->setText(currentText);
    }
}

void MainWindow::on_convertButton_clicked()
{
    std::vector<std::string> dirs = qstringToVector(ui->directoryLineEdit->text());
    std::vector<std::string> exts = qstringToVector(ui->extensionsLineEdit->text());
    std::string targetEnc = ui->targetEncodingComboBox->currentText().toStdString();
    bool backup = ui->backupCheckBox->isChecked();
    std::string backupSuffix = ui->backupSuffixLineEdit->text().toStdString();

    if (dirs.empty() || exts.empty()) {
        QMessageBox::warning(this, "Warning", "Please fill in directory and file extensions.");
        return;
    }

    // Disable UI to prevent duplicate operations
    ui->convertButton->setEnabled(false);
    ui->selectDirectoryButton->setEnabled(false);
    ui->logTextEdit->clear();
    ui->progressBar->setValue(0);
    
    // Create worker thread
    QThread* thread = new QThread();
    ConverterWorker* worker = new ConverterWorker(dirs, exts, targetEnc, backup, backupSuffix);
    worker->moveToThread(thread);

    connect(worker, &ConverterWorker::logMessage, this, &MainWindow::logMessage);
    connect(worker, &ConverterWorker::conversionProgress, this, &MainWindow::conversionProgress);

    connect(thread, &QThread::started, worker, &ConverterWorker::doWork);
    connect(worker, &ConverterWorker::conversionFinished, thread, &QThread::quit);
    connect(worker, &ConverterWorker::conversionFinished, this, [=]() {
        ui->convertButton->setEnabled(true);
        ui->selectDirectoryButton->setEnabled(true);
        ui->progressBar->setValue(100);
        thread->deleteLater();
        worker->deleteLater();
    });

    thread->start();
}

void MainWindow::on_quickExtButton1_clicked()
{
    ui->extensionsLineEdit->setText(".h,.cpp,.c,.hpp,.cc,.cxx");
}

void MainWindow::on_quickExtButton2_clicked()
{
    ui->extensionsLineEdit->setText(".txt,.log,.md,.ini,.conf");
}

void MainWindow::on_quickExtButton3_clicked()
{
    ui->extensionsLineEdit->setText(".java,.kt,.scala");
}

void MainWindow::on_quickExtButton4_clicked()
{
    ui->extensionsLineEdit->setText(".py,.pyx,.pyw");
}

// MOC files are automatically generated by CMAKE_AUTOMOC
#include "MainWindow.moc"
