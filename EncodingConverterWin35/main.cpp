#include "MainWindow.h"
#include <vaca/vaca.h>

using namespace vaca;

int VACA_MAIN()
{
    Application app;
    MainWindow mainWindow;
    mainWindow.setIcon(ResourceId(1));  // IDI_APP_ICON
    mainWindow.setVisible(true);
    app.run();
    return 0;
}