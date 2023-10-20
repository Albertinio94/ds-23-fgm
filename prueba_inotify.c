// Prueba de captura de notificaciones de Linux
// Adaptado de http://www.thegeekstuff.com/2010/04/inotify-c-program-example/
// por Alberto Lafuente, Fac. Informática UPV/EHU

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <linux/inotify.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
// #define EVENT_SIZE  (3*sizeof(uint32_t)+sizeof(int))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))
#define TESTIGO_NAME "inotify.example.executing"

#define PORT 7777

#define UPLOAD '3'
#define DELETE '4'
#define EXIT '5'

void send_through_socket(int socket, char command, const char buffer[1024])
{
  char message[1024];
  message[0] = command;
  message[1] = '\0';
  strcat(message, buffer);
  send(socket, message, strlen(message), 0);
}

void listen_through_socket(int socket)
{
  char message[1024];
  char response_code[3];
  recv(socket, message, sizeof(message), 0);
  memcpy(response_code, message, 2);
  if (strncmp("ER", response_code, 2) == 0)
  {
    fprintf(stderr, "Se ha procudido un error ejecutando el comando. Respuesta: %s asdfasdf\n", message);
  }
}

unsigned char compare_strings(const char *s1, const char *s2)
{
  int min_length = strlen(s1) < strlen(s2) ? strlen(s1) : strlen(s2);
  unsigned char return_value = 0;
  for (int i = 0; i < min_length; i++)
  {
    if (s1[i] != s2[i])
    {
      return_value = 1;
      break;
    }
  }

  return return_value;
}

int is_regular_file(const char *path)
{
  struct stat path_stat;
  stat(path, &path_stat);

  return S_ISREG(path_stat.st_mode);
}

int main(int argc, char *argv[])
{
  int length, i = 0;
  int fd;
  int wd;
  int wd_cd;
  char buffer[EVENT_BUF_LEN];
  char testigo[1024];
  char evento[1024];
  char file_path[1024];
  char folder_path[1024];
  int clientSocket;
  struct sockaddr_in serverAddr;
  char socket_buffer[1024];

  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == -1)
  {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  // Configure server details
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(PORT);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if (argc != 2)
  {
    fprintf(stderr, "Uso: %s Directorio_a_monitorizar\n", argv[0]);
    exit(1);
  }

  if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
  {
    perror("Error connecting");
    close(clientSocket);
    exit(EXIT_FAILURE);
  }
  //  fprintf(stderr, "---Prueba de inotify sobre %s\n", argv[1]);
  //  fprintf(stderr, "---Notifica crear/borrar ficheros/directorios sobre %s\n", argv[1]);
  //  fprintf(stderr, "---%s debe exixtir!\n", argv[1]);

  /*creating the INOTIFY instance*/
  fd = inotify_init();

  /*checking for error*/
  if (fd < 0)
  {
    perror("inotify_init");
  }

  /*adding the /tmp directory into watch list. Here, the suggestion is to validate the existence of the directory before adding into monitoring list.*/
  //  wd = inotify_add_watch( fd, "/tmp", IN_CREATE | IN_DELETE );

  /* Monitorizamos el directorio indicado como argumento. Debe estar creado. */
  //  mkdir(argv[1]);
  wd_cd = inotify_add_watch(fd, argv[1], IN_CREATE | IN_CLOSE_WRITE | IN_DELETE);

  /* Testigo para finalizar cuando lo borremos: */

  strcpy(testigo, argv[1]);
  strcat(testigo, "/");
  strcat(testigo, TESTIGO_NAME);
  mkdir(testigo, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);
  fprintf(stderr, "---Para salir, borrar %s/%s\n", argv[1], testigo);
  realpath(argv[1], folder_path);

  /*read to determine the event change happens on the directory. Actually this read blocks until the change event occurs*/
  struct inotify_event event_st, *event;
  int k = 0;
  int exiting = 0;

  // printf("%s\n", argv[2]);
  // printf("%s\n", argv[3]);
  write(1, "1\n", 2);

  while (!exiting)
  {
    fprintf(stderr, "---%s: waiting for event %d...\n", argv[0], ++k);
    length = read(fd, buffer, EVENT_BUF_LEN);
    fprintf(stderr, "---%s: event %d read.\n", argv[0], k);
    /*checking for error*/
    if (length < 0)
    {
      perror("read");
      break;
    }
    //    struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
    while ((i < length) && !exiting)
    {
      //    event= &event_st;
      event = (struct inotify_event *)&buffer[i];
      //    fprintf(stderr, "---example: event name length: %i\n", event->len);
      //    memcpy(event, buffer, length);
      if (event->len)
      {
        sprintf(file_path, "%s/%s", folder_path, event->name);
        //      memcpy(event+EVENT_SIZE, buffer+EVENT_SIZE, length);
        if (event->mask & IN_CREATE && is_regular_file(file_path))
        {
          fprintf(stderr, "---%s: New file %s created.\n", argv[0], event->name);
          sprintf(evento, "3\n%s\n", event->name);
          write(1, evento, strlen(evento));
          //            printf("3\n%s\n", event->name );
          send_through_socket(clientSocket, UPLOAD, file_path);
          listen_through_socket(clientSocket);
        }
        else if (event->mask & IN_DELETE)
        {
          if (strcmp(event->name, TESTIGO_NAME) == 0)
          {
            exiting = 1;
          }
          else if (is_regular_file(file_path))
          {
            fprintf(stderr, "---%s: File %s deleted.\n", argv[0], event->name);
            sprintf(evento, "4\n%s\n", event->name);
            send_through_socket(clientSocket, DELETE, file_path);
            listen_through_socket(clientSocket);
            write(1, evento, strlen(evento));
          }
        }
        else if (event->mask & IN_CLOSE_WRITE && is_regular_file(file_path))
        {
          fprintf(stderr, "---%s: file %s updated.\n", argv[0], event->name);
          sprintf(evento, "3\n%s\n", event->name);
          send_through_socket(clientSocket, UPLOAD, file_path);
          listen_through_socket(clientSocket);
          write(1, evento, strlen(evento));
        }
      }
      else
      { // event ignored
        fprintf(stderr, "---%s: event ignored for %s\n", argv[0], event->name);
      }
      i += EVENT_SIZE + event->len;
      //    fprintf(stderr, "---example.event count: %i\n", i);
    }
    i = 0;
  }

  fprintf(stderr, "---Exiting %s\n", argv[0]);
  write(1, "5\n", 2);
  //  printf("5\n");
  /*removing the directory from the watch list.*/
  inotify_rm_watch(fd, wd);
  inotify_rm_watch(fd, wd_cd);
  //  rmdir(argv[1]);

  /*closing the INOTIFY instance*/
  send_through_socket(clientSocket, EXIT, "");
  listen_through_socket(clientSocket);
  close(fd);
  close(clientSocket);
}
