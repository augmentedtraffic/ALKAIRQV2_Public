void checkButton(ButtonEvent_t *buttonEvent) {
    if (buttonEvent->id == E_BUTTON_A && buttonEvent->type == E_BUTTON_CLICK_TYPE_SINGLE) {
        debugMessage("Button A clicked once");
        if (isNormalMode()) {
            showFirstGraph();
            donotsleep = true;
        } else if (isGraphMode()) {
            showNextGraph();
        }
    } else if  (buttonEvent->id == E_BUTTON_B && buttonEvent->type == E_BUTTON_CLICK_TYPE_SINGLE) {
        debugMessage("Button B clicked once");
        if (isNormalMode()) {
            donotsleep=!donotsleep;
        } else if (isGraphMode()) {
            setNormalMode();
        }
        updateRunMode(donotsleep?"Continuous":"Auto Sleep");
    }
    buttonEvent->id = E_BUTTON_NONE;
    buttonEvent->type = E_BUTTON_CLICK_TYPE_NONE;
}