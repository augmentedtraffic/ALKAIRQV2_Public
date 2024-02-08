#ifndef sensorData_h
#define sensorData_h
#define NUM_SENSOR_READINGS 200
class sensorData {
    public:
        sensorData(boolean haveGuardTime, long sensorDurationBetweenSamples);
        int numValues();
        void addData(int sensorValue);
        int getData(int idx);
        int getLatest();

    private:
        int _numSensorValues = 0;
        int _idxSensor = 0;
        boolean _haveGuardTime=false;
        uint16_t _sensorSaved[NUM_SENSOR_READINGS];
        long _lastSensorTime = 0;
        long _sensorDurationBetweenSamples = 0;
};
#endif