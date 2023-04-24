#ifndef UI_STATE_H
#define UI_STATE_H

enum UserInterfaceState
{
    UI_STATE_DEFAULT,
    UI_STATE_CURSOR_MOVING,
    UI_STATE_VIEW_RESIZING,
    UI_STATE_KEYBOARD_SAMPLE_DRAG,
    UI_STATE_MOUSE_DRAG_SAMPLE_START,
    UI_STATE_MOUSE_DRAG_SAMPLE_LENGTH,
    UI_STATE_MOUSE_DRAG_MONO_LOWPASS,
    UI_STATE_MOUSE_DRAG_MONO_HIGHPASS,
};

#endif // UI_STATE_H