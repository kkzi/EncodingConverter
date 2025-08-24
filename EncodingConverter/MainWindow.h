#pragma once

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Slot function: handle select directory button click
    void on_selectDirectoryButton_clicked();
    // Slot function: handle convert button click
    void on_convertButton_clicked();
    // Slot function: quick select file extensions
    void on_quickExtButton1_clicked();
    void on_quickExtButton2_clicked();
    void on_quickExtButton3_clicked();
    void on_quickExtButton4_clicked();

signals:
    // Signal: notify conversion progress
    void conversionProgress(int progress);
    // Signal: notify log message
    void logMessage(const QString &message);

private:
    Ui::MainWindow *ui;
    // Helper function: perform conversion operation
    void performConversion();
};
