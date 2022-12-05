#include <arpa/inet.h>  //inet_aton, inet_ntoa, ...
#include <netinet/in.h>  //IPv4
#include <pthread.h> //Hilos
#include <semaphore.h> //Semaforos
#include <signal.h> //Señales
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> //Sockets
#include <unistd.h>

#include "protocol.h"
#include "split.h"

int finished;
/**
* @brief Manejador de señales para terminar
*
* @param sig
*/
void signal_handler(int sig);

int main(int argc, char * argv[]) {

	//Socket del servidor
	int s;

	char * local_ip = "127.0.0.1";
	char * ip;

	int port;

	//Manejador de señal SIGTERM
	struct sigaction act;

	//Validar argumentos por linea de comandos
	if (argc == 3) {
		ip = argv[1];
		port = atoi(argv[2]);
	}else {
		ip = local_ip;
		port = DEFAULT_PORT;
	}

	if (port < 1024) {
		port = DEFAULT_PORT;
	}

	// Limbiar la estructura del manejador
    memset(&act, 0, sizeof(struct sigaction));
	//Configurar e instalar el manejador de la señal
	act.sa_handler = signal_handler;
	// Interrupcion de teclado
    sigaction(SIGINT, & act, NULL);
    // Cerrar la terminal
    sigaction(SIGHUP, & act, NULL);
    // Apagar el computador
    sigaction(SIGTERM, & act, NULL);

	// Bandera para indicar la terminacion del programa
	finished = 0;

	//Direccion del servidor (IPv4)
	struct sockaddr_in addr;

	//Socket IPv4, de tipo flujo (stream)
	//1. Crear un socket
	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		perror("socket");
	}

	//Preparar la direccion para asociarla al socket
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_aton(ip,  &addr.sin_addr);

#ifdef DEBUG
	fprintf(stderr, "Connecting to %s, port=%d\n", ip, port);
#endif
	//2. Conectarse al servidor
	if (connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0) {
		perror("connect");
		exit(EXIT_FAILURE);
	}

	printf("Ingrese el comando.\n");

	char comando[BUFSIZ];
	request r;

	while (!finished) {

		memset(&r, 0, sizeof(request));

		//Leer una linea de entrada estandar

		printf(">");
		memset(comando, 0, BUFSIZ); //Rellenar con ceros el bufer

		//Leer maximo BUFSIZ - 1 caracteres de la entrada estandar
		if (fgets(comando, BUFSIZ, stdin) == NULL) {
			finished = 1;
			continue;
		}

		split_list * l;
		l = split(comando, " \n\r\t");
		if (l->count == 0) {
			continue;
		}

		if (strcmp(l->parts[0], "exit") == 0) {
			//leer el comando y partir
			//Enviar la solicitud al servidor
			strcpy(r.operation, "exit");

			//Enviar la solicitud al servidor
			if (write(s, &r, sizeof(request)) != sizeof(request)) {
				fprintf(stderr, "Error enviando la solicitud al servidor\n");
				continue;
			}
			finished = 1;
		}else if (
			strcmp(l->parts[0], "get") == 0
			&& l->count == 2
			) {
			strcpy(r.operation, l->parts[0]);
			strcpy(r.filename, l->parts[1]);
			//Enviar solicitud al servidor
			if (write(s, &r, sizeof(request)) != sizeof(request)) {
				fprintf(stderr, "Error enviando la solicitud al servidor\n");
				continue;
			}

			//Recibir informacion del archivo y el contenido
			//La funcion receive_file valida si el servidor
			//responde con un error si el archivo no existe
			if (!receive_file(s, "client_files")) {
				fprintf(stderr, "Error recibiendo %s\n", r.filename);
			}
		}
		else if (
			strcmp(l->parts[0], "put") == 0
			&& l->count == 2
			) {

			char ruta[PATH_MAX];

			//Validar si el archivo a enviar existe
			if (!file_exists(l->parts[1])) {
				fprintf(stderr, "El archivo a enviar %s no existe\n", ruta);
				continue;
			}

			//Enviar solicitud al servidor
			strcpy(r.operation, l->parts[0]);
			strcpy(r.filename, l->parts[1]);

			if (write(s, &r, sizeof(request)) != sizeof(request)) {
				fprintf(stderr, "Error enviando la solicitud al servidor\n");
				continue;
			}

#ifdef DEBUG
			//Simular una espera aleatoria para probar transferencias simultaneas
			sleep(rand() % 20);
#endif

			response resp;

			//Recibir la autorizacion del servidor
			if (read(s, &resp, sizeof(response)) != sizeof(response)) {
				fprintf(stdout, "Error recibiendo la respuesta del servidor\n");
				continue;
			}

			//Validar si existe otra transferencia activa con el mismo nombre de archivo
			if (resp.code == TRANSFER_DENIED) {
				fprintf(stderr, "%s\n", resp.message);
				continue;
			}

			//Enviar informacion del archivo y el contenido
			//La funcion valida si el archivo no existe,
			//En cuyo caso se envia size = -1

			if (!send_file(s, r.filename)) {
				fprintf(stderr, "Error enviando %s\n", r.filename);
			}
		}
	}

	close(s);

	exit(EXIT_SUCCESS);
}

void signal_handler(int sig) {
	if (sig == SIGINT)
    {
        printf("\nInterrupcion de teclado!\n");
    }else if (sig == SIGHUP)
    {
        printf("\nTerminal cerrada!\n");
    }else if (sig == SIGTERM)
    {
        printf("\nFuimonos!\n");
    }else {
        printf("\nSeñal %d recibida\n",sig);
    }
    
    // Informar que el servidor esta terminando
	finished = 1;
#ifdef DEBUG
	fprintf(stderr, "Signal received, finishing\n");
#endif
}
