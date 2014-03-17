/*
 Created by C Jones <github@cjones.org.uk>
 Stored at https://github.com/seajay/RadioNode
 */

/**
 * Remote sensor node using nRF24L01+ radio.
 *
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

RF24 radio(9,10);

// Radio pipe addresses. Node N recieves on pipe[N].
const uint64_t pipes[2] = { 0xCCCCCCCC00LL, 0xCCCCCCCC01LL };

// Start up as node 0 (master) ### Todo read this from EEPROM
uint8_t node_id = 0;

// ### Todo calc my max payload size. 32 is the radio maximum
#define BUF_SIZE 32
char rx_buf[BUF_SIZE];

enum msg_types {
  temp,
  time,
  button,
};

#define DATA_LEN 20 // ## probably 20ish, maybe check this

typedef struct msg_t {
  byte src_node;
  byte dest_node;
  enum msg_types msg_type;
  union
  {
    float temp;
    char txt[DATA_LEN];  
    //  ## todo other message types
  };
} msg_t;

msg_t * msg;

void setup_pipes(uint8_t node)
{
  radio.openReadingPipe(1,pipes[node]);
  if ( node == 0 )
  {
    radio.openWritingPipe(pipes[1]);
  }
  else
  {
    radio.openWritingPipe(pipes[0]);
  }
}
  

void setup(void)
{
  Serial.begin(57600);
  printf_begin(); // Needed to allow radio.printDetails();

  Serial.println("Starting setup");
  Serial.print("Node: ");
  Serial.println(node_id);

  radio.begin();

  // Use the max delay and number of retries, we care more about reliability than performance
  radio.setRetries(15,15);

  radio.setPayloadSize(BUF_SIZE);

  setup_pipes(node_id);

  radio.startListening();

  radio.printDetails();
}

void loop(void)
{

  if ( radio.available() )
  {
    radio.read( rx_buf, sizeof(rx_buf) );
    Serial.println("Recieved message");
    rx_buf[BUF_SIZE-1] =0; // Make sure null terminated ### don't leave this in!
    Serial.println((char *)rx_buf);

    msg = (msg_t *)&rx_buf;
    Serial.print("src ");
    Serial.println(msg->src_node);
    Serial.print("dest ");
    Serial.print(msg->dest_node);

    if (msg->dest_node == node_id)
    {
      Serial.println(" me!");
    }
    else {
      Serial.println(" someone else");
    }
  }

  if ( Serial.available() > 2)
  {
    switch(Serial.read()) {
      case 'n':
        node_id = (char)Serial.read() - '0'; // ### Todo range check
        setup_pipes(node_id);
        Serial.print("Set node id to ");
        Serial.println(node_id);
        break;
  
      case 's':  
        // Send message      
        
        radio.stopListening(); // First, stop listening so we can talk.
        msg->src_node = node_id;
        msg->dest_node = (char)Serial.read() - '0';
        int n;
        for (n = 0; n < DATA_LEN-1; ++n){
          msg->txt[n] = Serial.read();  // Serial.read returns -1 if there is nothing to read so this will fill with 0xFF 
        }
        msg->txt[n] = 0; // terminator

        bool ok = radio.write( msg, BUF_SIZE );
        if (ok)
          printf("ok...");
        else
          printf("failed.\n\r");
    
        // Now, continue listening
        radio.startListening();
        break;
    }
  }
}

