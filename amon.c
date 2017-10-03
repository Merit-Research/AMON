//================================================================================//
//================================================================================//
/*
 *
 * AMON Copyright 2016 The Regents of The University of Michigan
 *
 * Authors  - Michalis Kallitsis <mgkallit@merit.edu>
 *            Stilian Stoev <sstoev@umich.edu>
 *            Rajat Tandon <rajattan@usc.edu>
 *
 * Licensed under the GNU Lesser General Public License, version 3. You may
 * obtain a copy of the license at https://www.gnu.org/licenses/gpl-3.0.html
 */
//================================================================================//
//================================================================================//


#define _GNU_SOURCE
#include <signal.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/poll.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <net/ethernet.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <monetary.h>
#include <locale.h>
#include <pcap.h>

#include "pfring.h"
#include "pfutils.c"
#include "haship.h"
#include "haship.c"
#include "bm_structs.h"
#include <bson.h>
#include <mongoc.h>
#define MAX_NUM_DEVS 64
#define MAXLEN 128
#define CONFIG_FILE "amon.config"
#define VERBOSE_SUPPORT

pfring *pd[MAX_NUM_DEVS];
struct pollfd pfd[MAX_NUM_DEVS];
int num_devs = 0;
#ifdef VERBOSE_SUPPORT
int verbose = 0;
#endif
pfring_stat pfringStats;

static struct timeval startTime;
unsigned long long numPkts = 0, numBytes = 0;
u_int8_t wait_for_packet = 0, do_shutdown = 0;
int poll_duration = DEFAULT_POLL_DURATION;

//=======================================//
//==== Declare AMON-related variables====//
//=======================================//
pthread_mutex_t critical_section_lock;
u_int8_t modality_type;

unsigned int *P_bm;
unsigned int *P_bm_r;
unsigned int *P_bm_tmp;

flow_t *cand;		            /* array of candidate keys */
flow_t *cand_r;		            /* helper array for reading candidate keys */
flow_t *cand_tmp;	            /* tmp array used for swapping the above pointers */

long *count;		            /* array of counters */
long *count_r;		            /* helper array for reading counts */
long *count_tmp;	            /* tmp array used for swapping the above pointers */

//unsigned int *major_flags;	    /* flag indicating whether majority exists */
//unsigned int *major_flags_r;	    /* (helper for reading) flag indicating whether majority exists */
//unsigned int *major_flags_tmp;	    /* (tmp for swapping)  flag indicating whether majority exists */

unsigned int *databrick;	    /* databrick array */
unsigned int *databrick_r;	    /* (helper for reading) databrick array */
unsigned int *databrick_tmp;	    /* (tmp for swapping)  databrick array */

pthread_mutex_t critical_section_lock = PTHREAD_MUTEX_INITIALIZER;
flow_t pqueue[HEAPSIZE];            /* Heap data structure (priority queue)*/
struct nlist *hashtab[HASHSIZE];    /* Hash table associated with the priority queue*/

struct passingThreadParams
{
        int caller_id;
        int callee_id;
};

u_int32_t T1_16[65536];
u_int32_t T2_16[65536];
u_int32_t T3_17[131072];	     /*2^17-long array */
int IDX[Low14];
int IDX17[131072];		     /*2^17-long array */
int STRATA_IDX17_prefix_bin[131072]; /*2^17-long array */
int seed = 134;

struct conf_param {

        int alarm_sleep;
        int default_snaplen;
        char default_device[MAXLEN];
        char mongo_db_client[MAXLEN];
        char database[MAXLEN];
        char db_collection[MAXLEN];
        int seed;
        char strata_file[MAXLEN];
        char prefix_file[MAXLEN];

}
conf_param;
struct conf_param parms;


//====================================================//
//===== Function to trim strings for config file =====//
//====================================================//
char *trim(char *str)
{
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;

    if( str == NULL ) { return NULL; }
    if( str[0] == '\0' ) { return str; }

    len = strlen(str);
    endp = str + len;

    while( isspace((unsigned char) *frontp) ) { ++frontp; }
    if( endp != frontp )
    {
        while( isspace((unsigned char) *(--endp)) && endp != frontp ) {}
    }

    if( str + len - 1 != endp )
            *(endp + 1) = '\0';
    else if( frontp != str &&  endp == frontp )
            *str = '\0';

    endp = str;
    if( frontp != str )
    {
            while( *frontp ) { *endp++ = *frontp++; }
            *endp = '\0';
    }

    return str;
}

//=====================================================//
//===== Function to parse strings for config file =====//
//=====================================================//
void
parse_config (struct conf_param * parms)
{
  char *s, buff[256];
  FILE *fp = fopen (CONFIG_FILE, "r");
  if (fp == NULL)
  {
    printf ("\n Config file %s does not exist. Please include it and re-run.. \n",CONFIG_FILE);
    exit (0);
  }
  printf ("\n Reading config file %s ...",CONFIG_FILE);
  while ((s = fgets (buff, sizeof buff, fp)) != NULL)
  {
        /* Skip blank lines and comment lines */
        if (buff[0] == '\n' || buff[0] == '#')
          continue;

        /* Parse name/value pair from line */
        char name[MAXLEN], value[MAXLEN];
        memset(name, '\0', sizeof(name));
        memset(value, '\0', sizeof(value));
        s = strtok (buff, "=");
        if (s==NULL)
          continue;
        else
        {  strcpy (name, s);
           trim (name);
        } 
        s = strtok (NULL, "=");
        if (s==NULL)
          continue;
        else
        {
          strcpy (value, s);
          trim (value);
        }

        /* Copy into correct entry in parameters struct */
        if ( strcasecmp(name, "alarm_sleep")==0)
        {
          parms->alarm_sleep = atoi( value);
        }
        else if ( strcasecmp(name, "default_snaplen")==0)
        {
          parms->default_snaplen = atoi( value);
        }
        else if ( strcasecmp(name, "default_device")==0)
        {
          strncpy (parms->default_device, value, MAXLEN);
        }
        else if ( strcasecmp(name, "mongo_db_client")==0)
        {
          strncpy (parms->mongo_db_client, value, MAXLEN);
        }
        else if ( strcasecmp(name, "database")==0)
        {
          strncpy (parms->database, value, MAXLEN);
        }
        else if ( strcasecmp(name, "db_collection")==0)
        {
          strncpy (parms->db_collection, value, MAXLEN);
        }
        else if ( strcasecmp(name, "seed")==0)
        {
          parms->seed = atoi( value);
        }
        else if( strcasecmp(name, "strata_file")==0)
        {
          strncpy (parms->strata_file, value, MAXLEN);
        }
        else if( strcasecmp(name, "prefix_file")==0)
        {
          strncpy (parms->prefix_file, value, MAXLEN);
        }
        else
        {  printf ("WARNING: %s/%s: Unknown name/value pair!\n",
           name, value);
        }
  }

  fclose (fp);
}

//==========================================================//
//===== Export Databricks and Boyer Moore Output to DB =====//
//==========================================================//

void
export_to_db (unsigned int *databrick_r, /*unsigned int *major_flags_r,*/
	      flow_t * cand_r)
{
  /*Transmit databrick to MongoDB - centralized monitoring station */
  mongoc_client_t *client;
  mongoc_collection_t *collection;
  bson_error_t error;
  bson_oid_t oid;
  bson_t *doc;
  bson_t *child;
  char key[10];
  int num;
  bson_t *child_utf8;
  char tooltip_buffer[40];
  flow_t hitter = {0,0,0,0};
  u_int32_t hitter_src = 0;
  u_int32_t hitter_dst = 0;
  struct in_addr in_src;
  struct in_addr in_dst;
  char hitter_src_str[20];
  char hitter_dst_str[20];

  mongoc_init ();
  client = mongoc_client_new (parms.mongo_db_client);	/* sending to database */
  collection = mongoc_client_get_collection (client,parms.database, parms.db_collection);
  if (!collection)
    {
      fprintf (stderr,
	       "mongoc_client_get_collection FAILED - skipping and will try again later.\n");
      return;
    }

  doc = bson_new ();
  child = bson_new ();
  bson_oid_init (&oid, NULL);
  BSON_APPEND_OID (doc, "_id", &oid);
  bson_append_date_time (doc, "timestamp", -1, (long) (time (NULL) * 1000));
  bson_append_array_begin (doc, "data", -1, child);
  for (int i = 0; i < BRICK_DIMENSION * BRICK_DIMENSION; i++)
    {
      num = sprintf (key, "%d", i);
      if (num >= 10 - 1 || num < 0)
	{
	  fprintf (stderr, "Buffer Overflow. FATAL ERROR.\n");
	  exit (-1);
	}
      bson_append_int64 (child, key, -1, databrick_r[i]);
    }

  bson_append_array_end (doc, child);
  child_utf8 = bson_new ();
  bson_append_array_begin (doc, "hitters", -1, child_utf8);

  for (int i = 0; i < BRICK_DIMENSION * BRICK_DIMENSION; i++)
    {
      sprintf (key, "%d", i);
      if (/*major_flags_r[i] > 0 &&*/ cand_r[i].src > 0)
	{

	  hitter = cand_r[i];	// hitter here is u_int64_t
	  hitter_src = hitter.src;
	  hitter_dst = hitter.dst;
	  in_src.s_addr = htonl (hitter_src);
	  in_dst.s_addr = htonl (hitter_dst);
          num = sprintf (hitter_src_str, "%s:%d", inet_ntoa (in_src),hitter.sport);
	  if (num >= 26 - 1 || num < 0)
	    {			// 26 is the length of our char buffer
	      fprintf (stderr, "Buffer Overflow. FATAL ERROR.\n");
	      exit (-1);
	    }
	  num = sprintf (hitter_dst_str, "%s:%d", inet_ntoa (in_dst),hitter.dport);
	  if (num >= 26 - 1 || num < 0)
	    {			// 26 is the length of our char buffer
	      fprintf (stderr, "Buffer Overflow. FATAL ERROR.\n");
	      exit (-1);
	    }
	  num =
	    sprintf (tooltip_buffer, "[%s, %s]", hitter_src_str,
		     hitter_dst_str);
	  if (num >= 52 - 1 || num < 0)
	    {			// 52 is the length of our char tooltip_buffer
	      fprintf (stderr, "Buffer Overflow. FATAL ERROR.\n");
	      exit (-1);
	    }
	  bson_append_utf8 (child_utf8, key, -1, tooltip_buffer, -1);
	}
      else
	bson_append_utf8 (child_utf8, key, -1, "", -1);
    }
  bson_append_array_end (doc, child_utf8);

  if (!mongoc_collection_insert
      (collection, MONGOC_INSERT_NONE, doc, NULL, &error))
    {
      fprintf (stderr, "%s\n", error.message);
      bson_destroy (doc);
      bson_destroy (child);
      bson_destroy (child_utf8);
      mongoc_collection_destroy (collection);
      mongoc_client_destroy (client);
      return;
    }

  bson_destroy (doc);
  bson_destroy (child);
  bson_destroy (child_utf8);
  mongoc_collection_destroy (collection);
  mongoc_client_destroy (client);
}
/******************************************************************/

void
print_stats ()
{
  pfring_stat pfringStat;
  struct timeval endTime;
  double deltaMillisec;
  static u_int8_t print_all;
  static u_int64_t lastPkts = 0;
  static u_int64_t lastBytes = 0;
  u_int64_t diff, bytesDiff;
  static struct timeval lastTime;
  char buf1[64], buf2[64], buf3[64];
  unsigned long long nBytes = 0, nPkts = 0;
  double thpt;
  int i = 0;
  unsigned long long absolute_recv = 0, absolute_drop = 0;

  if (startTime.tv_sec == 0)
    {
      gettimeofday (&startTime, NULL);
      print_all = 0;
    }
  else
    print_all = 1;

  gettimeofday (&endTime, NULL);
  deltaMillisec = delta_time (&endTime, &startTime);

  for (i = 0; i < num_devs; i++)
    {
      if (pfring_stats (pd[i], &pfringStat) >= 0)
	{
	  absolute_recv += pfringStat.recv;
	  absolute_drop += pfringStat.drop;
	}
    }

  nBytes = numBytes;
  nPkts = numPkts;

  {
    thpt = ((double) 8 * nBytes) / (deltaMillisec * 1000);

    fprintf (stderr, "=========================\n"
	     "Absolute Stats: [%ld pkts rcvd][%ld pkts dropped]\n"
	     "Total Pkts=%ld/Dropped=%.1f %%\n",
	     (long) absolute_recv, (long) absolute_drop,
	     (long) (absolute_recv + absolute_drop),
	     absolute_recv == 0 ? 0 :
	     (double) (absolute_drop * 100) / (double) (absolute_recv +
							absolute_drop));
    fprintf (stderr, "%s pkts - %s bytes",
	     pfring_format_numbers ((double) nPkts, buf1, sizeof (buf1), 0),
	     pfring_format_numbers ((double) nBytes, buf2, sizeof (buf2), 0));

    if (print_all)
      fprintf (stderr, " [%s pkt/sec - %s Mbit/sec]\n",
	       pfring_format_numbers ((double) (nPkts * 1000) / deltaMillisec,
				      buf1, sizeof (buf1), 1),
	       pfring_format_numbers (thpt, buf2, sizeof (buf2), 1));
    else
      fprintf (stderr, "\n");

    if (print_all && (lastTime.tv_sec > 0))
      {
	deltaMillisec = delta_time (&endTime, &lastTime);
	diff = nPkts - lastPkts;
	bytesDiff = nBytes - lastBytes;
	bytesDiff /= (1000 * 1000 * 1000) / 8;

	fprintf (stderr, "=========================\n"
		 "Actual Stats: %llu pkts [%s ms][%s pps/%s Gbps]\n",
		 (long long unsigned int) diff,
		 pfring_format_numbers (deltaMillisec, buf1, sizeof (buf1),
					1),
		 pfring_format_numbers (((double) diff /
					 (double) (deltaMillisec / 1000)),
					buf2, sizeof (buf2), 1),
		 pfring_format_numbers (((double) bytesDiff /
					 (double) (deltaMillisec / 1000)),
					buf3, sizeof (buf3), 1));
      }

    lastPkts = nPkts, lastBytes = nBytes;
  }

  lastTime.tv_sec = endTime.tv_sec, lastTime.tv_usec = endTime.tv_usec;

  fprintf (stderr, "=========================\n\n");
}

/**********************************************************************/

void
sigproc (int sig)
{
  static int called = 0;
  int i = 0;

  fprintf (stderr, "Leaving...\n");
  if (called)
    return;
  else
    called = 1;
  do_shutdown = 1;
  print_stats ();

  for (i = 0; i < num_devs; i++)
    pfring_close (pd[i]);

  exit (0);
}

/***********************************************************************/

void
my_sigalarm (int sig)
{
  if (do_shutdown)
    return;

  print_stats ();
  alarm (parms.alarm_sleep);
  signal (SIGALRM, my_sigalarm);
}

/************************************************************************/

static char hex[] = "0123456789ABCDEF";

char *
etheraddr_string (const u_char * ep, char *buf)
{
  u_int i, j;
  char *cp;

  cp = buf;
  if ((j = *ep >> 4) != 0)
    *cp++ = hex[j];
  else
    *cp++ = '0';

  *cp++ = hex[*ep++ & 0xf];

  for (i = 5; (int) --i >= 0;)
    {
      *cp++ = ':';
      if ((j = *ep >> 4) != 0)
	*cp++ = hex[j];
      else
	*cp++ = '0';

      *cp++ = hex[*ep++ & 0xf];
    }

  *cp = '\0';
  return (buf);
}

//=========================================================//
//========= A faster replacement for inet_ntoa() ==========//
//=========================================================//
char *
_intoa (unsigned int addr, char *buf, u_short bufLen)
{
  char *cp, *retStr;
  u_int byte;
  int n;

  cp = &buf[bufLen];
  *--cp = '\0';

  n = 4;
  do
    {
      byte = addr & 0xff;
      *--cp = byte % 10 + '0';
      byte /= 10;
      if (byte > 0)
	{
	  *--cp = byte % 10 + '0';
	  byte /= 10;
	  if (byte > 0)
	    *--cp = byte + '0';
	}
      *--cp = '.';
      addr >>= 8;
    }
  while (--n > 0);

  /* Convert the string to lowercase */
  retStr = (char *) (cp + 1);

  return (retStr);
}

/*************************************************************/

char *
intoa (unsigned int addr)
{
  static char buf[sizeof "ff:ff:ff:ff:ff:ff:255.255.255.255"];

  return (_intoa (addr, buf, sizeof (buf)));
}

/*****************************************************************/
void
amonProcessing (struct pfring_pkthdr *h, const u_char * p)
{

  struct ether_header ehdr;
  u_short eth_type, vlan_id;
  struct ip ip;
  u_int16_t thin_s = 0, hash_j = 0;	    /* indices for the Boyer-Moore algorithm */
  int s_bucket = 0, d_bucket = 0;	    /* indices for the databrick */
  flow_t flow = {0,0,0,0};
  u_int32_t displ = 0;
  u_int32_t src_omega;		            /* The SRC IP in its integer representation */
  u_int32_t dst_omega;		            /* The DST IP in its integer representation */
  int error;
  unsigned int payload;
  u_int32_t IP_prefix = 0;

  memcpy (&ehdr, p, sizeof (struct ether_header));
  eth_type = ntohs (ehdr.ether_type);

  if (eth_type == 0x8100)
    {
      vlan_id = (p[14] & 15) * 256 + p[15];
      eth_type = (p[16]) * 256 + p[17];
      displ += 4;
      p += 4;
    }

  if (eth_type == 0x0800)
    {
      memcpy (&ip, p + sizeof (ehdr),sizeof (struct ip));        
      src_omega = ntohl (ip.ip_src.s_addr);
      dst_omega = ntohl (ip.ip_dst.s_addr);

      IP_prefix = src_omega >> 15;
      s_bucket = STRATA_IDX17_prefix_bin[IP_prefix];
      if (s_bucket == -1)
	  s_bucket = IDX17[Table16 (src_omega, T1_16, T2_16, T3_17)];

      IP_prefix = dst_omega >> 15;
      d_bucket = STRATA_IDX17_prefix_bin[IP_prefix];
      if (d_bucket == -1)
	  d_bucket = IDX17[Table16 (dst_omega, T1_16, T2_16, T3_17)];

      /* Populate sketch array! */
      thin_s = s_bucket * BRICK_DIMENSION + d_bucket;	        /* this is the sub-stream s */
      hash_j = Table16 (src_omega, T1_16, T2_16, T3_17) >> 9;	/* hashing entries of substream s even more */

      /* Entering Critical Section    */
      if ((error = pthread_mutex_lock (&critical_section_lock)))
	{
	  fprintf (stderr,
		   "Error Number %d For Acquiring Lock. FATAL ERROR. \n",
		   error);
	  exit (-1);
	}

      pfring_parse_pkt((u_char*)p, (struct pfring_pkthdr*)h, 4, 0, 0);
      if(h->extended_hdr.parsed_pkt.l4_src_port == 0 && h->extended_hdr.parsed_pkt.l4_dst_port == 0)
      {
        memset((void*)&h->extended_hdr.parsed_pkt, 0, sizeof(struct pkt_parsing_info));
        pfring_parse_pkt((u_char*)p, (struct pfring_pkthdr*)h, 4, 0, 0);
      }

	#if 0
      		printf ("[%s:%d ", intoa (ntohl (ip.ip_src.s_addr)),
              	h->extended_hdr.parsed_pkt.l4_src_port);
      		printf ("-> %s:%d] \n", intoa (ntohl (ip.ip_dst.s_addr)),
              	h->extended_hdr.parsed_pkt.l4_dst_port);
	#endif


       payload = (modality_type == 0) ? h->len : 1;
       databrick[s_bucket * BRICK_DIMENSION + d_bucket] += payload;	// add bytes

       flow.src = src_omega;
       flow.dst = dst_omega;
       flow.sport = h->extended_hdr.parsed_pkt.l4_src_port;
       flow.dport = h->extended_hdr.parsed_pkt.l4_dst_port;

      if (cand[thin_s].src == 0)
	{
	  cand[thin_s] = flow;
	  count[thin_s] = payload;
	  P_bm[thin_s * 256 + hash_j] = payload;
	}
      else
	{
	  if (cand[thin_s].src == flow.src && cand[thin_s].dst == flow.dst)
	    {
	      count[thin_s] += payload;
	      P_bm[thin_s * 256 + hash_j] += payload;
	    }
	  else
	    {
	      if (count[thin_s] > 0)
		{
		  count[thin_s] -= payload;
		  if (count[thin_s] < 0)
		    {
		      cand[thin_s] = flow;
		      count[thin_s] = -count[thin_s];	   /* reset candidate */
		      //major_flags[thin_s] = 0;
		    }
		  P_bm[thin_s * 256 + hash_j] += payload;
		}
	      else
		{
		  cand[thin_s] = flow;
		  count[thin_s] = payload;	           /* reset candidate */
		  //major_flags[thin_s] = 0;
		  P_bm[thin_s * 256 + hash_j] += payload;
		}
	    }
	}
      if ((error = pthread_mutex_unlock (&critical_section_lock)))
	{
	  fprintf (stderr,
		   "Error Number %d For Releasing Lock. FATAL ERROR. \n",
		   error);
	  exit (-1);
	}
      /* Exiting Critical Section    */
      asm volatile ("":::"memory");

    }

}

/***********************************************************************/
void *
reset_transmit (void *passed_params)
{
  int max;
  unsigned int P_est[BRICK_DIMENSION * BRICK_DIMENSION];
  int K = 20;
  int i = 0;
  int j = 0;
  int k = 0;
  int s = 0;
  int heapsize;
  //int majority_pop;
  int active_streams;
  flow_t hitter = {0,0,0,0}; 
  u_int32_t hitter_src = 0;
  u_int32_t hitter_dst = 0;
  struct in_addr in_src;
  struct in_addr in_dst;
  char hitter_src_str[20];
  char hitter_dst_str[20];
  int error;
  int num;
  char buf[64];
  // now handle card monitoring

  /* Initialize the heap and its hash table */
  for (s = 0; s < HEAPSIZE; s++)
  {
       pqueue[s].src = 0;
       pqueue[s].dst = 0;
       pqueue[s].sport = 0;
       pqueue[s].dport = 0;
    }

  init_hashtable (hashtab);

  while (1)
    {
      sleep (3);
      asm volatile ("":::"memory");
      printf ("=====================================================\n");
      fprintf (stderr, "Report from reset_transmit Thread\n");
      /*      Entering Critical Section     */
      if ((error = pthread_mutex_lock (&critical_section_lock)))
	{
	  fprintf (stderr,
		   "Error Number %d For Acquiring Lock. FATAL ERROR. \n",
		   error);
	  exit (-1);
	}

      P_bm_tmp = (unsigned int *) P_bm;
      P_bm = (unsigned int *) P_bm_r;
      P_bm_r = (unsigned int *) P_bm_tmp;

      cand_tmp = (flow_t *) cand;
      cand = (flow_t *) cand_r;
      cand_r = (flow_t *) cand_tmp;

      count_tmp = (long *) count;
      count = (long *) count_r;
      count_r = (long *) count_tmp;

      /*major_flags_tmp = (unsigned int *) major_flags;
      major_flags = (unsigned int *) major_flags_r;
      major_flags_r = (unsigned int *) major_flags_tmp;*/

      databrick_tmp = (unsigned int *) databrick;
      databrick = (unsigned int *) databrick_r;
      databrick_r = (unsigned int *) databrick_tmp;

      if ((error = pthread_mutex_unlock (&critical_section_lock)))
	{
	  fprintf (stderr,
		   "Error Number %d For Releasing Lock. FATAL ERROR. \n",
		   error);
	  exit (-1);
	}			/*      Exiting Critical Section     */


      /* Find the culprit */
      for (s = 0; s < BRICK_DIMENSION * BRICK_DIMENSION; s++)
	{
	  max = 0;
	  for (j = 0; j < 256; j++)
	    {
	      if (P_bm_r[s * 256 + j] > max)
		max = P_bm_r[s * 256 + j];
	    }
	  P_est[s] = max;
	}


      /* Insert all candidates and their values into our heap (pqueue) */
      heapsize = 0;
      active_streams = 0;
      //majority_pop = 0;
      for (i = 0; i < BRICK_DIMENSION * BRICK_DIMENSION; i++)
	{
	  if (cand_r[i].src > 0)
	    {
	      active_streams++;
	      max_heap_insert (pqueue, cand_r[i], P_est[i], &heapsize);
	      //if (major_flags_r[i] > 0)
		//majority_pop++;
	    }
	}

      /*fprintf (stderr,
	       "A fraction of [%.2f] of substreams includes a majority element\n",
	       (float) majority_pop / active_streams);*/
      /* Display the top-K heavy-hitters */
      for (k = 0; k < (active_streams > K ? K : active_streams); k++)
	{
	  hitter = heap_extract_max (pqueue, &heapsize);	// hitter here is u_int64_t 
	  if (hitter.src == 0)
	    {
	      fprintf (stderr, "We got a zero candidate key. FATAL ERROR.\n");
	      exit (-1);
	    }
	  hitter_src = hitter.src;
	  hitter_dst = hitter.dst;
	  in_src.s_addr = htonl (hitter_src);
	  in_dst.s_addr = htonl (hitter_dst);
	  num = sprintf (hitter_src_str, "%s", inet_ntoa (in_src));
	  if (num >= 20 - 1 || num < 0)
	    {			// 20 is the length of our char buffer
	      fprintf (stderr, "Buffer Overflow. FATAL ERROR.\n");
	      exit (-1);
	    }
	  num = sprintf (hitter_dst_str, "%s", inet_ntoa (in_dst));
	  if (num >= 20 - 1 || num < 0)
	    {			// 20 is the length of our char buffer
	      fprintf (stderr, "Buffer Overflow. FATAL ERROR.\n");
	      exit (-1);
	    }
	  if (modality_type == 0)
	      fprintf (stderr, "Hitter Flow #%d: [%s] -> [%s]: %s bytes.\n\n",
		     k + 1, hitter_src_str, hitter_dst_str,
		     pfring_format_numbers ((double) hashtab_read (hitter),
					    buf, sizeof (buf), 0));
	  else
	      fprintf (stderr, "Hitter Flow #%d: [%s] -> [%s]: %s packets.\n\n",
		     k + 1, hitter_src_str, hitter_dst_str,
		     pfring_format_numbers ((double) hashtab_read (hitter),
					    buf, sizeof (buf), 0));
	}

      /* Transmit databrick to MongoDB - centralized monitoring station */
      export_to_db (databrick_r,/* major_flags_r,*/ cand_r);
      /* End of mongodb part - this should go into a function */

      /* Reset pqueue and its hash table and get ready for new iteration */
      for (s = 0; s < HEAPSIZE; s++)
      {
       pqueue[s].src = 0;
       pqueue[s].dst = 0;
       pqueue[s].sport = 0;
       pqueue[s].dport = 0;
      }
      free_hashtable (hashtab);
      init_hashtable (hashtab);

      memset ((unsigned int *) P_bm_r, 0,
	      (BRICK_DIMENSION * BRICK_DIMENSION * 256) *
	      sizeof (unsigned int));
      memset ((flow_t *) cand_r, 0,
	      BRICK_DIMENSION * BRICK_DIMENSION * sizeof (flow_t));
      memset ((long *) count_r, 0,
	      BRICK_DIMENSION * BRICK_DIMENSION * sizeof (long));
      /*memset ((unsigned int *) major_flags_r, 1,
	      BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int));*/
      memset ((unsigned int *) databrick_r, 0,
	      (BRICK_DIMENSION * BRICK_DIMENSION) * sizeof (unsigned int));
      printf ("=====================================================\n");
    }

  pthread_exit (NULL);
}

/*************************************************************************/

#ifdef VERBOSE_SUPPORT
static int32_t thiszone;

#endif

/***************************************************************************/

void
printHelp (void)
{
  printf ("amon\n(C) 2015  Merit Network, Inc.\n\n");
  printf ("-h              Print this help\n");
  printf
    ("-r <inputfile>  Input PCAP file; besides -f option, all other options ignored\n");
  printf ("-i <devices>    Comma-separated list of devices: ethX,ethY\n");
  printf ("-l <len>        Capture length\n");
  printf ("-g <core_id>    Bind to a core\n");
  printf ("-f <filter>     [BPF filter]\n");
  printf ("-p <poll wait>  Poll wait (msec)\n");
  printf ("-b <cpu %%>      CPU pergentage priority (0-99)\n");
  printf ("-s              Use poll instead of active wait\n");
#ifdef VERBOSE_SUPPORT
  printf ("-v              Verbose\n");
#endif
}

/***************************************************************************/

inline int
bundlePoll ()
{
  int i;

  for (i = 0; i < num_devs; i++)
    {
      pfring_sync_indexes_with_kernel (pd[i]);
      pfd[i].events = POLLIN;
      pfd[i].revents = 0;
    }
  errno = 0;

  return poll (pfd, num_devs, poll_duration);
}

/****************************************************************************/

void
packetConsumer ()
{
  u_char *buffer;
  struct pfring_pkthdr hdr;
  memset (&hdr, 0, sizeof (hdr));
  int next = 0, hunger = 0;



  while (!do_shutdown)
    {

      if (pfring_is_pkt_available (pd[next]))
	{
	  if (pfring_recv
	      (pd[next], &buffer, 0, &hdr, 0 /* wait_for_packet */ ) > 0)
	    {
	      amonProcessing (&hdr, buffer);
	      numPkts++;
	      numBytes += hdr.len + 24 /* 8 Preamble + 4 CRC + 12 IFG */ ;
	    }

	  hunger = 0;
	}
      else
	hunger++;

      if (wait_for_packet && hunger >= num_devs)
	{
	  bundlePoll ();
	  hunger = 0;
	}

      next = (next + 1) % num_devs;
    }
}

/***********************************************************************/

int
main (int argc, char *argv[])
{
  parse_config (&parms);                /* Read config file */

  char *devices = NULL, *dev = NULL, *tmp = NULL;
  char c, buf[32];
  u_char mac_address[6] = { 0 };
  int snaplen = parms.default_snaplen, rc;
  int bind_core = -1;
  u_int16_t cpu_percentage = 0;
  u_int32_t version;
  u_int32_t flags = 0;
  int i = 0;
  pthread_t thread_id;
  int retstatus;
  char *bpfFilter = NULL;
  char *pcap_in = NULL;
  struct bpf_program fcode;

  startTime.tv_sec = 0;
#ifdef VERBOSE_SUPPORT
  thiszone = gmt_to_local (0);
#endif

  while ((c = getopt (argc, argv, "hi:l:vsw:p:b:g:f:r:")) != '?')
    {
      if ((c == 255) || (c == -1))
	break;

      switch (c)
	{
	case 'h':
	  printHelp ();
	  return (0);
	  break;
	case 'r':
	  pcap_in = strdup (optarg);
	  break;
	case 's':
	  wait_for_packet = 1;
	  break;
	case 'l':
	  snaplen = atoi (optarg);
	  break;
	case 'i':
	  devices = strdup (optarg);
	  break;
	case 'f':
	  bpfFilter = strdup (optarg);
	  break;
#ifdef VERBOSE_SUPPORT
	case 'v':
	  verbose = 1;
	  break;
#endif
	case 'b':
	  cpu_percentage = atoi (optarg);
	  break;
	case 'p':
	  poll_duration = atoi (optarg);
	  break;
	case 'g':
	  bind_core = atoi (optarg);
	  break;
	}
    }


  if (devices == NULL)
    devices = strdup (parms.default_device);

  srand (parms.seed);
  init_tables16 (T1_16, T2_16, T3_17);

  int reserved = 0;
  int STRATA_index = 0;
  u_int32_t IP_prefix = 0;
  char line[80];
  init_STRATA_IDX17 (STRATA_IDX17_prefix_bin);

  FILE *fid;
  fid = fopen (parms.strata_file, "rt");
  if (fid != NULL)
    {
      //
      // It is assumed that Strata.txt has lines with the format:
      //  IP_prefix Index
      // where the IP_prefix is an IP number and only prefix of the first 17 bits are
      // considered.  Index is a number from 1 to k, denoting the bin to stratify the prefix to.
      // All numbers from 1 through k should appear in the file but need not be in any particular order.
      //  The variable reserved is assigned as the maximum index.
      //
      printf ("\n Reading Strata.txt ...");
      while (fgets (line, 80, fid) != NULL)
	{
	  sscanf (line, "%u %d", &IP_prefix, &STRATA_index);
	  if (STRATA_index > reserved)
	    {
	      reserved = STRATA_index;
	    }
	  update_STRATA_IDX17 (STRATA_IDX17_prefix_bin,
			       (u_int32_t) IP_prefix >> 15, STRATA_index);
	}
      fclose (fid);
    }
  reserved += 1;		// +1 because we have C-based indexing starting from 0
  printf ("\n Reserved bins = %d \n", reserved);
  init_IDX17 (IDX17, reserved);

  /* Initialize AMON variables and structs */
  /* Define the pointers to the sketch arrays */
  unsigned int *mem =
    malloc ((BRICK_DIMENSION * BRICK_DIMENSION * 256) *
	    sizeof (unsigned int) + 7);
  P_bm = (unsigned int *) (((uintptr_t) mem + 7) & ~(uintptr_t) 0x07);
  memset ((unsigned int *) P_bm, 0,
	  (BRICK_DIMENSION * BRICK_DIMENSION * 256) * sizeof (unsigned int));

  unsigned int *mem2 =
    malloc ((BRICK_DIMENSION * BRICK_DIMENSION * 256) *
	    sizeof (unsigned int) + 7);
  P_bm_r = (unsigned int *) (((uintptr_t) mem2 + 7) & ~(uintptr_t) 0x07);
  memset ((unsigned int *) P_bm_r, 0,
	  (BRICK_DIMENSION * BRICK_DIMENSION * 256) * sizeof (unsigned int));
  P_bm_tmp = (unsigned int *) P_bm;

  /* Initialize array of candidate keys */
  u_int64_t *mem3 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (flow_t) + 7);
  cand = (flow_t *) (((uintptr_t) mem3 + 7) & ~(uintptr_t) 0x07);
  memset ((flow_t *) cand, 0,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (flow_t));

  u_int64_t *mem4 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (flow_t) + 7);
  cand_r = (flow_t *) (((uintptr_t) mem4 + 7) & ~(uintptr_t) 0x07);
  memset ((flow_t *) cand_r, 0,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (flow_t));

  /* Initialize array of counters */
  long *mem5 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (long) + 7);
  count = (long *) (((uintptr_t) mem5 + 7) & ~(uintptr_t) 0x07);
  memset ((long *) count, 0,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (long));

  long *mem6 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (long) + 7);
  count_r = (long *) (((uintptr_t) mem6 + 7) & ~(uintptr_t) 0x07);
  memset ((long *) count_r, 0,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (long));

  /* Initialize majority flags */
/*  unsigned int *mem7 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int) + 7);
  major_flags = (unsigned int *) (((uintptr_t) mem7 + 7) & ~(uintptr_t) 0x07);
  memset ((unsigned int *) major_flags, 1,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int));

  unsigned int *mem8 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int) + 7);
  major_flags_r =
    (unsigned int *) (((uintptr_t) mem8 + 7) & ~(uintptr_t) 0x07);
  memset ((unsigned int *) major_flags_r, 1,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int));
*/
  /* Initialize arrays for databricks */
  unsigned int *mem9 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int) + 7);
  databrick = (unsigned int *) (((uintptr_t) mem9 + 7) & ~(uintptr_t) 0x07);
  memset ((unsigned int *) databrick, 0,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int));

  unsigned int *mem10 =
    malloc (BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int) + 7);
  databrick_r =
    (unsigned int *) (((uintptr_t) mem10 + 7) & ~(uintptr_t) 0x07);
  memset ((unsigned int *) databrick_r, 0,
	  BRICK_DIMENSION * BRICK_DIMENSION * sizeof (unsigned int));

  /* libpcap functionality follows */
  if (pcap_in)
    {
      char ebuf[256];
      u_char *p;
      struct pcap_pkthdr *h;
      pcap_t *pt = pcap_open_offline (pcap_in, ebuf);
      unsigned long long num_pcap_pkts = 0;
      struct timeval beginning = { 0, 0 };
      struct pfring_pkthdr hdr;
      memset (&hdr, 0, sizeof (hdr));

      if (pt)
	{
	  int datalink = pcap_datalink (pt);

	  if (datalink != DLT_EN10MB)
	    printf ("WARNING [pcap] Datalink not DLT_EN10MB (Ethernet).\n");

	  if (bpfFilter != NULL)
	    {
	      if (pcap_compile (pt, &fcode, bpfFilter, 1, 0xFFFFFF00) < 0)
		{
		  printf ("pcap_compile error: '%s'\n", pcap_geterr (pt));
		}
	      else
		{
		  if (pcap_setfilter (pt, &fcode) < 0)
		    {
		      printf ("pcap_setfilter error: '%s'\n",
			      pcap_geterr (pt));
		    }
		}
	    }

	  /* Ready to start the reset and transmit helper thread */
	  int retstatus =
	    pthread_create (&thread_id, NULL, reset_transmit, NULL);
	  if (retstatus)
	    {
	      printf ("ERROR; return code from pthread_create() is %d\n",
		      retstatus);
	      exit (-1);
	    }

	  while (1)
	    {
	      int rc = pcap_next_ex (pt, &h, (const u_char **) &p);

	      if (rc <= 0)
		break;

	      if (num_pcap_pkts == 0)
		{
		  beginning.tv_sec = h->ts.tv_sec;
		  beginning.tv_usec = h->ts.tv_usec;
		  printf ("First packet seen at %ld\n",
			  beginning.tv_sec * 1L);
		}
	      num_pcap_pkts++;

	      memcpy (&hdr, h, sizeof (struct pcap_pkthdr));
	      amonProcessing (&hdr, p);
	    }
	}

      free(mem);
      free(mem2);
      free(mem3);
      free(mem4);
      free(mem5);
      free(mem6);
      //free(mem7);
      //free(mem8);
      free(mem9);
      free(mem10);

      return 0;			// Exit program
    }

  /* PF_RING functionality follows */
  bind2node (bind_core);

  if (wait_for_packet && (cpu_percentage > 0))
    {
      if (cpu_percentage > 99)
	cpu_percentage = 99;
      pfring_config (cpu_percentage);
    }

  dev = strtok_r (devices, ",", &tmp);
  while (i < MAX_NUM_DEVS && dev != NULL)
    {
      flags |= PF_RING_PROMISC;
      flags |= PF_RING_DNA_SYMMETRIC_RSS;	/* Note that symmetric RSS is ignored by non-DNA drivers */
#if 0				/* TODO This will be needed when we require the port info */
      flags |= PF_RING_LONG_HEADER;
#endif
      pd[i] = pfring_open (dev, snaplen, flags);

      if (pd[i] == NULL)
	{
	  fprintf (stderr,
		   "pfring_open error [%s] (pf_ring not loaded or perhaps you use quick mode and have already a socket bound to %s ?)\n",
		   strerror (errno), dev);
	  return (-1);
	}

      if (i == 0)
	{
	  pfring_version (pd[i], &version);

	  printf ("Using PF_RING v.%d.%d.%d\n",
		  (version & 0xFFFF0000) >> 16,
		  (version & 0x0000FF00) >> 8, version & 0x000000FF);
	}

      pfring_set_application_name (pd[i], "amon");

      printf ("Capturing from %s", dev);
      if (pfring_get_bound_device_address (pd[i], mac_address) == 0)
	printf (" [%s]\n", etheraddr_string (mac_address, buf));
      else
	printf ("\n");

      printf ("# Device RX channels: %d\n",
	      pfring_get_num_rx_channels (pd[i]));

      if (bpfFilter != NULL)
	{
	  rc = pfring_set_bpf_filter (pd[i], bpfFilter);
	  if (rc != 0)
	    printf ("pfring_set_bpf_filter(%s) returned %d\n", bpfFilter, rc);
	  else
	    printf ("Successfully set BPF filter '%s'\n", bpfFilter);
	}
	
      if ((rc = pfring_set_socket_mode (pd[i], recv_only_mode)) != 0)
	fprintf (stderr, "pfring_set_socket_mode returned [rc=%d]\n", rc);

      pfd[i].fd = pfring_get_selectable_fd (pd[i]);

      pfring_enable_ring (pd[i]);

      dev = strtok_r (NULL, ",", &tmp);
      i++;
    }
  num_devs = i;

  signal (SIGINT, sigproc);
  signal (SIGTERM, sigproc);
  signal (SIGINT, sigproc);

#ifdef VERBOSE_SUPPORT
  if (!verbose)
    {
#endif
      signal (SIGALRM, my_sigalarm);
      alarm (parms.alarm_sleep);
#ifdef VERBOSE_SUPPORT
    }
#endif

  if (bind_core >= 0)
    bind2core (bind_core);

  /* Ready to start the reset and transmit helper thread */
  retstatus = pthread_create (&thread_id, NULL, reset_transmit, NULL);
  if (retstatus)
    {
      printf ("ERROR; return code from pthread_create() is %d\n", retstatus);
      exit (-1);
    }

  packetConsumer ();

  alarm (0);
  sleep (1);

  for (i = 0; i < num_devs; i++)
    pfring_close (pd[i]);

  free(mem);
  free(mem2);
  free(mem3);
  free(mem4);
  free(mem5);
  free(mem6);
  //free(mem7);
  //free(mem8);
  free(mem9);
  free(mem10);

  return (0);
}
