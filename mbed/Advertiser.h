#ifndef ADVERTISER
#define ADVERTISER

#include "mbed.h"
#include "ble/BLE.h"

typedef struct {
    uint8_t data;
    uint8_t id[3];
    uint8_t sequenceNumber;
    uint8_t ttl;
} service_data_t;

class Advertiser 
{
    public:
        Advertiser(const char* deviceName);
        void start(float time);
        void stop();
        
        void setDeviceName(const char* deviceName);
        void setButtonState(bool pressed);
        void setBatteryPercentage(uint8_t percentage);
        void setDeviceID(uint8_t* id, uint8_t size);
        void setSequenceNumber(uint8_t seqNum);
        void setTTL(uint8_t ttl);
        
        
        bool getButtonState() const;
        uint8_t getBatteryPercentage() const;
        uint8_t* getDeviceID() const;
        uint8_t getDeviceIDSize() const;
        uint8_t getSequenceNumber() const;
        uint8_t getTTL() const;
        
        service_data_t& getServiceData();
        
    protected:
        void setupAdvertisingPayload();
        
    private:
        const char* deviceName;
        service_data_t serviceData;
};

#endif