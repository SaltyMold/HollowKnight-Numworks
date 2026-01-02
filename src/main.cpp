// eadk
#include "libs/eadk/eadkpp.h"
#include "libs/eadk/eadk_vars.h"
// storage
#include "libs/storage/storage.h"
// lz4
#include "libs/lz4/lz4.h"
//color
#include "color.h"
//image 
#include "display_image.h"
#include "../output/assets/icon.h"

using namespace EADK;

int main(void) {
    Display::pushRectUniform(Screen::Rect, color_white); 
    Display::drawString("Press Back to Exit", Point(0, 0), false, color_black, color_white);

    DRAW_IMAGE_AT(EADK::Point(50, 50), Image::Icon);
    
    Keyboard::State state;
    while (1) {
        state = Keyboard::scan();
        if (state.keyDown(Keyboard::Key::Back)) {
            break;
        }
    }

    return 0;
}