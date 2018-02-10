
extern "C" {
#include "user_interface.h"
}

#include "sigksens.h"
#include "systemHz.h"

SystemHzSensorInfo::SystemHzSensorInfo(String addr) {
  strcpy(address, addr.c_str());
  signalKPath[0] = "";
  signalKPath[1] = "";
  attrName[0] = "systemHz";
  attrName[1] = "freeMem";
  type = SensorType::local;
  valueJson[0] = "null";
  valueJson[1] = "null";

  isUpdated = false;
}

SystemHzSensorInfo::SystemHzSensorInfo(String addr, 
                                         String path1, 
                                         String path2) {
  strcpy(address, addr.c_str());
  signalKPath[0] = path1;
  signalKPath[1] = path2;
  attrName[0] = "systemHz";
  attrName[1] = "freeMem";
  type = SensorType::local;
  valueJson[0] = "null";
  valueJson[1] = "null";

  isUpdated = false;
}

SystemHzSensorInfo *SystemHzSensorInfo::fromJson(JsonObject &jsonSens) {
  return new SystemHzSensorInfo(
    jsonSens["address"],
    jsonSens["signalKPaths"][0],
    jsonSens["signalKPaths"][1]
  );
}

void SystemHzSensorInfo::toJson(JsonObject &jsonSens) {
  jsonSens["address"] = address;
  jsonSens["type"] = (int)SensorType::local;
  JsonArray& jsonPaths = jsonSens.createNestedArray("signalKPaths");
  for (int x=0 ; x < MAX_SENSOR_ATTRIBUTES ; x++) {
    if (strcmp(attrName[x].c_str(), "") == 0 ) {
      break; //no more attributes
    }
    jsonPaths.add(signalKPath[x]);
  }
}


os_timer_t  updateHz; // once request cycle starts, this timer set so we can send when ready
bool readyToUpdateHz = false;

uint32_t systemHzCount = 0, systemHzMs = 0;

float systemHz = 0;


void interruptSystemHz(void *pArg) {
  readyToUpdateHz = true;
}


void setupSystemHz(bool &need_save) {
  os_timer_setfn(&updateHz, interruptSystemHz, NULL);
  os_timer_arm(&updateHz, 1000, true);
  systemHzMs = millis();
  SensorInfo *tmpSensorInfo;

  // Setup "sensor" if not already existing
  bool known = sensorStorage[(int)SensorType::local].find(
    "Local") != nullptr;

  if (!known) {
    Serial.print("Setting up System info ");
    SensorInfo *newSensor = new SystemHzSensorInfo("Local");
    sensorStorage[(int)newSensor->type].add(newSensor);
    need_save = true;
  }    
}


void updateSystemHz() {
  SensorInfo *thisSensorInfo;
  uint32_t elapsed = millis() - systemHzMs;
  
  if (elapsed == 0) { return; } // getting sporadic devide by 0 exceptions, no harm in skipping a loop.
  
  systemHz = (systemHzCount*1000) / elapsed;
 // Serial.print ("System Hz :");
 // Serial.println (systemHz);

  for (uint8_t i=0; i < sensorStorage[(int)SensorType::local].size(); i++) {
    thisSensorInfo = sensorStorage[(int)SensorType::local].get(i);
    if (thisSensorInfo->type==SensorType::local) {
      if (strcmp(thisSensorInfo->signalKPath[0].c_str(),  "") != 0) {
        thisSensorInfo->valueJson[0] = systemHz;
        thisSensorInfo->isUpdated = true;
      }
      if (strcmp(thisSensorInfo->signalKPath[1].c_str(),  "") != 0) {
        thisSensorInfo->valueJson[1] = ESP.getFreeHeap();
        thisSensorInfo->isUpdated = true;
      }        
      
    }
  }

  systemHzCount = 0;
  systemHzMs = millis();
}


void handleSystemHz() {
  if (readyToUpdateHz) {
    // reset interupt
    readyToUpdateHz = false;
    updateSystemHz();
  }
  systemHzCount++;
}
