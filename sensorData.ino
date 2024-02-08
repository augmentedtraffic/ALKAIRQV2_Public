  #include "sensorData.h"
  sensorData::sensorData(boolean haveGuardTime, long sensorDurationBetweenSamples) {  
      _numSensorValues = 0;
      _idxSensor = 0;
      _haveGuardTime = haveGuardTime;
      _sensorDurationBetweenSamples = sensorDurationBetweenSamples;
      _lastSensorTime = -_sensorDurationBetweenSamples; // to trigger update
  }
  int sensorData::numValues() {
    return _numSensorValues;
  }
  int sensorData::getLatest() {
    return sensorData::getData(_numSensorValues-1);
  }

  void sensorData::addData(int sensorValue) {
    if (!_haveGuardTime || (_lastSensorTime+(_sensorDurationBetweenSamples))< millis()) {
        _lastSensorTime = millis();
        _sensorSaved[_idxSensor] = sensorValue;
        _numSensorValues++; 
        _idxSensor++;
        if (_numSensorValues >= NUM_SENSOR_READINGS) _numSensorValues = NUM_SENSOR_READINGS;
        if (_idxSensor>= NUM_SENSOR_READINGS) _idxSensor = 0;
    }
  }

  // idx 0 - oldest, 1 - 2nd oldest, etc
  int sensorData::getData(int idx) {
      if (idx >=0 && idx <_numSensorValues) {
            int _tmp = idx;  // default 0 is oldest, 1, 2nd oldest etc
            if (_numSensorValues >= NUM_SENSOR_READINGS) {
                _tmp = _idxSensor + idx;
                if (_tmp >= _numSensorValues) _tmp = _tmp - _numSensorValues;
            }
            return _sensorSaved[_tmp];
      }
    return -1;
  }