#include "Advertiser.h"
#define DEVICE_ID_SIZE 3

Advertiser::Advertiser(const char* deviceName)
{
    setDeviceName(deviceName);
    serviceData.sequenceNumber = 0;
    serviceData.ttl = 0xFF;
}

void Advertiser::start(float time)
{
    //Setup advertising payload and start
    setupAdvertisingPayload();
    BLE& ble = BLE::Instance();
    ble.gap().setAdvertisingTimeout(time);
    ble.gap().startAdvertising();
}

void Advertiser::stop()
{
    BLE &ble = BLE::Instance();
    ble.gap().stopAdvertising();
}
    
void Advertiser::setupAdvertisingPayload()
{
    BLE &ble = BLE::Instance();
    ble.gap().setAdvertisingInterval(1000); //1000ms
    ble.gap().clearAdvertisingPayload();
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)deviceName, sizeof(deviceName));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SERVICE_DATA, (uint8_t*)&serviceData, sizeof(serviceData));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);
}

void Advertiser::setDeviceName(const char* deviceName)
{
    this->deviceName = deviceName;
}

void Advertiser::setButtonState(bool pressed)
{
    serviceData.data &= ~(1 << 7); //Clear highest order bit
    if(pressed)
    {
        serviceData.data |= (1 << 7);
    }
}

void Advertiser::setBatteryPercentage(uint8_t percentage)
{
    bool pressed = getButtonState();
    serviceData.data = percentage;
    setButtonState(pressed);
}

void Advertiser::setDeviceID(uint8_t* id, uint8_t size)
{
    for(unsigned int i = 0; i < size; i++)
    {
        serviceData.id[i] = id[i];
    }
}

void Advertiser::setSequenceNumber(uint8_t seqNum)
{
    serviceData.sequenceNumber = seqNum;
}

void Advertiser::setTTL(uint8_t ttl)
{
    serviceData.ttl = ttl;
}

bool Advertiser::getButtonState() const
{
    return (serviceData.data & (1 << 7)) != 0;   
}

uint8_t Advertiser::getBatteryPercentage() const
{
    return serviceData.data & ~(1 << 7);
}

uint8_t* Advertiser::getDeviceID() const
{
    uint8_t* copy = new uint8_t[DEVICE_ID_SIZE]();
    for(unsigned int i = 0; i < DEVICE_ID_SIZE; i++)
    {
        copy[i] = serviceData.id[i];
    }
    return copy;
}

uint8_t Advertiser::getDeviceIDSize() const
{
    return DEVICE_ID_SIZE;
}

uint8_t Advertiser::getSequenceNumber() const
{
    return serviceData.sequenceNumber;
}

uint8_t Advertiser::getTTL() const
{
    return serviceData.ttl;
}

service_data_t& Advertiser::getServiceData()
{
    return serviceData;
}