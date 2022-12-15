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
	file_info info;
	char *ruta;
	int dst_fd;
	int faltantes;

	// Lee la informacion del archivo desde el socket
	// receive_file_info(s, info) 

	if (!receive_file_info(s, &info))
	{
		perror("No se recibio el encabezado...");
	}

	// Crea la ruta en la cual se almacena el archivo:

	if (!dir_exists(destination_folder))
	{
		if (mkdir(destination_folder, 0755) < 0)
		{
			fprintf(stderr, "No se pudo crear el directorio %s\n", destination_folder);
			return 0;
		}
	}

	ruta = (char *)malloc(strlen(destination_folder) + strlen(info.filename) + 2);
	strcpy(ruta, destination_folder);
	strcat(ruta, "/");
	strcat(ruta, info.filename);
#ifdef DEBUG
			//
			printf("ruta donde recibo  %s\n", ruta);
#endif
	//  Abre el archivo de salida
	// Lee el contenido del archivo.
	
	dst_fd = open(ruta, O_CREAT | O_WRONLY | O_TRUNC, info.mode); 
	if (dst_fd < 0)
	{
#ifdef DEBUG
		//
		fprintf(stderr, "Error abriendo %s\n", ruta);
#endif
		return 0;
	}
	//faltantes = tamaño del archivo a recibir
	faltantes = info.size;
#ifdef DEBUG
	//
	printf("Cantidad recibida %d\n", faltantes);
#endif
	while (faltantes > 0)
	{
		int a_leer = BUFSIZ;
		if (a_leer > faltantes)
		{
			a_leer = faltantes;
		}
		//      // Lee un bloque del socket
		n_leidos = read(s, buff, a_leer);
#ifdef DEBUG
		//
		printf("Cantidad leida %d\n", n_leidos);
#endif
	
		if (n_leidos > 0)
		{
			//       // Escribe el bloque al archivo
			n_escritos = write(dst_fd, buff, n_leidos);
#ifdef DEBUG
			//
			printf("Cantidad escrita %d\n", n_escritos);
#endif
			if (n_escritos != n_leidos)
			{
				fprintf(stderr, "Error la cantidad escrita %d es diferente a la leida %d", n_escritos, n_leidos);
			}
		}
		
		faltantes = faltantes - n_leidos;
#ifdef DEBUG
		//
		printf("Cantidad faltante %d\n", faltantes);
#endif
	}
	// Cierra el archivo
	close(dst_fd);

	return 1;
}

int send_file(int s, char *path)
{
	size_t n_leidos;
	size_t n_escritos;
	file_info info;
	char buff[BUFSIZ];
	char *filename;
	char *ruta;
	int src_fd;
	struct stat st;
	int faltantes;

	// Obtiene el nombre del archivo de la ruta
	filename = basename(path);

	// Llena los atributos de la estructura de tipo file_info

	// Obtiene la ruta absoluta del archivo
	ruta = realpath(path, NULL);
	
	if (ruta == NULL)
	{
		info.size = -1;
		if (!send_file_info(s, &info))
		{
			perror("Error no se pudo enviar la informacion del archivo");
		}

		return 0;
	}
	// Verifica que la ruta corresponde a un archivo regular
	if (stat(ruta, &st) != 0 || !S_ISREG(st.st_mode))
	{
		info.size = -1;
		if (!send_file_info(s, &info))
		{
			perror("Error no se pudo enviar la informacion del archivo");
		}
		return 0;
	}
	

	// Llena los atributos del encabezado

	strcpy(info.filename, filename);
	info.size = st.st_size;
	info.mode = st.st_mode;

	// Envia el encabezado

	
	if (!send_file_info(s, &info))
	{
		perror("Error no se pudo enviar la informacion del archivo");
	}
	// Abre el archivo que se va a enviar
	src_fd = open(ruta, O_RDONLY);
	if (src_fd != -1)
	{
	
		//Envia la informacion del archivo
		faltantes = st.st_size;
		while (faltantes > 0)
		{
			int a_leer = BUFSIZ;
			
			if (a_leer > faltantes)
			{
				a_leer = faltantes;
			}
			//      // Lee un bloque del socket
			n_leidos = read(src_fd, buff, a_leer);
			//		  si n_leidos > 0
			if (n_leidos > 0)
			{
				//       // Escribe el bloque al archivo
				n_escritos = write(s, buff, n_leidos);
				if (n_escritos != n_leidos)
				{
					fprintf(stderr, "Error la cantidad escrita %d es diferente a la leida %d", n_escritos, n_leidos);
				}
			}
		
			faltantes = faltantes - n_leidos;
		}
	}
	else
		return 0;

	// Cierra el archivo
	close(src_fd);
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
int dir_exists(char *path)
{
	struct stat s;

	if (stat(path, &s) != 0 || !S_ISDIR(s.st_mode))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
