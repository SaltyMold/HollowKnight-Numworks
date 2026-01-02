// eadk
#include "libs/eadk/eadkpp.h"
#include "libs/eadk/eadk_vars.h"
// storage
#include "libs/storage/storage.h"
// lz4
#include "libs/lz4/lz4.h"
//color
#include "color.h"

using namespace EADK;

int main(void) {
    Display::pushRectUniform(Screen::Rect, color_white); 
    Display::drawString("Press Back to Exit", Point(0, 0), false, color_black, color_white);

    Keyboard::State state;
    while (1) {
        state = Keyboard::scan();
        if (state.keyDown(Keyboard::Key::Back)) {
            break;
        }
    }

    return 0;
}