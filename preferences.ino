void bootUpPreferences() {
  preferences.begin("augmented", false);
  bootupCount = preferences.getInt("bootcount", 0);
  if (bootupCount == 0) {
    needToReadSEN55Data = true;
  } else {
    needToReadSEN55Data = false;
  }
  bootupCount++;
  if (bootupCount>=MAX_BOOTUP_COUNT) bootupCount = 0;
  preferences.putInt("bootcount", bootupCount);
  debugMessage("Bootup Count "+String(bootupCount));
  preferences.end();
}

void saveSensorReadings(int voc, int nox) {
  preferences.putInt("voc", voc);
  preferences.putInt("nox", nox);
}

void readSensorReadings() {
  voc = preferences.getInt("voc", 0);
  nox = preferences.getInt("nox", 0);
}
