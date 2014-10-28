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
int             sequence = 0;
int             member_count = 0;

membership_info memb_info;
FILE            *file;


/** Method Declarations **/
static void Usage(int argc, char *argv[]);
static void Read_message();
static void Send_messages();
void Write_message(message m);

int main( int argc, char *argv[] )
{
	/** Local Variables **/
	sp_time test_timeout;

	/** Seed random number generator for future use **/
	srand(time(NULL));

	/** Open file for writing **/
	char filename[NAME_LENGTH];
	sprintf(filename, "%d", machine_index);
	strcat(filename, ".out");
	if((file = fopen(filename, "w")) == NULL) {
		perror("fopen");
		exit(0);
	}

	Usage( argc, argv );

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
	while(member_count < num_machines)
	{
		Read_message();
	}

	/** Processes send bursts and receive packets by dealing with events **/
	E_init();

	E_attach_fd( 0, READ_FD, Send_message_burst, 0, NULL, LOW_PRIORITY);

	E_attach_fd( Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY);

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

static void Read_message()
{
	/** Initialize locals **/
	int     service_type;
	char    sender[MAX_GROUP_NAME];
	int     num_groups;
	char    target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	int16   mess_type;
	int     endian_mismatch;
	char    mess[MAX_MESSLEN];
	message received_mess;

	/** Receive a message **/
	ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, 
		target_groups, &mess_type, &endian_mismatch, sizeof(mess), mess );

	/** Check if it is a membership message**/
	if( Is_membership_mess( service_type ) )
	{
		/** Get the new membership informations **/
		ret = SP_get_memb_info( mess, service_type, &memb_info );
		if( ret < 0 )
		{
			SP_error( ret );
			exit( 1 );
		}

		printf("\ngrp id is %d %d %d\n",memb_info.gid.id[0], 
			memb_info.gid.id[1], memb_info.gid.id[2] ); 
	
	/** Check if it is a regular message**/
	}else if( Is_regular_mess( service_type ) )
	{
		mess = *((message *) mess);
		if(mess.message_index > 0)
		{
			//Write_message(mess);
		}else{
			//The process which sent this message is finished sending
			//Check if all processes are finished sending
				//If true, terminate = 1
		}
	}
}

static void Send_message_burst()
{
	message mess;

	for(int i = 0; i < BURST_SIZE; i++)
	{
		/** Create new packet **/
		mess.message_index = sequence++;
		mess.process_index = machine_index;
		mess.random_number = rand();
		
		/** Write the message locally **/
		Write_message(mess);
		
		/** Send the message to the other processes **/
		ret = SP_multicast( Mbox, AGREED_MESS, group, 1, MAX_MESSLEN, 
			(char *) &mess);
	}
}

void Write_message(message m)
{
	/** Write the message to the file **/	
	fprintf(file, "%2d, %8d, %8d\n", m.process_index, m.message_index,
		m.random_number);
}
