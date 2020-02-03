// NAME: Meiyi Zheng
// EMAIL: meiyizheng@g.ucla.edu
// ID: 605147145

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>

#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#ifdef DUMMY
#define	MRAA_GPIO_IN	0
typedef int mraa_aio_context;
typedef int mraa_gpio_context;
…
int mraa_aio_read(mraa_aio_context c)    {
	return 650;
}
void mraa_aio_close(mraa_aio_context c)  {
}
…
#else
#include <mraa.h>
#include <mraa/aio.h>
#endif


int period = 1;
int sflag=0;
int log_flag = 0;
FILE* log_file;
int report = 1;
int client_socket;

mraa_aio_context temp_sensor;

struct timespec curr;

char* host_name;
int port_num;

void get_current_time( struct timespec ts)
{
  if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
    fprintf(stderr,"Could not get the current time\n");
    exit(1);
  }
  //struct tm * tm;
  //tm = localtime(&(ts.tv_sec));
  //if (tm == NULL) {
  //fprintf(stderr,"Could not get the local time\n");
  // exit(1);
  //}
   
  //fprintf(stdout,“hour:%d, min:%d, second: %d\n”, tm->tm_hour, 
  //	  tm->tm_min, tm->tm_sec);
}
void shut_down() {
  char buffer[18];
  struct timespec final;
  clock_gettime(CLOCK_REALTIME, &final);
  struct tm * tm;
  tm = localtime(&(final.tv_sec));
  if (tm == NULL) {
    fprintf(stderr,"Could not get the local time\n");
    exit(1);
  }
  sprintf(buffer, "%02d:%02d:%02d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
  write(client_socket,buffer,sizeof(buffer));
  if (log_flag)
    fprintf(log_file, "%02d:%02d:%02d SHUTDOWN\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
  exit(0);  
}

void parse_command(char* command) {
  if (strcmp(command, "SCALE=F") == 0) {
    sflag=0;
  }
  else if (strcmp(command, "SCALE=C") == 0) {
    sflag=1;
  }
  else if (strncmp(command, "PERIOD=", 7) == 0) {
    period = atoi(command + 7);
  }

  else if (strcmp(command, "STOP") == 0) {
    report = 0;
  }

  else if (strcmp(command, "START") == 0) {
    report =  1;   
  }
  
  else if (strcmp(command, "OFF") == 0) {
    shut_down();
  }
  else if (strncmp(command, "LOG", 3) == 0) {
  }
  else {
    fprintf(stderr, "Invalid command\n");
    exit(1);
  }
  
}

float convert_temp()
{
  const int B = 4275;            
  const int R0 = 100000; 
  int reading = mraa_aio_read(temp_sensor);
  float R = 1023.0/((float) reading) - 1.0;
  R = R0*R;
  //C is the temperature in Celcious
 
  float C = 1.0/(log(R/R0)/B + 1/298.15) - 273.15;
  //F is the temperature in Fahrenheit

  
  return C;
}


int client_connect() {
  struct sockaddr_in serv_addr; //encode the ip address and the port for the remote
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Could not create socket\n");
    exit(2);
  }
    
  // AF_INET: IPv4, SOCK_STREAM: TCP connection
  struct hostent *server = gethostbyname(host_name);
  if (server == NULL) {
    fprintf(stderr, "Could not find the host by the name\n");
    exit(2);
  }
		    
  
  // convert host_name to IP addr  
  memset(&serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr.sin_family = AF_INET; //address is Ipv4
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length); 
  //copy ip address from server to serv_add
  serv_addr.sin_port = htons(port_num); //setup the port
 
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Could not connect to the server\n");
    exit(2);
  } //initiate the connection to server
  return sockfd; 
		 
}


int main(int argc, char * argv[]) {
  int opt;
  char id[13];
  
  static struct option long_options[] = {
    
     {"period", required_argument, NULL, 'p'},
     {"scale", required_argument,  NULL, 's'},
     {"log", required_argument, NULL, 'l'},
     {"id", required_argument, 0, 'i'},
     {"host", required_argument, 0, 'h'},
     {0,0,0,0},
  };

  while (1) {
    opt = getopt_long(argc,argv,"",long_options,NULL);
    
     if (opt == -1) 
       break;

     switch (opt) {
     case 'p':
       period = atoi(optarg);
       
       break;

     case 's':
       if (strcmp(optarg,"C") == 0)
	 sflag = 1;
       else if (strcmp(optarg,"F") != 0) {
	 fprintf(stderr, "Invalid argument: scale either is F or C\n");
	 exit(1);
       }
       break;

     case 'l':
       log_file = fopen(optarg, "w+");
       if (log_file == NULL) {
	 fprintf(stderr, "Could not open log file\n");
	 exit(1);
       }
       log_flag = 1;
      
       break;
     case 'i':
       sprintf(id, "ID=%s\n", optarg);
       
       break;

     case 'h':
       host_name = malloc((strlen(optarg)+1)*sizeof(char));
       strcpy(host_name,optarg);
       
       break;
       
     default:
       fprintf(stderr, "Usage: ./lab4c_tcp --period=seconds --scale=F/C --log=file_name --id=id_num --host=host_name port\n");
       exit(1);
     }
  }

  port_num = atoi(argv[optind]);
  client_socket = client_connect();

  write(client_socket,id,sizeof(id));

  if (log_flag)
    fprintf(log_file, "%s\n", id);
    

  temp_sensor = mraa_aio_init(1);
  if (temp_sensor == NULL) {
    fprintf(stderr, "Failed to initialize AIO \n");
  }
   

  struct timespec prev;
  clock_gettime(CLOCK_REALTIME, &prev);
  struct tm * tm;
  tm = localtime(&(prev.tv_sec));
  
  
  int size = 0;
  char* command = malloc(sizeof(char));
  if (command == NULL) {
    fprintf(stderr, "Can not assign space\n");
    exit(1);
  }
  
  while (1) {
    
    struct pollfd fds[1]; 
    fds[0].fd = client_socket;
    fds[0].events = POLLIN;

    int pret = poll(fds, 1, 0);
    if (pret == -1) {
      fprintf(stderr, "The poll function fails\n");
      exit(1);
    }

  
      
    if (pret > 0) { // command ready
      if (fds[0].revents & POLLIN) {
	char buf[1];
	int ret = read(client_socket, buf, sizeof(buf));
	if (ret < 0) {
	  fprintf(stderr, "Can not read from stdin\n");
	  exit(1);
	}

	// read till we meet a "\n"
	if (*buf == '\n') {
	  command[size] = '\0';
	  if (log_flag)
	    fprintf(log_file, "%s\n", command);
	  parse_command(command);
	  free(command);
	  command = malloc(sizeof(char));
	  size = 0;
     	  
	}
	else {
	  command[size] = *buf;
	  size++;
	  command = realloc(command,(size+1)*sizeof(char));
	  
	}
	
      }
    }

    
    //get_current_time(curr);
    struct timespec curr;
    clock_gettime(CLOCK_REALTIME, &curr);
    if (curr.tv_sec >= prev.tv_sec + period && report) {
      
      struct tm * tm2;
      tm2 = localtime(&(curr.tv_sec));
      float temp=convert_temp();
      char buf2[14];
      
      if (!sflag) {
	temp = temp * 9/5 + 32;
      }
      sprintf(buf2,"%02d:%02d:%02d %04.1f\n",tm2->tm_hour, tm2->tm_min, tm2->tm_sec,temp);
      
    
	
      write(client_socket,buf2,sizeof(buf2));
      
      
      clock_gettime(CLOCK_REALTIME, &prev);
      if (log_flag)
	fprintf(log_file,"%02d:%02d:%02d %.1f\n", tm->tm_hour,tm->tm_min, tm->tm_sec,temp);
      
    }
   
    
	    
  }
  
  mraa_aio_close(temp_sensor);
  fclose(log_file);
  free(host_name);
  exit(0);
}
