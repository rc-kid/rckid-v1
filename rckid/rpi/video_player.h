#pragma once

#include "widget.h"
#include "gui.h"

/** A video player, frontend to cvlc controlled the same way as retroarch is. 
 
    However retaking the framebuffer by reinitializing the window does not seem to work. But for the video playback, it is arguably not necessary as there is no in-game menu. 

    Maybe use omxplayer which can be controlled via dbus too? 
 */
class VideoPlayer : public Widget {

}; // Video