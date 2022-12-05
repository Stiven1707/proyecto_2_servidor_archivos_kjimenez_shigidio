#include <arpa/inet.h>  //IPv4
#include <netinet/in.h>  //IPv4
#include <pthread.h> //Hilos
#include <semaphore.h> //Semaforos
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> //Sockets
#include <unistd.h>

#include "protocol.h"

/**
 * @brief Cliente
 */
typedef struct {
	int socket; /**< Descriptor del socket */
	int index; /**< Indice dentro del arreglo de clientes*/
	struct sockaddr_in address; /**< Direccion del cliente */
	char ip[INET_ADDRSTRLEN]; /** < Direccion IP en formato aaa.bbb.ccc.ddd **/
	char active_transfer[NAME_MAX]; /** Archivo en transferencia */
}client;

/** Redefinicion de tipo para semaforo */
typedef sem_t semaphore;


/**
 * @brief Hilo de cliente.
 *
 * @param arg Apuntador al arreglo de conectores de cliente
 *
 * @return NULL
 */
void * client_thread(void * arg);


/**
 * @brief Adiciona un socket de cliente al arreglo de clientes
 *
 * @param c Socket de conexion del cliente
 * @param addr Referencia a la direccion del cliente
 *
 * @return Apuntador a un nuevo cliente
 */
client * add_client(int c, struct sockaddr_in * addr);

/**
 * @brief Elimina un cliente
 *
 * @param c Apuntador al cliente
 */
void remove_client(client * c);

/**
* @brief Manejador de señales para terminar
*
* @param sig
*/
void signal_handler(int sig);

/**
* @brief Intenta registrar una nueva transferencia
*
* @param c Cliente
* @param filename Nombre del archivo
* @param message Mensaje de retorno
*
* @return TRANSFER_AUTHORIZED | TRANFER_DENIED
*/
response_code register_transfer(client * c, char * filename, char * message);


/**
* @brief Marca la transferencia como finalizada
* 
* @param client Cliente que finaliza la transferencia
*/
void finalize_transfer(client * c);

/**
* @brief Funcion auxiliar para bloquear un semaforo
*
* @param s Semaforo
*/
void down(semaphore * s) {
	sem_wait(s);
}

/**
* @brief Funcion auxiliar para desbloquear un semaforo
*
* @param s Semaforo
*/
void up(semaphore * s) {
	sem_post(s);
}

client * clients; /**< Arreglo de clientes */
int nclients; /**< Cantidad actual de clientes */
int maxclients; /**< Capacidad del arreglo de clientes */
semaphore mutex; /**< Exclusion mutua para la gestion de los clientes y transferencias */
int finished; /**< Bandera para indicar el fin del programa */

int main(int argc, char * argv[]) {

	//Socket del servidor
	int s;
	//Socket del cliente
	int c;

	// Puerto
	int port;

	//Manejador de señal SIGTERM
	struct sigaction act;

	//Direccion del servidor (IPv4)
	struct sockaddr_in addr;

	//Validar los argumentos de linea de comandos
	port = DEFAULT_PORT;
	if (argc == 2) {
		port = atoi(argv[1]);
		if (port < 1024) {
			port = DEFAULT_PORT;
		}
	}

	sem_init(&mutex, 0, 1); //Inicializar el mutex de registro de clientes
	
	maxclients = 0; //Cantidad maxima de clientes (crece dinamicamente)
	nclients = 0; //Cantidad actual de clientes
	clients = NULL; //Arreglo de clientes, comienza en nulo

	//TODO Implementar!
	//Configurar e instalar el manejador de la señal
	// Se debe instalar los manejadores para las señales:
	// SIGINT
	// SIGHUP
	// SIGTERM
	//Se puede usar la misma estructura para todas las señales.

	// Bandera para indicar la terminacion del programa
	finished = 0;

	//Socket IPv4, de tipo flujo (stream)
	//1. Crear un socket
	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	fprintf(stderr, "Socket created\n");
#endif

	//Preparar la direccion para asociarla al socket
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	//inet_aton("AAA.BBB.CCC.DDD", &addr.sin_addr);
	//inet_aton("0.0.0.0", &addr.sin_addr);
	//tambien se puede usar gethostbyname
	addr.sin_addr.s_addr = INADDR_ANY; //0.0.0.0

	//2. Asociar el socket a una direccion (IPv4)
	if (bind(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0) {
		perror("bind");
		exit(EXIT_FAILURE);
	}

	//3. Poner el socket listo
	if (listen(s, 10) != 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

#ifdef DEBUG
	fprintf(stderr, "Listening at port %d\n", port);
#endif

	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(struct sockaddr_in));
	socklen_t client_addr_len;
	client_addr_len = sizeof(struct sockaddr_in); //Tamaño esperado de la direccion


	while(!finished) {
		//4. Aceptar la conexion
		//el sistema "llena" client_addr con la informacion del cliente
		//y almacena en client_addr_len el tamaño obtenido de esa direccion
#ifdef DEBUG
		fprintf(stderr, "Waiting for conections...\n");
#endif
		c = accept(s, (struct sockaddr *)&client_addr, &client_addr_len);
		if (c < 0) {
			perror("accept");
			finished = 1;
			continue;
		}
		//Crear un hilo pasandole como parametro c (almacenado en un "arreglo")
		//Registrar el nuevo cliente

		//Seccion critica
		down(&mutex);
		client * new_client  = add_client(c, &client_addr);
		if (new_client != NULL) {
#ifdef DEBUG
			fprintf(stderr, "Client connected from %s. Socket = %d. Index = %d\n", 
										new_client->ip,
										new_client->socket,
										new_client->index);
#endif
			pthread_t t;
			// Si el cliente se pudo adicionar al arreglo, crear un nuevo hilo
			// pasando como parametro el apuntador a la estructura del nuevo cliente
			pthread_create(&t, NULL, client_thread, new_client);
		}
		up(&mutex);
		// Fin de la seccion critica
	}

	//Cerrar el socket original para liberar el puerto
	close(s);

	exit(EXIT_SUCCESS);
}

void * client_thread(void * arg) {
	client * c;
	int nread;

	int client_finished; //Indicador de fin de comunicacion con el cliente

	c = (client*)arg;

	client_finished = 0;

	while (!client_finished && !finished) {
		request r;

		memset(&r, 0, sizeof(request));
		//Leer la solicitud del cliente
		if ( (nread = read(c->socket, (char*)&r, sizeof(request))) != sizeof(request)) {
#ifdef DEBUG
			fprintf(stderr, "Request must be %d bytes, only %d bytes received.\n", sizeof(request), nread);
#endif
			client_finished = 1;
			continue;
		}

		if (EQUALS(r.operation, "get")) {
			//enviar archivo
			char ruta[PATH_MAX];
			sprintf(ruta, "files/%s", r.filename);
			if (!send_file(c->socket, ruta)) {
				fprintf(stderr, "Error sending %s\n", ruta);
			}
		}else if (EQUALS(r.operation, "put")) {

			response resp;

			//Verificar si no existe una transferencia activa con el mismo archivo
			down(&mutex);
			resp.code = register_transfer(c, r.filename, resp.message);
			up(&mutex);

			//Enviar la respuesta al cliente
			if (write(c->socket, &resp, sizeof(response)) != sizeof(response)) {
				fprintf(stderr, "Error sending response to client\n");
			}

			//Si no se pudo registrar la transferencia, no se va a recibir ningun archivo
			if (resp.code == TRANSFER_DENIED) {
				continue;
			}

			//recibir archivo
			if (!receive_file(c->socket, "files")) {
				fprintf(stderr, "Error receiving file %s\n", r.filename);
			}

			//Retirar la transferencia
			down(&mutex);
			finalize_transfer(c);
			up(&mutex);

		}else if(EQUALS(r.operation, "exit")) {
			//cerrar la conexion con el cliente
			client_finished = 1;
		}
	}
#ifdef DEBUG
	fprintf(stderr, "Closing the connection with the client %s Index %d, socket = %d..\n", c->ip, c->index, c->socket);
#endif
	//Cerrar la conexion con el cliente
	close(c->socket);

#ifdef DEBUG
fprintf(stderr, "Removing client %s Index %d, socket = %d\n", c->ip, c->index, c->socket);
#endif
	//Quitar el cliente
	down(&mutex);
	remove_client(c);
	up(&mutex);
}

client * add_client(int c, struct sockaddr_in * addr) {
	client * new_client;

	new_client = NULL;

	//Es necesario aumentar la cantidad de clientes?
	if (nclients == maxclients) {
		client * new_clients;
		maxclients += 5; //Abrir espacio para 5 clientes más
		//Crear un nuevo arreglo con mayor capacidad
		new_clients = (client*)malloc(maxclients * sizeof(client));
		//Llenar el nuevo arreglo con 0
		memset(new_clients, 0, maxclients * sizeof(client));
		if (nclients > 0) {
			//Copiar los datos del arreglo anterior al arreglo nuevo
			memcpy(new_clients, clients, nclients * sizeof(client));
			//Liberar el arreglo anterior
			free(clients);
		}
		//Guardar el apuntador al arreglo
		clients = new_clients;
	}
	//Buscar una posicion libre en el arreglo
	for (int i = 0; i < maxclients; i++) {
		if (clients[i].socket <= 0) {
			// Limpiar el espacio!
			memset(&clients[i], 0, sizeof(client));
			clients[i].socket = c;
			clients[i].index = i;
			memcpy(&clients[i].address, addr, sizeof(struct sockaddr_in));
			if (inet_ntop(
					clients[i].address.sin_family, 
					&clients[i].address.sin_addr, 
					clients[i].ip, 
					INET_ADDRSTRLEN) == NULL) {
#ifdef DEBUG
					fprintf(stderr, "Warning! unable to translate the client address\n");
#endif
			}

			//Guarar la referencia al nuevo cliente
			new_client = &clients[i];
			//Incrementar la cantidad de clientes registrados
			nclients++;
			break;
		}
	}
	return new_client;
}

void remove_client(client * c) {
	//Borrar la informacion del cliente
	memset(c, 0, sizeof(client));
	//Decrementar la cantidad de clientes conectados
	nclients--;
	printf("Active clients remaining: %d\n", nclients);
	//Liberar la memoria si la cantidad de clientes es cero
	if (nclients == 0) {
		maxclients = 0;
		free(clients);
		clients = NULL;
	}
}

void signal_handler(int sig) {
	//Informar que el servidor esta terminando
	finished = 1;
	// TODO implementar!
	// Recorrer el arreglo de clientes, cerrando cada uno de los sockets activos
	//Liberar el arreglo de clientes
	free(clients);
	printf("Server finished.\n");
	exit(EXIT_SUCCESS);
}

response_code register_transfer(client * c, char * filename, char * message) {
	response_code code;
	code = TRANSFER_DENIED;

	// TODO Borrar cuando la logica este implementada
	strcpy(message, "TODO implementar!");

	//TODO Implementar!

	// Buscar en el arreglo de clientes activos (socket > 0) diferentes al
	// cliente actual, si el nom bre de la transferencia activa (active_transfer) es igual
	// al nombre del archivo que este cliente desea transferir

	// Si se encuentra algun cliente cuyo atributo active_transfer es igual a filename,
	// el codigo es TRANSFER_DENIED.

	// En caso que ningun cliente este transfiriendo un archivo con el mismo nombre,
	// el codigo es TRANSFER_AUTHORIZED

	return code;

}

void finalize_transfer(client * c) {
	// TODO implementar!
	// Limpiar el nombre de la transferencia activa
}

