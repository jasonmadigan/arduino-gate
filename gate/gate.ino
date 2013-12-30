// Pins
#define GSMBAUD 9600  // TC35 Baud
#define CUTDOWNPIN 52 // To Relay
#define GSMGROUND 13

#define modemDelay 500

int SMS_location_number;
const unsigned int MAX_INPUT = 165; // 160 characters for SMS plus a few extra
static unsigned int input_pos = 0;

void setupModem() {
  // AT+CMGF=1          for txt mode
  // AT+CNMI=2,1,0,0,1  message indication
  // AT^SMGO=1          SMS full indication
  // AT&W               store to memory

  delay(modemDelay);

  //Set the Prefered Message Storage Parameter
  Serial2.println("AT+CMGF=1");
  delay(modemDelay);
  Serial2.println("AT+CNMI=2,1,0,0,1");
  delay(modemDelay);
  Serial2.println("AT^SMGO=1");
  delay(modemDelay);

  //Save Settings to Modem flash memory permanently
  Serial2.println("AT&W");
  delay(modemDelay);
}


void setup() {
  Serial.begin(9600);

  // Setup Radio
  pinMode(RADIOPIN, OUTPUT);

  // Setup Relay
  pinMode(CUTDOWNPIN, OUTPUT);
  digitalWrite(CUTDOWNPIN, HIGH);
  
  //--- turn on TC35 ---
  // To IGT pin on TC35
  // it grounds IGN pin for 100 ms
  // this is the same as pressing the button
  // on the TC35 to start it up

  pinMode(GSMGROUND, INPUT);
  digitalWrite(GSMGROUND, LOW);
  pinMode(GSMGROUND, OUTPUT);
  delay(100);
  pinMode(GSMGROUND, INPUT);

  // Setup GSM
  Serial2.begin(GSMBAUD);

  setupModem();

  Serial.println("Ready.");
}

// Main loop
void loop() {    
  // Read SMS queue
  readTC35();
}

void hangUp() {
  Serial.print("Hanging up.");
  Serial2.print("ATH\r");
  delay(100);
}

void openGate() {
  Serial.println("Opening - relay open.");
  digitalWrite(CUTDOWNPIN, LOW);
  delay(2500);
  Serial.println("Open sent.");
  digitalWrite(CUTDOWNPIN, HIGH);
  Serial.println("Relay closed.");
}

void readTC35() {
  static char input_line [MAX_INPUT];

  if (Serial2.available() > 0) {
    Serial.println("Serial2.available()");
    while (Serial2.available () > 0) {
      char inByte = Serial2.read();

      switch (inByte) {

      case '\n':   // end of text
        input_line [input_pos] = 0;  // terminating null byte

        // terminator reached! process input_line here ...
        process_data (input_line);

        // reset buffer for next time
        input_pos = 0;
        break;

      case '\r':   // discard carriage return
        break;

      default:
        // keep adding if not full ... allow for terminating null byte
        if (input_pos < (MAX_INPUT - 1))
          input_line [input_pos++] = inByte;
        break;

      }  // end of switch
    }  // end of while incoming data
  }  // end of if incoming data
}  // end of readTC35

void process_data (char * data) {

  // display the data
  Serial.print("*** Data: ");
  Serial.println(data);
  
  if(strstr(data, "RING")) {
    Serial.println("Ringing, sending.");
    hangUp();
    openGate();
  }

  if(strstr(data, "+CMTI:")) {   // An SMS has arrived
    char* copy = data + 12;      // Read from position 12 until a non ASCII number to get the SMS location
    SMS_location_number = (byte) atoi(copy);  // Convert the ASCII number to an int
    Serial2.print("AT+CMGR=");
    Serial2.println(SMS_location_number);  // Print the SMS in Serial Monitor
  }

  if(strstr(data, "open") || strstr(data, "Open")) {
    openGate();
  }

  if(strstr(data, "^SMGO: 2")) { // SIM card FULL
    delete_All_SMS();           // delete all SMS
  }
}

void delete_one_SMS() {
  Serial.print("deleting SMS ");
  Serial.println(SMS_location_number);
  Serial2.print("AT+CMGD=");
  Serial2.println(SMS_location_number);
}

void delete_All_SMS() {
  for(int i = 1; i <= 20; i++) {
    Serial2.print("AT+CMGD=");
    Serial2.println(i);
    Serial.print("deleting SMS ");
    Serial.println(i);
    delay(500);
  }
}
