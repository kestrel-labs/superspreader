/*
 *  ESP32
 *  DEEP Sleep and ULP wake up
 *  by Mischianti Renzo <https://www.mischianti.org>
 *
 *  https://www.mischianti.org/2021/03/23/esp32-practical-power-saving-preserve-gpio-status-external-and-ulp-wake-up-5/
 *
 */
 
#include "esp32/ulp.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
 
void init_ulp_program();
 
RTC_DATA_ATTR int bootCount = 0;
 
/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
 
  wakeup_reason = esp_sleep_get_wakeup_cause();
 
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP :
      Serial.println("Wakeup caused by ULP program");
      digitalWrite(16, !digitalRead(16));
      delay(5000);
      break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
 
void setup() {
    Serial.begin(115200);
    Serial.println("Init");
    pinMode(16, OUTPUT);
    //Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
 
    //Print the wakeup reason for ESP32
    print_wakeup_reason();
 
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        Serial.println("Initializing ULP");
        init_ulp_program();
        /* Set ULP wake up period to 5s */
        ulp_set_wakeup_period(0, 5 * 1000 * 1000);
    }
 
    Serial.println("Entering deep sleep\n");
    ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
 
    esp_deep_sleep_start();
}
 
void loop(){
}
 
void init_ulp_program() {
    const ulp_insn_t program[] = {
            // initiate wakeup of the SoC
            I_WAKE(),
            // stop the ULP program
            I_HALT()
    };
 
    size_t load_addr = 0;
    size_t size = sizeof(program)/sizeof(ulp_insn_t);
    ulp_process_macros_and_load(load_addr, program, &size);
 
    ulp_run(0);
}
