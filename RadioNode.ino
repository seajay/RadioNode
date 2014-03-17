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
const uint64_t pipes[2] = { 0xCCCCCCCC00, 0xCCCCCCCC01 };

// Start up as node 0 (master) ### Todo read this from EEPROM
uint8_t node_id = 0;

// ### Todo calc my max payload size. 32 is the radio maximum
#define RX_BUF_SIZE 32
char rx_buf[RX_BUF_SIZE];

enum msg_types {
  temp,
  time,
  button,
};

typedef struct msg {
  byte src_node;
  byte dest_node;
  enum msg_types msg_type;
  union data
  {
    float temp;
    //  ## todo other message types
  }
} msg;

msg *rx_msg;

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

  radio.setPayloadSize(RX_BUF_SIZE);

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
    rx_buf[RX_BUF_SIZE-1] =0; // Make sure null terminated ### don't leave this in!
    Serial.println(rx_buf);

    msg = &rx_buf;
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
    // Delay just a little bit to let the other unit
    // make the transition to receiver ### is this necessary?
    delay(20);
  }

  if ( Serial.available() > 2)
  {
    switch(Serial.read()) {
      case 'n':
        node_id = Serial.read(); // ### Todo range check
        setup_pipes(node_id);
        Serial.print("Set node id to ");
        Serial.println(node_id);
        break;
      case 's':  
        // Send string      
























  //
  // Ping out role.  Repeatedly send the current time
  //

  if (role == role_ping_out)
  {
    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    unsigned long time = millis();
    printf("Now sending %lu...",time);
    bool ok = radio.write( &time, sizeof(unsigned long) );
    
    if (ok)
      printf("ok...");
    else
      printf("failed.\n\r");

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout (250ms)
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 200 )
        timeout = true;

    // Describe the results
    if ( timeout )
    {
      printf("Failed, response timed out.\n\r");
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
      unsigned long got_time;
      radio.read( &got_time, sizeof(unsigned long) );

      // Spew it
      printf("Got response %lu, round-trip delay: %lu\n\r",got_time,millis()-got_time);
    }

    // Try again 1s later
    delay(1000);
  }

  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  //

  if ( role == role_pong_back )
  {
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      unsigned long got_time;
      bool done = false;
      while (!done)
      {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &got_time, sizeof(unsigned long) );

        // Spew it
        printf("Got payload %lu...",got_time);

	// Delay just a little bit to let the other unit
	// make the transition to receiver
	delay(20);
      }

      // First, stop listening so we can talk
      radio.stopListening();

      // Send the final one back.
      radio.write( &got_time, sizeof(unsigned long) );
      printf("Sent response.\n\r");

      // Now, resume listening so we catch the next packets.
      radio.startListening();
    }
  }

  //
  // Change roles
  //

}
// vim:cin:ai:sts=2 sw=2 ft=cpp
