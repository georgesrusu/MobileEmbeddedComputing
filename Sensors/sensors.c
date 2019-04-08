/*
 * Authors : George Rusu, Jeremy Bricq, Ilias Boulif
 * Mobile Embedded Computing Project
 * Sensor Nodes Code
 */


#include <stdio.h>
#include "random.h"
#include "contiki.h"
#include "net/rime/rime.h"

static struct broadcast_conn broadcastConnection;


/*-------------------------------Processes Definition -------------------------------------*/
PROCESS(broadcastProcess, "Broadcast Messages");
AUTOSTART_PROCESSES(&broadcastProcess);
/*-------------------------------BroadCast Thread Definition --------------------------------------------*/

static void broadcastReceive(struct broadcast_conn *c, const linkaddr_t *from){
  printf("broadcast message received from %d.%d: '%s'\n",from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}

static const struct broadcast_callbacks broadcastCallback = {broadcastReceive};

PROCESS_THREAD(broadcastProcess, ev, data)
{
  static struct etimer et;

  PROCESS_EXITHANDLER(broadcast_close(&broadcastConnection);)

  PROCESS_BEGIN();

  broadcast_open(&broadcastConnection, 129, &broadcastCallback);

  while(1) {

    /* Delay 2-4 seconds */
    etimer_set(&et, CLOCK_SECOND * 4 + random_rand() % (CLOCK_SECOND * 4));

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    packetbuf_copyfrom("Hello", 6);
    broadcast_send(&broadcastConnection);
    printf("broadcast message sent\n");
  }

  PROCESS_END();
}

