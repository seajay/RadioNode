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
// Todo, deal with more than two nodes by generating the pipe number from the node ID
const uint64_t pipes[2] = { 0xCCCCCCCC00LL, 0xCCCCCCCC01LL };

// Start up as node 0 (master) ### Todo read this from EEPROM
uint8_t node_id = 0;

// ### Todo calc my max payload size. 32 is the radio maximum
#define BUF_SIZE 32
uint8_t rx_buf[BUF_SIZE];

enum msg_types {
  debug,
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

void print_hex(uint8_t * data, int length)
{
  for (int n = 0; n < length; ++n){
    Serial.print(data[n], HEX);
  }
  Serial.println();
}
  

void setup(void)
{
  msg = (msg_t *)&rx_buf;
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
    print_hex(rx_buf, sizeof(rx_buf));

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
    
    if (msg->msg_type == debug)
    {
      Serial.println("Debug");
      Serial.println(msg->txt);
    }
    
  }

  if ( Serial.available())
  {
    switch(Serial.read()) {
      case 'n':
        delay(10); // Wait for the next serial character
        node_id = (char)Serial.read() - '0'; // ### Todo range check
        setup_pipes(node_id);
        Serial.print("Set node id to ");
        Serial.println(node_id);
        break;
  
      case 's':  
        // Send message syntax is snxxxxxxxx s is the send message command
        // n is the node to send to and xxxxxxxx is the message to send
        radio.stopListening(); // First, stop listening so we can talk.
        delay(10); // Wait for the next serial character
        msg->src_node = node_id;
        msg->dest_node = (char)Serial.read() - '0';
        msg->msg_type = debug;
        int n;
        for (n = 0; n < DATA_LEN-1; ++n){
          msg->txt[n] = Serial.read();  
          if (msg->txt[n] == -1)
          {
            msg->txt[n] = 0;       
            break;
          }
        }
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

