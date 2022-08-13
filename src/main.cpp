#include <Arduino.h>
#include "config.h"
#include "certificates.h"
#include <PromLokiTransport.h>
#include <PrometheusArduino.h>
#include <OneWire.h>
#include <DFRobot_ORP_PRO.h>
#include <DFRobot_PH.h>
#include <esp_task_wdt.h>

#define PH_PIN A1
float voltage, phValue, orpValue;
DFRobot_PH ph;

OneWire ds(25);

#define PIN_ORP A0
#define ADC_RES 4096
#define V_REF 3300

float ADC_voltage;

DFRobot_ORP_PRO ORP(0);

PromLokiTransport transport;
PromClient client(transport);

// Create a write request for 2 series.
WriteRequest req(4);

// Define a TimeSeries which can hold up to 5 samples, has a name of `uptime_milliseconds`
TimeSeries ts1(5, "uptime_milliseconds_total", "{job=\"pool\",host=\"poolmon\"}");

// Define a TimeSeries which can hold up to 5 samples, has a name of `orp`
TimeSeries ts2(5, "orp", "{job=\"pool\",host=\"poolmon\"}");

// Define a TimeSeries which can hold up to 5 samples, has a name of `ph`
TimeSeries ts3(5, "ph", "{job=\"pool\",host=\"poolmon\"}");

// Define a TimeSeries which can hold up to 5 samples, has a name of `temp`
TimeSeries ts4(5, "temp", "{job=\"pool\",host=\"poolmon\"}");

int loopCounter = 0;

void setup()
{
  Serial.begin(115200);

  // Start the watchdog timer, sometimes connecting to wifi or trying to set the time can fail in a way that never recovers
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  // Wait 5s for serial connection or continue without it
  // some boards like the esp32 will run whether or not the
  // serial port is connected, others like the MKR boards will wait
  // for ever if you don't break the loop.
  uint8_t serialTimeout;
  while (!Serial && serialTimeout < 50)
  {
    delay(100);
    serialTimeout++;
  }

  Serial.println("Starting");

  // Configure and start the transport layer
  transport.setWifiSsid(WIFI_SSID);
  transport.setWifiPass(WIFI_PASS);
  transport.setNtpServer(NTP);
  transport.setUseTls(true);
  transport.setCerts(cert, strlen(cert));
  transport.setDebug(Serial); // Remove this line to disable debug logging of the client.
  if (!transport.begin())
  {
    Serial.println(transport.errmsg);
    while (true)
    {
    };
  }

  // Configure the client
  client.setUrl(PROM_URL);
  client.setPath((char *)PROM_PATH);
  client.setPort(PORT);
  client.setDebug(Serial); // Remove this line to disable debug logging of the client.
  if (!client.begin())
  {
    Serial.println(client.errmsg);
    while (true)
    {
    };
  }

  // Add our TimeSeries to the WriteRequest
  req.addTimeSeries(ts1);
  req.addTimeSeries(ts2);
  req.addTimeSeries(ts3);
  req.addTimeSeries(ts4);
  req.setDebug(Serial); // Remove this line to disable debug logging of the write request serialization and compression.

  ph.begin();

  Serial.print("Free Mem After Setup: ");
  Serial.println(freeMemory());
};

void loop()
{

  int64_t time;
  time = transport.getTimeMillis();
  Serial.println(time);

  byte i;
  byte present = 0;
  byte data[9];
  byte addr[8];
  byte type_s;
  float celsius;
  if (!ds.search(addr))
  {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  Serial.print("ROM =");
  for (i = 0; i < 8; i++)
  {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

  if (OneWire::crc8(addr, 7) != addr[7])
  {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1); // start conversion, with parasite power on at the end

  delay(1000); // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad

  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
  for (i = 0; i < 9; i++)
  { // we need 9 bytes
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s)
  {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10)
    {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  }
  else
  {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00)
      raw = raw & ~7; // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20)
      raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40)
      raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");

  // temperature = readTemperature();         // read your temperature sensor to execute temperature compensation
  voltage = analogRead(PH_PIN) / 4096.0 * 3300; // read the voltage
  Serial.print("Voltage");
  Serial.println(voltage);
  phValue = ph.readPH(voltage, celsius); // convert voltage to pH with temperature compensation
  Serial.print("pH:");
  Serial.println(phValue, 2);

  //ADC_voltage = ((unsigned long)analogRead(PIN_ORP) * V_REF + ADC_RES / 2) / ADC_RES;
  ADC_voltage = analogRead(PIN_ORP);
  Serial.print("ORP value is : ");
  //orpValue = ORP.getORP(ADC_voltage);
  Serial.print(ADC_voltage);
  Serial.println("mV");

  if (!ts1.addSample(time, millis()))
  {
    Serial.println(ts1.errmsg);
  }
  if (!ts2.addSample(time, ADC_voltage))
  {
    Serial.println(ts2.errmsg);
  }
  if (!ts3.addSample(time, voltage))
  {
    Serial.println(ts3.errmsg);
  }
  if (!ts4.addSample(time, celsius))
  {
    Serial.println(ts4.errmsg);
  }

  // Send data

  PromClient::SendResult res = client.send(req);
  if (!res == PromClient::SendResult::SUCCESS)
  {
    Serial.println(client.errmsg);
    // Note: additional retries or error handling could be implemented here.
    // the result could also be:
    // PromClient::SendResult::FAILED_DONT_RETRY
    // PromClient::SendResult::FAILED_RETRYABLE
  }
  // Batches are not automatically reset so that additional retry logic could be implemented by the library user.
  // Reset batches after a succesful send.
  ts1.resetSamples();
  ts2.resetSamples();
  ts3.resetSamples();
  ts4.resetSamples();

  // Prometheus default is 15 second intervals but you can send several times per second if you want to.
  // Collection and Sending could be parallelized or timed to ensure we're on a 15 seconds cadence,
  // not simply add 15 second to however long collection & sending took.
  delay(15000);

  // Reset watchdog, this also gives the most time to handle OTA
  // be careful if your wdt reset is too quick you may want to disable
  // it before OTA.
  esp_task_wdt_reset();
};