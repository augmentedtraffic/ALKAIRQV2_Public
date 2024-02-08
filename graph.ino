uint16_t tmpSaved[NUM_SENSOR_READINGS];
void drawGraph(int x_pos, int y_pos, int gwidth, int gheight, float Y1Min, float Y1Max, String title, sensorData sensorInfo, boolean auto_scale, boolean barchart_mode, boolean graphOnly) {
#define auto_scale_margin 0 // Sets the autoscale increment, so axis steps up in units of e.g. 3
#define y_minor_axis 4      // 5 y-axis division markers
  int maxYscale = -10000;
  int minYscale =  10000;
  int last_x, last_y;
  float x2, y2;
  int readings = sensorInfo.numValues();
  for (int i=0; i<readings; ++i) {
    tmpSaved[i] = sensorInfo.getData(i);
  }
  for (int i=readings; i<NUM_SENSOR_READINGS; ++ i) {
    tmpSaved[i]=0;
  }
  if (auto_scale == true) {
    for (int i = 1; i < readings; i++ ) { // Adjusted graph range
      if (tmpSaved[i] >= maxYscale) maxYscale = tmpSaved[i];
      if (tmpSaved[i] <= minYscale) minYscale = tmpSaved[i];
    }
    maxYscale = round(maxYscale + auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Max
    Y1Max = round(maxYscale + 0.5);
    if (minYscale != 0) minYscale = round(minYscale - auto_scale_margin); // Auto scale the graph and round to the nearest value defined, default was Y1Min
    Y1Min = round(minYscale);
  }
  //DISP.fillRect(0,y_pos, WIDTH, 103, 0); 
  canvasGraph.fillSprite(TFT_WHITE);
  // Draw the graph
  last_x = x_pos + 1;
  last_y = y_pos + (Y1Max - constrain(tmpSaved[1], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight;
  canvasGraph.drawRect(x_pos, y_pos, gwidth + 3, gheight + 2, TFT_BLACK);
  canvasGraph.setTextSize(2);
  canvasGraph.setTextColor(TFT_BLACK);
  canvasGraph.drawString(title, x_pos + gwidth / 2-50, y_pos - 20);
  // Draw the data
  for (int gx = 1; gx < readings; gx++) {
    x2 = x_pos + gx * gwidth / (readings - 1) - 1 ; // max_readings is the global variable that sets the maximum data that can be plotted
    y2 = y_pos + (Y1Max - constrain(tmpSaved[gx], Y1Min, Y1Max)) / (Y1Max - Y1Min) * gheight + 1;
    if (barchart_mode) {
      canvasGraph.fillRect(x2, y2, (gwidth / readings) - 1, y_pos + gheight - y2 + 2, TFT_BLACK);
    } else {
      canvasGraph.drawLine(last_x, last_y, x2, y2, TFT_BLACK);
    }
    last_x = x2;
    last_y = y2;
  }
  //Draw the Y-axis scale
#define number_of_dashes 20
   canvasGraph.setTextSize(1);
  //DISP.setTextColor(TFT_WHITE);
  for (int spacing = 0; spacing <= y_minor_axis; spacing++) {
    for (int j = 0; j < number_of_dashes; j++) { // Draw dashed graph grid lines
      if (spacing < y_minor_axis) canvasGraph.fillRect((x_pos + 3 + j * gwidth / number_of_dashes), y_pos + (gheight * spacing / y_minor_axis), gwidth / (2 * number_of_dashes),2, TFT_BLACK);
    }
    if (!graphOnly) {
      if (Y1Min < 1 && Y1Max < 10)
        canvasGraph.drawString(String((int) (Y1Max - (int)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01)), x_pos - 20, y_pos + gheight * spacing / y_minor_axis - 1);
      else
        canvasGraph.drawString(String((int) (Y1Max - (int)(Y1Max - Y1Min) / y_minor_axis * spacing + 0.01)), x_pos - 20, y_pos + gheight * spacing / y_minor_axis - 1);
    }
  }
  canvasGraph.setTextSize(2);
}
