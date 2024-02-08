int WIDTH = 200;
#define DISPLAY_MODE_NORMAL 1
#define DISPLAY_MODE_GRAPH 2
int mode = DISPLAY_MODE_NORMAL;
boolean isNormalMode() { return mode == DISPLAY_MODE_NORMAL; }
boolean isGraphMode() { return mode == DISPLAY_MODE_GRAPH; }

void setNormalMode() {
      mode = DISPLAY_MODE_NORMAL;
      showHeader();
      showWifiStatus(connectedInternet);
      updateInfo();
      updateRunMode(donotsleep?"Continuous":"Auto Sleep");
}
void showHeader() {
    canvas.fillSprite(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextSize(2);
    String textHeader1 = "Augmented";
    String textHeader2 = "Presence";
    int x = (WIDTH - display.textWidth(textHeader1)) / 2-15;
    canvas.drawString(textHeader1, x, 0);
    canvas.drawString(textHeader2, x+3, 18);
    canvas.pushSprite(0, 0);
}
void displayInit() {

    display.begin();

    if (display.isEPD()) {
      display.setEpdMode(epd_mode_t::epd_fastest);
  //    display.invertDisplay(true);
      display.clear(TFT_WHITE);
    }
    if (display.width() < display.height())
    {
      display.setRotation(display.getRotation() ^ 1);
    }
    canvasGraph.createSprite(display.width(), display.height());
    canvasGraph.setColorDepth(1); // mono color
    canvas.setColorDepth(1); // mono color
    canvas.createSprite(display.width(), display.height());
    showHeader();
    updateRunMode(donotsleep?"Continuous":"Auto Sleep");
}

float updatePower(uint32_t voltage) {
    static uint32_t lastVoltage = 0;
    static float v = 0;
    if (lastVoltage == voltage) {
        return v;
    }
    char str[13] = { 0 };
    v = (((float)voltage / 1000) * 2);
    v = v > 4.2 ? 4.2 : v;
    sprintf(str, "BAT:%.2fV", v);
    lastVoltage = voltage;
    return v;
}
void showMessage(String message) {
  if (mode != DISPLAY_MODE_NORMAL) return;
  canvas.fillSprite(TFT_BLACK);
  canvas.setTextColor(TFT_WHITE);
  canvas.drawString(message, 0, 0);
  canvas.pushSprite(0, 100);
}
void updateRunMode(String msg, boolean invertMode) {
  if (mode != DISPLAY_MODE_NORMAL) return;
  if (invertMode) {
      canvas.fillSprite(TFT_BLACK);
      canvas.setTextColor(TFT_WHITE);
      canvas.drawString(msg, 0, 0);
  } else {
      canvas.fillSprite(TFT_WHITE);
      canvas.setTextColor(TFT_BLACK);
      canvas.drawString(msg, 0, 0);
  }
  canvas.pushSprite(0, 184);
}
void showWifiStatus(boolean connectedInternet) {
  if (mode != DISPLAY_MODE_NORMAL) return;
  String msg = "None";
  String tmpS = connectedSSID;
  if (connectedInternet) {
      msg = "" + tmpS.substring(0,(tmpS.length()>15)?15:tmpS.length());
      canvas.fillSprite(TFT_BLACK);
      canvas.setTextColor(TFT_WHITE);
  } else {
      canvas.fillSprite(TFT_WHITE);
      canvas.setTextColor(TFT_BLACK);
  }
  canvas.drawString(msg, 0, 0);
  canvas.pushSprite(0, 38);
}
const char *bootupIndicator[MAX_BOOTUP_COUNT] = { "****", " ", "*", "**", "***" };

void updateInfo() {
  if (mode != DISPLAY_MODE_NORMAL) return;
  static int count = 0;
  count++;
  canvas.fillSprite(TFT_WHITE);
  canvas.setTextColor(TFT_BLACK);
  int startX = 20;
  int startY = 2;
  int yINC = 16;
  canvas.drawString("  CO2:" + String(sensor.scd40.co2),                 startX, startY);
  canvas.drawString(" Temp:" + String(sensor.scd40.temperature),        startX, startY + yINC);
  canvas.drawString("ATemp:" + String(sensor.sen55.ambientTemperature), startX, startY + yINC*2);
  canvas.drawString("   RH:" + String(sensor.sen55.ambientHumidity),    startX, startY + yINC*3);
  canvas.drawString("PM2.5: " + String(sensor.sen55.massConcentrationPm2p5), startX, startY + yINC*4);
  canvas.drawString("  VOC: " + String(voc) + (needToReadSEN55Data||donotsleep?" ":bootupIndicator[bootupCount]),  startX, startY+ yINC*5);
  canvas.drawString("  NOX: " + String(nox) + (needToReadSEN55Data||donotsleep?" ":bootupIndicator[bootupCount]),  startX, startY+ yINC*6);
  canvas.drawString(" Batt: " + String(updatePower(sensor.battery.raw)),          startX, startY+ yINC*7);
  //canvas.drawString("Count: " + String(count),                          startX, startY+ yINC*8);
  canvas.pushSprite(0, 54);
}

#define GRAPH_CO2 1
#define GRAPH_VOC 2
#define GRAPH_PPM 3
#define GRAPH_TEMP 4

int whichGraph = GRAPH_CO2;

void showFirstGraph() {
    mode = DISPLAY_MODE_GRAPH;
    whichGraph = GRAPH_CO2;
    drawGraph(20, 20, display.width()-40,  display.width()-40,  300, 1600,"    CO2", co2Saved, false, false, false);
    canvasGraph.pushSprite(0, 0);
}
void showNextGraph() {
    if (whichGraph == GRAPH_CO2) {
         whichGraph = GRAPH_VOC;
        drawGraph(20, 20, display.width()-40,  display.width()-40,  0, 200,"     VOC", vocSaved, false, false, false);
    } else  if (whichGraph == GRAPH_VOC) {
        whichGraph = GRAPH_PPM;
        drawGraph(20, 20, display.width()-40,  display.width()-40,  300, 1600,"    PPM", ppmSaved, false, false, false);
    } else  if (whichGraph == GRAPH_PPM) {
        whichGraph = GRAPH_TEMP;
        drawGraph(20, 20, display.width()-40,  display.width()-40,  -20, 35,"    Temp", tempSaved, false, false, false);
    } else  if (whichGraph == GRAPH_TEMP) {
        whichGraph = GRAPH_CO2;
        drawGraph(20, 20, display.width()-40,  display.width()-40,  300, 1600,"    CO2", co2Saved, false, false, false);
    }
    canvasGraph.pushSprite(0, 0);
}