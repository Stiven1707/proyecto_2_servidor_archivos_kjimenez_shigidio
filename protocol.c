#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "protocol.h"

int receive_file_info(int s, file_info *info)
{
	int nread;
	nread = read(s, info, sizeof(file_info));
	return (nread == sizeof(file_info));
}

int send_file_info(int s, file_info *info)
{
	int nwritten;
	nwritten = write(s, info, sizeof(file_info));
	return (nwritten == sizeof(file_info));
}

int receive_file(int s, char *destination_folder)
{
	char buff[BUFSIZ];
	size_t n_leidos;
	size_t n_escritos;
	file_info *info;
	char *ruta;
	int dst_fd;
	int faltantes;
	// Recibe un archivo de un socket y lo almacena en destination_folder

	// Primero se debe leer el socket el encabezado (de tipo file_info)
	// que contiene la informacion del archivo:

	// info.filename = nombre del archivo
	// info.size = tamaño del archivo
	// info.mode = modo (permisos).

	// Se debe validar que se leyo el encabezado completo

	// Luego se lee el contenido del archivo en bloques de BUFSIZ.

	// Se debe tener en cuenta que el ultimo bloque (o el primero y unico, si el
	// tamaño es menor que BUFSIZ) va a contener nulos al final que no deben ser
	// escritos en el archivo de salida.

	// =============

	// 1. Leer la informacion del archivo desde el socket (estructura file_info)
	// receive_file_info(s, info) //TODO validar!

	if (!receive_file_info(s, info))
	{
		perror("No se recibio el encabezado...");
	}

	// 2. Crear la ruta en la cual se almacena el archivo:

	//	  destination_folder + "/" + file_info.filename
	//    El modo se encuentra en file_info.mode
	//  ruta=malloc(...)
	ruta = (char *)malloc(strlen(destination_folder) + strlen(info->filename) + 2);
	strcpy(ruta, destination_folder);
	strcat(ruta, "/");
	strcat(ruta, info->filename);

	// 3. Abrir el archivo de salida

	// Leer el contenido del archivo.
	// La transferencia se realiza en bloques de BUFSIZ bytes
	// Se debe prestar atencion al ultimo bloque, ya que puede ser
	// de tamaño menor.
	// En caso que el archivo sea pequeño, la transferencia se realizara en una
	// sola lectura
	dst_fd = open(ruta, O_CREAT | O_WRONLY | O_TRUNC, info->mode); // TODO validar!
	if (dst_fd != 0)
	{
		return 0;
		
	}
	else
		// 4. faltantes = tamaño del archivo a recibir (tomado de la estructura file_info)
		faltantes = info->size;
		// 4.1 Mientras faltantes > 0
		while (faltantes > 0)
		{
			int a_leer = BUFSIZ;
			//			si a_leer > faltantes // Ultimo bloque
			//				a_leer = faltantes
			//			fin si
			if (a_leer > faltantes)
			{
				a_leer = faltantes;
			}
			//      // Leer un bloque del socket
			//			n_leidos = read(s, buf, a_leer) //TODO validar!
			n_leidos = read(s, buff, a_leer);
			//		  si n_leidos > 0
			if (n_leidos > 0)
			{
				//       // Escribir el bloque al archivo
				//				n_escritos = write(dst_fd, buf, n_leidos) //TODO validar!
				n_escritos = write(dst_fd, buff, n_leidos);
				if (n_escritos != n_leidos)
				{
					fprintf(stderr,"Error la cantidad escrita %d es diferente a la leida %d",n_escritos,n_leidos);
				}
				
			}
			//      fin si

			//			faltaltes = faltantes - n_leidos
			faltantes = faltantes - n_leidos;
			//     Fin Mientras
		}
	// 5. Cerrar el archivo
	// close(dst_fd)
	close(dst_fd);

	// 6. Retornar OK
	// return 1
	return 1;
}

int send_file(int s, char *path)
{
	size_t n_leidos;
	size_t n_escritos;
	file_info *info;
	char buff[BUFSIZ];
	char *filename;
	char *ruta;
	int src_fd;
	struct stat st;
	int faltantes;
	// Verificar si el archivo existe
	// Si el archivo no existe, enviar un encabezado (file_info)
	// con size = -1 y retornar 0

	// Si el archivo existe y es un archivo regular,
	// enviar primero el encabezado con la informacion del archivo:
	// info.size = tamaño del archio
	// info.filename = nombre el archivo (sin directorio)
	// info.mode = modo del archivo

	// El tamaño y el modo se obtienen con stat (2).

	// Despues de enviar el encabezado, se debe enviar el contenido
	// del archivo en bloques de BUFSIZ. Se debe tener en cuenta
	// que el ultimo bloque (o el primero y unico, si el archivo es pequeño)
	// llevara caracters nulos al final.

	// ====================

	// 1. Obtener el nombre del archivo de la ruta
	// filename = basename(path)
	filename = basename(path);

	// Llenar los atributos de la estructura de tipo file_info

	// 2. Obtener la ruta absoluta del archivo
	// realpath obtiene la ruta absolutam retornando NULL si no existe.
	ruta = realpath(path, NULL);
	// ruta = realpath(path, NULL)
	// si ruta == NULL
	// info.size = -1
	//  send_file_info(s, info) //TODO validar!
	//  retornar 0 (falso)
	// fin si
	if (ruta == NULL)
	{
		info->size = -1;
		if (!send_file_info(s, info))
		{
			perror("Error no se pudo enviar la informacion del archivo");
		}

		return 0;
	}
	// 3. Verificar que la ruta corresponde a un archivo regular
	// si stat(ruta, stat_s) != 0
	if (stat(ruta, &st) != 0 || !S_ISREG(st.st_mode))
	{
		//  info.size = -1
		//  send_file_info(s, info) // TODO validar!
		//  retornar 0 (falso)
		info->size = -1;
		if (!send_file_info(s, info))
		{
			perror("Error no se pudo enviar la informacion del archivo");
		}
		return 0;
	}
	// fin si

	// 4. Llenar los atributos del encabezado

	// info.filename = filenane
	strcpy(info->filename, filename);
	// info.size = stat_s.st_size
	info->size = st.st_size;
	// info.mode = stat_s.st_mode
	info->mode = st.st_mode;

	// 5. Enviar el encabezado

	// send_file_info(s, info)  //TODO validar!
	if (!send_file_info(s, info))
	{
		perror("Error no se pudo enviar la informacion del archivo");
	}
	// 6. Abrir el archivo que se va a enviar
	// src_fd = open(ruta, O_RDONLY) //TODO validar!
	src_fd = open(ruta, O_RDONLY);
	if (src_fd != -1)
	{
		// 7. Enviar el contenido archivo.
		// La transferencia se realiza en bloques de BUFSIZ bytes
		// Se debe prestar atencion al ultimo bloque, ya que puede ser
		// de tamaño menor.
		// En caso que el archivo sea pequeño, la transferencia se realizara en una
		// sola lectura

		// 8. faltantes = stat.st_size // Cantidad de bytes del archivo
		faltantes = st.st_size;
		while (faltantes > 0)
		{
			int a_leer = BUFSIZ;
			//			si a_leer > faltantes // Ultimo bloque
			//				a_leer = faltantes
			//			fin si
			if (a_leer > faltantes)
			{
				a_leer = faltantes;
			}
			//      // Leer un bloque del socket
			//			n_leidos = read(src_fd, buf, a_leer) //TODO validar!
			n_leidos = read(src_fd, buff, a_leer);
			//		  si n_leidos > 0
			if (n_leidos > 0)
			{
				//       // Escribir el bloque al archivo
				//				n_escritos = write(s, buf, n_leidos) //TODO validar!
				n_escritos = write(s, buff, n_leidos);
				if (n_escritos != n_leidos)
				{
					fprintf(stderr,"Error la cantidad escrita %d es diferente a la leida %d",n_escritos,n_leidos);
				}
			}
			//      fin si

			//			faltaltes = faltantes - n_leidos
			faltantes = faltantes - n_leidos;
			//     Fin Mientras
		}
	}
	else
		return 0;

	// 9. Cerrar el archivo
	// close(src_fd)
	close(src_fd);
	// 10. Retornar OK
	// return 1
	return 1;
}

int file_exists(char *path)
{
	struct stat s;

	if (stat(path, &s) != 0 || !S_ISREG(s.st_mode))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
