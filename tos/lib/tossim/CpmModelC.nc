/*
 * Copyright (c) 2005 Stanford University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *
 * CPM (closest-pattern matching) is a wireless noise simulation model
 * based on statistical extraction from empirical noise data.  This
 * model provides far more precise software simulation environment by
 * exploiting time-correlated noise characteristics. For details,
 * please refer to the paper
 *
 * "Improving Wireless Simulation through Noise Modeling." HyungJune
 * Lee and Philip Levis, IPSN 2007. You can find a copy at
 * http://sing.stanford.edu.
 * 
 * @author Hyungjune Lee, Philip Levis
 * @date   Oct 12 2006
 */ 

#include <sim_gain.h>
#include <sim_noise.h>
#include <randomlib.h>

module CpmModelC {
  provides interface GainRadioModel as Model;
}

implementation {
  
  message_t* outgoing; // If I'm sending, this is my outgoing packet
  bool requestAck;
  bool receiving = FALSE;  // Whether or not I think I'm receiving a packet
  bool transmitting = FALSE; // Whether or not I think I'm transmitting a packet
  sim_time_t transmissionEndTime; // to check pending transmission

  struct receive_message;
  typedef struct receive_message receive_message_t;

  struct receive_message {
    sim_time_t start;
    sim_time_t end;
    double power;
    double reversePower;
    double pow10power_10; // cached pow(10.0, power / 10.0)
    int source;
    int8_t strength;
    bool lost;
    bool ack;
    message_t* msg;
    receive_message_t* next;
  };

  receive_message_t* outstandingReceptionHead = NULL;

  receive_message_t* allocate_receive_message();
  void free_receive_message(receive_message_t* msg);
  sim_event_t* allocate_receive_event(sim_time_t t, receive_message_t* m);

  bool shouldReceive(double SNR);
  bool checkReceive(const receive_message_t* msg);
  double packetNoise(const receive_message_t* msg);
  double checkPrr(const receive_message_t* msg);

  sim_time_t timeInMs(void) {
    const sim_time_t ticks_per_sec = sim_ticks_per_sec();
    const sim_time_t ftime = sim_time();

    sim_time_t ms_time = ftime / (ticks_per_sec / (1000ULL * 100));
    const sim_time_t ms_time_mod100 = ms_time % 100;

    // Forwarding old rounding a rounded number bug
    if (ms_time_mod100 >= 45)
    {
      ms_time += (100 - ms_time_mod100);
    }
    else
    {
      ms_time -= ms_time_mod100;
    }

    ms_time /= 100;

    return ms_time;
  }

  //Generate a CPM noise reading
  double noise_hash_generation(void)   {
    const sim_time_t CT = timeInMs();

    const uint16_t node_id = sim_node();
    double noise_val;

    dbg("CpmModelC", "IN: noise_hash_generation()\n");

    noise_val = (double)sim_noise_generate(node_id, CT);

    dbg("CpmModelC,Tal", "%s: OUT: noise_hash_generation(): %lf\n", sim_time_string(), noise_val);

    return noise_val;
  }
  
  double arr_estimate_from_snr(double SNR) {
    const double beta1 = 0.9794;
    const double beta2 = 2.3851;
    const double X = SNR-beta2;
    const double PSE = 0.5*erfc(beta1*X/M_SQRT2);
    double prr_hat = pow(1-PSE, 23*2);
    dbg("CpmModelC,SNRLoss", "SNR is %lf, ARR is %lf\n", SNR, prr_hat);
    if (prr_hat > 1)
      prr_hat = 1.1;
    else if (prr_hat < 0)
      prr_hat = -0.1;
        
    return prr_hat;
  }
  
  int shouldAckReceive(double snr) {
    double prr = arr_estimate_from_snr(snr);
    const double coin = RandomUniform(); // TODO: PERFORMANCE: Move inside if
    if ( (prr >= 0) && (prr <= 1) ) {
      if (coin < prr)
        prr = 1.0;
      else
        prr = 0.0;
    }
    return (int)prr;
  }
  
  void sim_gain_ack_handle(sim_event_t* evt)
  {
    // Four conditions must hold for an ack to be issued:
    // 1) Transmitter is still sending a packet (i.e., not cancelled)
    // 2) The packet requested an acknowledgment
    // 3) The transmitter is on
    // 4) The packet passes the SNR/ARR curve
    if (requestAck && // This 
        outgoing != NULL &&
        sim_mote_is_on(sim_node()))
    {
      const receive_message_t* rcv = (receive_message_t*)evt->data;
      const double power = rcv->reversePower;
      const double noise = packetNoise(rcv);
      const double snr = power - noise;

      if (shouldAckReceive(snr))
      {
        signal Model.acked(outgoing);
      }
    }

    free_receive_message((receive_message_t*)evt->data);
  }

  // This clear threshold comes from the CC2420 data sheet
  double clearThreshold = -72.0;

  command void Model.setClearValue(double value) {
    clearThreshold = value;
    dbg("CpmModelC", "Setting clear threshold to %f\n", clearThreshold);
  }
  
  command bool Model.clearChannel() {
    const double noise = packetNoise(NULL);
    dbg("CpmModelC", "Checking clear channel @ %s: %f <= %f \n", sim_time_string(), noise, clearThreshold);
    return noise < clearThreshold;
  }

  void sim_gain_schedule_ack(int source, sim_time_t t, receive_message_t* r) {
    sim_event_t* const ackEvent = sim_queue_allocate_raw_event();
    
    ackEvent->mote = source;
    ackEvent->force = 1;
    ackEvent->cancelled = 0;
    ackEvent->time = t;
    ackEvent->handle = sim_gain_ack_handle;
    ackEvent->cleanup = sim_queue_cleanup_event;
    ackEvent->data = r;
    
    sim_queue_insert(ackEvent);
  }

  double prr_estimate_from_snr(double SNR) {
    // Based on CC2420 measurement by Kannan.
    // The updated function below fixes the problem of non-zero PRR
    // at very low SNR. With this function PRR is 0 for SNR <= 3.
    const double beta1 = 0.9794;
    const double beta2 = 2.3851;
    const double X = SNR-beta2;
    const double PSE = 0.5*erfc(beta1*X/M_SQRT2);
    double prr_hat = pow(1-PSE, 23*2);
    dbg("CpmModelC,SNR", "SNR is %lf, PRR is %lf\n", SNR, prr_hat);
    if (prr_hat > 1)
      prr_hat = 1.1;
    else if (prr_hat < 0)
      prr_hat = -0.1;
        
    return prr_hat;
  }

  bool shouldReceive(double SNR) {
    double prr = prr_estimate_from_snr(SNR);
    const double coin = RandomUniform(); // TODO: PERFORMANCE: Move inside if
    if ( (prr >= 0) && (prr <= 1) ) {
      if (coin < prr)
        prr = 1.0;
      else
        prr = 0.0;
    }
    return prr;
  }

  bool checkReceive(const receive_message_t* msg) {
    double noise = noise_hash_generation();
    const receive_message_t* list;
    noise = pow(10.0, noise / 10.0);
    for (list = outstandingReceptionHead; list != NULL; list = list->next) {
      if (list != msg) {
        noise += list->pow10power_10;
      }
    }
    noise = 10.0 * log10(noise);
    return shouldReceive(msg->power - noise);
  }
  
  double packetNoise(const receive_message_t* msg) {
    double noise = noise_hash_generation();
    const receive_message_t* list;
    noise = pow(10.0, noise / 10.0);
    for (list = outstandingReceptionHead; list != NULL; list = list->next) {
      if (list != msg) {
        noise += list->pow10power_10;
      }
    }
    noise = 10.0 * log10(noise);
    return noise;
  }

  /*double checkPrr(const receive_message_t* msg) {
    return prr_estimate_from_snr(msg->power / packetNoise(msg));
  }*/
  

  /* Handle a packet reception. If the packet is being acked,
     pass the corresponding receive_message_t* to the ack handler,
     otherwise free it. */
  void sim_gain_receive_handle(sim_event_t* evt) {
    receive_message_t* const mine = (receive_message_t*)evt->data;
    receive_message_t* predecessor = NULL;
    receive_message_t* list;

    dbg("CpmModelC", "Handling reception event @ %s.\n", sim_time_string());
    for (list = outstandingReceptionHead; list != NULL; list = list->next) {
      if (list->next == mine) {
        predecessor = list;
        break;
      }
    }

    if (predecessor) {
      predecessor->next = mine->next;
    }
    else if (mine == outstandingReceptionHead) { // must be head
      outstandingReceptionHead = mine->next;
    }
    else {
      dbgerror("CpmModelC", "Incoming packet list structure is corrupted: entry is not the head and no entry points to it.\n");
    }

    dbg("CpmModelC,SNRLoss", "Packet from %i to %i\n", (int)mine->source, (int)sim_node());
    if (!checkReceive(mine)) {
      dbg("CpmModelC,SNRLoss", " - lost packet from %i as SNR was too low.\n", (int)mine->source);
      mine->lost = 1;
    }
    if (!mine->lost) {
      // Copy this receiver's packet signal strength to the metadata region
      // of the packet. Note that this packet is actually shared across all
      // receivers: a higher layer performs the copy.
      tossim_metadata_t* const meta = (tossim_metadata_t*)&mine->msg->metadata;
      meta->strength = mine->strength;
      
      dbg_clear("CpmModelC,SNRLoss", "  -signalling reception\n");
      signal Model.receive(mine->msg);
      if (mine->ack) {
        dbg_clear("CpmModelC", " acknowledgement requested, ");
      }
      else {
        dbg_clear("CpmModelC", " no acknowledgement requested.\n");
      }
      // If we scheduled an ack, receiving = 0 when it completes
      if (mine->ack && signal Model.shouldAck(mine->msg)) {
        dbg_clear("CpmModelC", " scheduling ack.\n");
        sim_gain_schedule_ack(mine->source, sim_time() + 1, mine);
      }
      else { // Otherwise free the receive_message_t*
        free_receive_message(mine);
      }
      // We're searching for new packets again
      receiving = 0;
    } // If the packet was lost, then we're searching for new packets again
    else {
      if (RandomUniform() < 0.001) {
        dbg("CpmModelC,SNRLoss", "Packet was technically lost, but TOSSIM introduces an ack false positive rate.\n");
        if (mine->ack && signal Model.shouldAck(mine->msg)) {
          dbg_clear("CpmModelC", " scheduling ack.\n");
          sim_gain_schedule_ack(mine->source, sim_time() + 1, mine);
        }
        else { // Otherwise free the receive_message_t*
          free_receive_message(mine);
        }
      }
      else {
        free_receive_message(mine);
      }
      receiving = 0;
      dbg_clear("CpmModelC,SNRLoss", "  -packet was lost.\n");
    }
  }
   
  // Create a record that a node is receiving a packet,
  // enqueue a receive event to figure out what happens.
  void enqueue_receive_event(int source, sim_time_t endTime, message_t* msg, bool receive, double power, double reversePower) {
    sim_event_t* evt;
    receive_message_t* list;
    receive_message_t* const rcv = allocate_receive_message();
    const double noiseStr = packetNoise(rcv);
    rcv->source = source;
    rcv->start = sim_time();
    rcv->end = endTime;
    rcv->power = power;
    rcv->reversePower = reversePower;
    rcv->pow10power_10 = pow(10.0, power / 10.0);
    // The strength of a packet is the sum of the signal and noise. In most cases, this means
    // the signal. By sampling this here, it assumes that the packet RSSI is sampled at
    // the beginning of the packet. This is true for the CC2420, but is not true for all
    // radios. But generalizing seems like complexity for minimal gain at this point.
    rcv->strength = (int8_t)floor(10.0 * log10(rcv->pow10power_10 + pow(10.0, noiseStr/10.0)));
    rcv->msg = msg;
    rcv->lost = 0;
    rcv->ack = receive;
    // If I'm off, I never receive the packet, but I need to keep track of
    // it in case I turn on and someone else starts sending me a weaker
    // packet. So I don't set receiving to 1, but I keep track of
    // the signal strength.

    if (!sim_mote_is_on(sim_node())) { 
      dbg("CpmModelC,PacketLoss", "Lost packet from %i due to %i being off\n", source, sim_node());
      rcv->lost = 1;
    }
    else if (!shouldReceive(power - noiseStr)) {
      dbg("CpmModelC,SNRLoss,PacketLoss", "Lost packet from %i to %i due to SNR being too low (%i)\n", source, sim_node(), (int)(power - noiseStr));
      rcv->lost = 1;
    }
    else if (receiving) {
      dbg("CpmModelC,SNRLoss,PacketLoss", "Lost packet from %i due to %i being mid-reception\n", source, sim_node());
      rcv->lost = 1;
    }
    else if (transmitting && (rcv->start < transmissionEndTime) && (transmissionEndTime <= rcv->end)) {
      dbg("CpmModelC,SNRLoss,PacketLoss", "Lost packet from %i due to %i being mid-transmission, transmissionEndTime %llu\n", source, sim_node(), transmissionEndTime);
      rcv->lost = 1;
    }
    else {
      receiving = 1;
    }

    
    for (list = outstandingReceptionHead; list != NULL; list = list->next) {
      if (!shouldReceive(list->power - rcv->power)) {
        dbg("Gain,SNRLoss", "Going to lose packet from %i with signal %lf as am receiving a packet from %i with signal %lf\n", list->source, list->power, source, rcv->power);
        list->lost = 1;
      }
    }
    
    rcv->next = outstandingReceptionHead;
    outstandingReceptionHead = rcv;
    evt = allocate_receive_event(endTime, rcv);
    sim_queue_insert(evt);
  }
  
  void sim_gain_put(int dest, message_t* msg, sim_time_t endTime, bool receive, double power, double reversePower) {
    const int prevNode = sim_node();
    dbg("CpmModelC", "Enqueueing reception event for %i at %llu with power %lf.\n", dest, endTime, power);
    sim_set_node(dest);
    enqueue_receive_event(prevNode, endTime, msg, receive, power, reversePower);
    sim_set_node(prevNode);
  }

  command void Model.putOnAirTo(int dest, message_t* msg, bool ack, sim_time_t endTime, double power, double reversePower) {
    receive_message_t* list;
    const void* neighborEntryIter;
    requestAck = ack;
    outgoing = msg;
    transmissionEndTime = endTime;
    dbg("CpmModelC", "Node %i transmitting to %i, finishes at %llu.\n", sim_node(), dest, endTime);

    for (neighborEntryIter = sim_gain_iter(sim_node());
         neighborEntryIter != NULL;
         neighborEntryIter = sim_gain_next(sim_node(), neighborEntryIter))
    {
      const gain_entry_t* gain = sim_gain_iter_get(neighborEntryIter);
      const int other = gain->mote;
      const double other_gain = gain->gain;
      sim_gain_put(other, msg, endTime, ack, power + other_gain, reversePower + sim_gain_value(other, sim_node()));
    }

    for (list = outstandingReceptionHead; list != NULL; list = list->next) {    
      list->lost = 1;
      dbg("CpmModelC,SNRLoss", "Lost packet from %i because %i has outstanding reception, startTime %llu endTime %llu\n",
        list->source, sim_node(), list->start, list->end);
    }
  }


  command void Model.setPendingTransmission() {
    transmitting = TRUE;
    dbg("CpmModelC", "setPendingTransmission: transmitting %i @ %s\n", transmitting, sim_time_string());
  }

 default event void Model.receive(message_t* msg) {}

 sim_event_t* allocate_receive_event(sim_time_t endTime, receive_message_t* msg) {
   sim_event_t* evt = sim_queue_allocate_raw_event();
   evt->mote = sim_node();
   evt->time = endTime;
   evt->handle = sim_gain_receive_handle;
   evt->cleanup = sim_queue_cleanup_event;
   evt->cancelled = 0;
   evt->force = 1; // Need to keep track of air even when node is off
   evt->data = msg;
   return evt;
 }

 receive_message_t* allocate_receive_message() {
   return (receive_message_t*)malloc(sizeof(receive_message_t));
 }

 void free_receive_message(receive_message_t* msg) {
   free(msg);
 }
}
