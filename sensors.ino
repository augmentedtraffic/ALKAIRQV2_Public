void powerDownSensors() {
    scd4x.powerDown();
    digitalWrite(SEN55_POWER_EN, HIGH);
}
void initSensors(boolean initSEN55) {

      if (initSEN55) {
          digitalWrite(SEN55_POWER_EN, LOW);
      }

      log_i("SCD40 sensor init");
      char errorMessage[256];
      scd4x.begin(Wire);
      /** stop potentially previously started measurement */
      uint16_t error = scd4x.stopPeriodicMeasurement();
      if (error) {
        errorToString(error, errorMessage, 256);
        log_w("Error trying to execute stopPeriodicMeasurement(): %s", errorMessage);
      }
      /** Start Measurement */
      error = scd4x.startPeriodicMeasurement();
      if (error) {
        errorToString(error, errorMessage, 256);
        log_w("Error trying to execute startPeriodicMeasurement(): %s", errorMessage);
      }
      log_i("Waiting for first measurement... (5 sec)");
      
      /** Init SEN55 */
      if (initSEN55) {
          log_i("SEN55 sensor init");
          sen5x.begin(Wire);
          error = sen5x.deviceReset();
          if (error) {
            errorToString(error, errorMessage, 256);
            log_w("Error trying to execute deviceReset(): %s", errorMessage);
          }
          float tempOffset = 0.0;
          error = sen5x.setTemperatureOffsetSimple(tempOffset);
          if (error) {
            errorToString(error, errorMessage, 256);
            log_w("Error trying to execute setTemperatureOffsetSimple(): %s", errorMessage);
          } else {
            log_i("Temperature Offset set to %f deg. Celsius (SEN54/SEN55 only)", tempOffset);
          }
          /** Start Measurement */
          error = sen5x.startMeasurement();
          if (error) {
            errorToString(error, errorMessage, 256);
            log_w("Error trying to execute startMeasurement(): %s", errorMessage);
          }
      }

      /** fixme: 超时处理 */
      bool isDataReady = false;
      do {
        error = scd4x.getDataReadyFlag(isDataReady);
        if (error) {
          errorToString(error, errorMessage, 256);
          log_w("Error trying to execute getDataReadyFlag(): %s", errorMessage);
          return ;
        }
      } while (!isDataReady);

}