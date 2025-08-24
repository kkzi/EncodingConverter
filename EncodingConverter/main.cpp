#include "MainWindow.h"
#include <QApplication>
#include <QScreen>

int main(int argc, char *argv[])
{
    // Enable high DPI support
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    
    // For Qt 5.14+, scaling factor can be set
    #if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
    #endif
    
    QApplication a(argc, argv);
    
    // Set application style to ensure proper display in high DPI
    a.setStyle("Fusion");
    
    MainWindow w;
    w.show();
    
    // Optional: adjust window size on high DPI screens
    QScreen *screen = w.screen();
    if (screen) {
        qreal dpi = screen->logicalDotsPerInch();
        if (dpi > 120) {  // High DPI screen
            // Can adjust window size as needed
            // w.resize(w.width() * dpi / 96, w.height() * dpi / 96);
        }
    }
    
    return a.exec();
}
