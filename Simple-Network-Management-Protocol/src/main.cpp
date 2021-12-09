/************************************************************
 *Property-off: PT. INTI (PERSERO)                          *
 *Written by  : SNMP Project Tel-U                          *
 *Date        : 27-Juli-2019                                *
 *                                                          *
 * *********************************************************/
#include <Arduino.h>
#include <Streaming.h>         // Include the Streaming library
#include <UIPEthernet.h>       // Include the Ethernet library
#include <SPI.h>
#include <MemoryFree.h>
#include <UIPAgentuino.h> 
#include <Flash.h>
#include <DHT.h>

char oid[10][SNMP_MAX_OID_LEN];

uint32_t  prevMillis = millis(),
          adcValue,
          dcVoltage,
          acVoltageMaks,
          acVoltageEff,
          acValue,
          acValue2,
          Vmax,
          VeffD,
          Veff;

const int ledPin = 13,
          dhtPin = 25,
          dcVoltagePin = 36, //31
          acVoltagePin = 23;
          
float humidityInPercent,
      temperatureInCelcius,
      tempperatureInCelcius;

const float akarDua = 1.4142135624;

int help,
    maks,
    maks2,
    counting,
    data[20];

DHT dht(dhtPin, DHT22);   // DHT Sensor Initials Pin and DHT's Kind
SNMP_API_STAT_CODES api_status;
SNMP_ERR_CODES status;

static byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static byte ip[] = { 192, 168, 20, 6 };
static byte RemoteIP[4] = {192, 168, 20, 2}; // The IP address of the host that will receive the trap

const char sysDescr[] PROGMEM      = "1.3.6.1.2.1.1.1.0";  // read-only  (DisplayString)
const char sysUpTime[] PROGMEM     = "1.3.6.1.2.1.1.3.0";  // read-only  (TimeTicks)
const char sysContact[] PROGMEM    = "1.3.6.1.2.1.1.4.0";  // read-write (DisplayString)
const char sysName[] PROGMEM       = "1.3.6.1.2.1.1.5.0";  // read-write (DisplayString)
const char sysLocation[] PROGMEM   = "1.3.6.1.2.1.1.6.0";  // read-write (DisplayString)
const char sysServices[] PROGMEM   = "1.3.6.1.2.1.1.7.0";  // read-only  (Integer)

// RFC1213 local values
static char locDescr[]              = "Agentuino, a light-weight SNMP Agent.";  // read-only (static)
static uint32_t locUpTime           = 0;                                        // read-only (static)
static char locContact[20]          = "Petr Domorazek";                         // should be stored/read from EEPROM - read/write (not done for simplicity)
static char locName[20]             = "Agentuino";                              // should be stored/read from EEPROM - read/write (not done for simplicity)
static char locLocation[20]         = "Czech Republic";                         // should be stored/read from EEPROM - read/write (not done for simplicity)
static int32_t locServices          = 6;                                        // read-only (static)

const char temperatureC[]       PROGMEM     = "1.3.6.1.3.2019.5.1.0";     //Temperature in Celsius
const char humidity[]           PROGMEM     = "1.3.6.1.3.2019.5.2.0";     //Humidity in Percent
const char ACVoltage[]          PROGMEM     = "1.3.6.1.3.2019.5.3.0";     //Effective AC Voltage in Volt
const char DCVoltage[]          PROGMEM     = "1.3.6.1.3.2019.5.4.0";     //DC Voltage in Volt
const char enterpriseOID[]      PROGMEM     = "1.3.6.1.3.2019.0";         //Enterprise OID

void pduReceived(){
  SNMP_PDU pdu;
  api_status = Agentuino.requestPdu(&pdu);
  
  if ((pdu.type == SNMP_PDU_GET || pdu.type == SNMP_PDU_GET_NEXT || pdu.type == SNMP_PDU_SET)
    && pdu.error == SNMP_ERR_NO_ERROR && api_status == SNMP_API_STAT_SUCCESS){

    // Implementation SNMP GET NEXT
    Serial.println("-------------------------");
    Serial.println("SNMP Implementation");

    for(int _number=0; _number<= pdu.SNMP_TOTAL_OID; _number++){
      pdu.OID[_number].toString(oid[_number]);
    
      if (pdu.type == SNMP_PDU_GET_NEXT){
        char tmpOIDfs[SNMP_MAX_OID_LEN];
        //Serial.println("SNMP GET Implementation");
        
        if (strcmp_P(oid[_number], sysDescr) == 0){
          strcpy_P(oid[_number], sysUpTime);
          strcpy_P(tmpOIDfs, sysUpTime);
          pdu.OID[_number].fromString(tmpOIDfs);} 
        else if (strcmp_P(oid[_number], sysUpTime) == 0){
          strcpy_P(oid[_number], sysContact);
          strcpy_P(tmpOIDfs, sysContact);
          pdu.OID[_number].fromString(tmpOIDfs);}
        else if (strcmp_P(oid[_number], sysContact) == 0){  
          strcpy_P(oid[_number], sysName);
          strcpy_P(tmpOIDfs, sysName);
          pdu.OID[_number].fromString(tmpOIDfs);}
        else if (strcmp_P(oid[_number], sysName) == 0){
          strcpy_P(oid[_number], sysLocation);
          strcpy_P(tmpOIDfs, sysLocation);
          pdu.OID[_number].fromString(tmpOIDfs);} 
        else if (strcmp_P(oid[_number], sysLocation) == 0){
          strcpy_P(oid[_number], sysServices);
          strcpy_P(tmpOIDfs, sysServices);
          pdu.OID[_number].fromString(tmpOIDfs);}
        else if (strcmp_P(oid[_number], sysServices) == 0){
          strcpy_P(oid[_number], temperatureC);
          strcpy_P(tmpOIDfs, temperatureC);
          pdu.OID[_number].fromString(tmpOIDfs);}
        else if (strcmp_P(oid[_number], temperatureC) == 0){
          strcpy_P(oid[_number], humidity);
          strcpy_P(tmpOIDfs, humidity);
          pdu.OID[_number].fromString(tmpOIDfs);}
        else if (strcmp_P(oid[_number], humidity) == 0){
          strcpy_P(oid[_number], ACVoltage);
          strcpy_P(tmpOIDfs, ACVoltage);
          pdu.OID[_number].fromString(tmpOIDfs);}
        else if (strcmp_P(oid[_number], ACVoltage) == 0){
          strcpy_P(oid[_number], DCVoltage);
          strcpy_P(tmpOIDfs, DCVoltage);
          pdu.OID[_number].fromString(tmpOIDfs);}
        else if (strcmp_P(oid[_number], DCVoltage) == 0){
          strcpy_P(oid[_number], "1.0");}
        else{
          int ilen = strlen(oid[_number]);
          if (strncmp_P(oid[_number], sysDescr, ilen) == 0){
            strcpy_P(oid[_number], sysDescr);
            strcpy_P(tmpOIDfs, sysDescr);
            pdu.OID[_number].fromString(tmpOIDfs);} 
          else if (strncmp_P(oid[_number], sysUpTime, ilen) == 0 ){
            strcpy_P(oid[_number], sysUpTime);
            strcpy_P(tmpOIDfs, sysUpTime);
            pdu.OID[_number].fromString(tmpOIDfs);}
          else if (strncmp_P(oid[_number], sysContact, ilen) == 0){
            strcpy_P(oid[_number], sysContact );
            strcpy_P(tmpOIDfs, sysContact);        
            pdu.OID[_number].fromString(tmpOIDfs);}
          else if (strncmp_P(oid[_number], sysName, ilen) == 0){
            strcpy_P(oid[_number], sysName);
            strcpy_P(tmpOIDfs, sysName);
            pdu.OID[_number].fromString(tmpOIDfs);}
          else if (strncmp_P(oid[_number], sysLocation, ilen) == 0){
            strcpy_P(oid[_number], sysLocation); 
            strcpy_P(tmpOIDfs, sysLocation);
            pdu.OID[_number].fromString(tmpOIDfs);}
          else if (strncmp_P(oid[_number], sysServices, ilen) == 0){
            strcpy_P(oid[_number], sysServices);
            strcpy_P(tmpOIDfs, sysServices);
            pdu.OID[_number].fromString(tmpOIDfs);} 
          else if (strncmp_P(oid[_number], temperatureC, ilen) == 0){
            strcpy_P(oid[_number], temperatureC);
            strcpy_P(tmpOIDfs, temperatureC);
            pdu.OID[_number].fromString(tmpOIDfs);}
          else if (strncmp_P(oid[_number], humidity, ilen) == 0){
            strcpy_P(oid[_number], humidity);
            strcpy_P(tmpOIDfs, humidity);
            pdu.OID[_number].fromString(tmpOIDfs);}
          else if (strncmp_P(oid[_number], ACVoltage, ilen) == 0){
            strcpy_P(oid[_number], ACVoltage);
            strcpy_P(tmpOIDfs, ACVoltage);
            pdu.OID[_number].fromString(tmpOIDfs);}
          else if (strncmp_P(oid[_number], DCVoltage, ilen) == 0){
            strcpy_P(oid[_number], DCVoltage);
            strcpy_P(tmpOIDfs, DCVoltage);
            pdu.OID[_number].fromString(tmpOIDfs);}}}
      // End of implementation SNMP GET NEXT / WALK
  
      // Start of Implementation SNMP GET / SET
      if (strcmp_P(oid[_number], sysDescr) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = SNMP_ERR_READ_ONLY;} 
        else{                           //Get Request
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_OCTETS, locDescr);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}}
      else if (strcmp_P(oid[_number], sysUpTime) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = SNMP_ERR_READ_ONLY;}
        else{                           //Get Request
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_TIME_TICKS, locUpTime);       
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}} 
      else if (strcmp_P(oid[_number], sysName) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          status = pdu.VALUE[_number].decode(locName, strlen(locName)); 
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;} 
        else{                           //Get Request
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_OCTETS, locName);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}} 
      else if (strcmp_P(oid[_number], sysContact) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          status = pdu.VALUE[_number].decode(locContact, strlen(locContact)); 
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;} 
        else{                           //Get Request
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_OCTETS, locContact);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}} 
      else if (strcmp_P(oid[_number], sysLocation) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          status = pdu.VALUE[_number].decode(locLocation, strlen(locLocation)); 
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;} 
        else{                           //Get Request
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_OCTETS, locLocation);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}}
      else if (strcmp_P(oid[_number], sysServices) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = SNMP_ERR_READ_ONLY;} 
        else{                           //Get Request
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_INT, locServices);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}} 
      else if (strcmp_P(oid[_number], temperatureC) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          Serial.println("Handle temperatureC Set Requests");
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = SNMP_ERR_READ_ONLY;}
        else{                           //Get Request
          Serial.println("Handle temperatureC Get Requests");
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_UINT32, (uint32_t)temperatureInCelcius);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}}
      else if (strcmp_P(oid[_number], humidity) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          Serial.println("Handle Humidity Set Requests");
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = SNMP_ERR_READ_ONLY;}
        else{                           //Get Request
          Serial.println("Handle Humidity Get Requests");
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_UINT32, (uint32_t)humidityInPercent);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}}
      else if (strcmp_P(oid[_number], ACVoltage) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          Serial.println("Handle Effective AC Voltage Set Requests");
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = SNMP_ERR_READ_ONLY;}
        else{                           //Get Request
          Serial.println("Handle Effective AC Voltage Get Requests");
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_UINT32, acVoltageEff);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}}
      else if (strcmp_P(oid[_number], DCVoltage) == 0){
        if (pdu.type == SNMP_PDU_SET){  //Set Request
          Serial.println("Handle DC Voltage Set Requests");
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = SNMP_ERR_READ_ONLY;}
        else{                           //Get Request
          Serial.println("Handle DC Voltage Get Requests");
          status = pdu.VALUE[_number].encode(SNMP_SYNTAX_UINT32, dcVoltage);
          pdu.type = SNMP_PDU_RESPONSE;
          pdu.error = status;}}
      else {
        // OID Does Not Exist || Response Packet - Object Not Found
        Serial.println("OID Does Not Exist");
        pdu.type = SNMP_PDU_RESPONSE;
        pdu.error = SNMP_ERR_NO_SUCH_NAME;
      }
      // End of Implementation SNMP GET / SET
    }
      
    Serial.println("Agentuino ResponsePDU");
    Agentuino.responsePdu(&pdu);}

  Serial.println("Agentuino FreePDU");
  Serial.println("-------------------------");
  Agentuino.freePdu(&pdu);}

void pduTrapListen(){
  SNMP_TRAP_PDU pduu;
  
  char oid[SNMP_MAX_OID_LEN];
  char tmpOIDfs[SNMP_MAX_OID_LEN];
  
  //Enterprise OID
  strcpy_P(tmpOIDfs, enterpriseOID);
  pduu.EnterpriseTrapOID.fromString(tmpOIDfs);
  pduu.EnterpriseTrapOID.toString(oid);
  
  Serial.println("-------------------------");
  if(dcVoltage <= 5){                                           //DC Voltage Trap
    strcpy_P(tmpOIDfs, DCVoltage);
    pduu.TrapOID.fromString(tmpOIDfs);
    pduu.TrapOID.toString(oid);

    Agentuino.Trap("DC VOLTAGE UNDER LIMIT!", RemoteIP, ip, locUpTime, &pduu);
    Agentuino.freeTrapPdu(&pduu);
    Serial.println("TRAP : DC VOLTAGE UNDER LIMIT!");}

  if(acVoltageEff <= 200){                                       //AC Voltage Trap Under Voltage
    strcpy_P(tmpOIDfs, ACVoltage);
    pduu.TrapOID.fromString(tmpOIDfs);
    pduu.TrapOID.toString(oid);

    Agentuino.Trap("AC VOLTAGE UNDER LIMIT!", RemoteIP, ip, locUpTime, &pduu);
    Agentuino.freeTrapPdu(&pduu);
    Serial.println("TRAP : AC VOLTAGE UNDER LIMIT!");}

  if(acVoltageEff >= 250){                                       //AC Voltage Trap Upper Voltage
    strcpy_P(tmpOIDfs, ACVoltage);
    pduu.TrapOID.fromString(tmpOIDfs);
    pduu.TrapOID.toString(oid);

    Agentuino.Trap("AC VOLTAGE PASS LIMIT!", RemoteIP, ip, locUpTime, &pduu);
    Agentuino.freeTrapPdu(&pduu);
    Serial.println("TRAP : AC VOLTAGE PASS LIMIT!");}

  if(humidityInPercent < 45 || humidityInPercent > 65){        //Humidity Trap
    strcpy_P(tmpOIDfs, humidity);
    pduu.TrapOID.fromString(tmpOIDfs);
    pduu.TrapOID.toString(oid);

    Agentuino.Trap("HUMIDITY NOT IDEAL!", RemoteIP, ip, locUpTime, &pduu);
    Agentuino.freeTrapPdu(&pduu);
    Serial.println("TRAP : HUMIDITY NOT IDEAL!");}
    
  Serial.println("-------------------------");}

void readDHT22(){
  humidityInPercent = dht.readHumidity();
  temperatureInCelcius = dht.readTemperature();
  tempperatureInCelcius = dht.readTemperature(true);

  if (isnan(humidityInPercent) || isnan(temperatureInCelcius) || isnan(tempperatureInCelcius)) {
    Serial.println("Failed to read from DHT sensor!");
    return;}

  float hif = dht.computeHeatIndex(tempperatureInCelcius, humidityInPercent);
  float hic = dht.computeHeatIndex(temperatureInCelcius, humidityInPercent, false);}

void readDcVoltage(){
  float R1 = 30000,
        R2 = 7500,
        dcInput;
  
  adcValue = analogRead(dcVoltagePin);
  dcInput = adcValue*(3.3/1023);
  dcVoltage = dcInput / (R2/(R1+R2));}

void readAcVoltage(){
  acValue = analogRead(acVoltagePin);
  data[counting] = acValue;

  if(counting < 10){
    if(data[counting] > maks){      //Grafik Naik
      maks = data[counting];}
    else if(data[counting] < maks){ //Grafik Turun
      maks = maks;}
    counting++;}
  else if(counting == 10){
    acVoltageMaks = (maks, 600, 840, 0, 339);
    acVoltageEff = acVoltageMaks/akarDua;
    maks = 0;
    counting = 0;}
  delay(82);}

void readAllSensor(){
  readAcVoltage();
  readDcVoltage();
  readDHT22();}

void setup(){
  Serial.begin(9600);
  Ethernet.begin(mac, ip);
  dht.begin();

  Serial.println("SNMP Init..");
  api_status = Agentuino.begin();
  if (api_status == SNMP_API_STAT_SUCCESS){
    Agentuino.onPduReceive(pduReceived);
    return;}
    
  delay(1000);
  pinMode(ledPin, INPUT);
  pinMode(dcVoltagePin, INPUT);
  pinMode(acVoltagePin, INPUT);
  Serial.println("Initialized!");}

void loop(){
  readAllSensor();        // Read Sensor
  pduTrapListen();        // Trap Identification
  Agentuino.listen();     // Listen Incoming SNMP Requests

  if (millis() - prevMillis > 1000){
    prevMillis += 1000;   // increment previous ms
    locUpTime += 100;     // increment up-time counter on
  }}
