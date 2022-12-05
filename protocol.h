#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>


#define DEFAULT_PORT 1234 /**< Puerto por defecto */

/**
* @brief Solicitud del cliente
*/
typedef struct {
	char operation[16]; /**< Operacion a realizar (get, put, exit)*/
	char filename[NAME_MAX]; /**< Nombre del archivo a transferir */
}request;

/**
* @brief Codigos de respuesta
*/
typedef enum {
	TRANSFER_AUTHORIZED,
	TRANSFER_DENIED,
} response_code;

/**
* @brief Respuesta del servidor para verificar archivos repetidos
*/
typedef struct {
	response_code code; /**< Codigo de respuesta */
	char message[256]; /**< Mensaje (opcional) */
}response;

/**
* @brief Informacion de un archivo a transferir
*/
typedef struct {
	int size; /**< Tamano del archivo a transferir, -1 si hay error ej. no existe*/
	char filename[NAME_MAX]; /**< Nombre del archivo */
	mode_t mode; /**< Modo */
}file_info;

#define EQUALS(s1, s2) (strcmp(s1, s2) == 0)


/**
* @brief Recibe del socket un encabezado con la informacion de un archivo
* @param s Socket
* @param info Referencia a la estructura file_info
* @return 1 si se recibio el encabezado, 0 en caso contrario
*/
int receive_file_info(int s, file_info * info);

/**
* @brief Envia al socket un encabezado con la informacion del archivo
* @param s Socket
* @param info Referencia a la estructura file_info
* @return 1 si se envio el encabezado, 0 en caso contrario
*/
int send_file_info(int s, file_info * info);

/**
* @brief Recibe un archivo y lo almacena en la carpeta especificada
* @param s Socket del cual se recibe el archivo
* @param destination_folder Carpeta en la cual se almacenara el archivo
*
* @return 1 en caso de exito, 0 si ocurre un error
*/
int receive_file(int s, char * destination_folder);

/**
* @brief Envia un archivo
* @param s Socket al cual se envia el archivo
* @param path Ruta al archivo a enviar
*
* @return 
*/
int send_file(int s, char * path);

/**
* @brief Verifica si un archivo existe
* @param path Ruta al archivo
*
* @return 1 si el archivo existe, 0 en caso contrario
*/
int file_exists(char * path);

#endif
