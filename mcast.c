/*
 * John An
 * Edward Schembor
 * Distributed Systems
 * Spread Multicast File Transfer Program
 */

#include "sp.h"
#include "message.h"

#include <sys/types.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MEMBERS 100
#define MAX_MESSLEN  1212
#define NAME_LENGTH 80
#define BURST_SIZE 100

/** Global Variables **/
static int      num_messages;
static int      machine_index;
static int      num_machines;

static char     Spread_name[80];
static char     User[80];
static mailbox  Mbox;
static char     Private_group[MAX_GROUP_NAME];
static char     group[80];
int             ret;
int             num_groups = 0;
int             num_sent = 0;
int             connected = 0;

membership_info memb_info;
FILE            *file;

char            *all_machines; //temporary measure to terminate

/** Method Declarations **/
static void Usage(int argc, char *argv[]);
static void Handle_messages();
static void Send_message_burst();
void Write_message(message m);

int main( int argc, char *argv[] )
{
	/** Local Variables **/
	sp_time test_timeout;

	/** Seed random number generator for future use **/
	srand(time(NULL));

	/** Check usage **/
	Usage(argc, argv);

	/** Open file for writing **/
	char filename[NAME_LENGTH];
	sprintf(filename, "%d", machine_index);
	strcat(filename, ".out");
	if((file = fopen(filename, "w")) == NULL) {
		perror("fopen");
		exit(0);
	}

	/** Set up array for ending logic **/
    all_machines = malloc(num_machines);

	/** Set up timeout for Spread connection **/
	test_timeout.sec = 5;
	test_timeout.usec = 0;

	/** Connect to the Spread client **/
	ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout);
	if( ret != ACCEPT_SESSION )
	{
		SP_error( ret );
		exit(0);
	}

	/** Join a Group **/
	ret = SP_join( Mbox, group );
	if( ret < 0 ) SP_error( ret );

	/** Wait for all Expected Processes to Join the Group**/
	while(num_groups < num_machines)
	{
		Handle_messages();
	}

	connected = 1;

	if(machine_index == 1) {
		Send_message_burst();
	}

	printf("\nRECEIVED ALL MEMBERS\n");

	/** Processes send bursts and receive packets by dealing with events **/
	
    E_init();

	E_attach_fd( Mbox, READ_FD, Handle_messages, 0, NULL, HIGH_PRIORITY);

	printf("\nAbout to handle events\n");
	E_handle_events();
    
	return 1;

}

static void Usage(int argc, char *argv[])
{
	if(argc != 4) {
		printf("Usage: mcast <num_messages> <machine_index> <num_machines>\n");
		exit(0);
	}

	num_messages = atoi(argv[1]);
	machine_index = atoi(argv[2]);
	num_machines = atoi(argv[3]);

	if(num_machines > 10) {
		printf("Usage: Max machines: 10\n");
		exit(0);
	}

	sprintf( User, "user" );
	sprintf( Spread_name, "4803" );
	sprintf( group, "johned" );
}

static void Handle_messages()
{

	printf("\nREADING\n");

	/** Initialize locals **/
	int     service_type;
	char    sender[MAX_GROUP_NAME];
	char    target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	int16   mess_type;
	int     endian_mismatch;
	char    mess[MAX_MESSLEN];
	message received_mess;

	//Deals with intial receive calls
	if(connected) {
		Send_message_burst();
	}

	/** Receive a message **/
	ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, 
		target_groups, &mess_type, &endian_mismatch, sizeof(mess), mess );

	if( Is_regular_mess( service_type ) ) {
		received_mess = *((message *) mess);
		if(received_mess.message_index >= 0)
		{
			Write_message(received_mess);

		}else{
			//The process which sent this message is finished sending
			//Check if all processes are finished sending
				//If true, terminate = 1
			printf("ENDING CONDITIONAL------\n");
            /**FIXME ALL HACKS HERE **/
            all_machines[received_mess.process_index-1] = 1;
            int i = 0;
            while (all_machines[i] > 0)
                i++;
			printf("\ni: %d\n", i);
            if (i == num_machines) {
				fclose(file);
				E_exit_events();
                exit(0);
			}
            /**FIXME ALL HACKS HERE **/
		}
	}
}

static void Send_message_burst()
{

	printf("\nSENDING\n");

	message mess;

	for(int i = 0; i < BURST_SIZE; i++)
	{
        if (num_sent >= num_messages) {
			printf("\nDONE SENDING\n");
            message *end_mess = malloc(sizeof(message));
            end_mess->process_index = machine_index;
            end_mess->message_index = -1;

            SP_multicast( Mbox, AGREED_MESS, group, 1, MAX_MESSLEN,
                (char *) end_mess);
            break;
        } else {
            /** Create new packet **/
            mess.message_index = num_sent++;
            mess.process_index = machine_index;
            mess.random_number = rand();
			printf("\nnum sent: %d\n", num_sent);

            /** Send the message to the other processes **/
            SP_multicast( Mbox, AGREED_MESS, group, 1, MAX_MESSLEN, 
                (char *) &mess);
        }
	}

	if (num_sent >= num_messages) {
		printf("\nDONE SENDING\n");
        message *end_mess = malloc(sizeof(message));
        end_mess->process_index = machine_index;
    	end_mess->message_index = -1;

        SP_multicast( Mbox, AGREED_MESS, group, 1, MAX_MESSLEN,
            (char *) end_mess);
	}
}

void Write_message(message m)
{
	/** Write the message to the file **/	
	fprintf(file, "%2d, %8d, %8d\n", m.process_index, m.message_index,
		m.random_number);
}
