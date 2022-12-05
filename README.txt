Elaborado por: Erwin Meza Vega <emezav@unicauca.edu.co>

Se deberá desarrollar dos programas en C que implementen la funcionali-
dad para enviar y recibir archivos a través de la red. Uno de los dos extremos
actuará como cliente, y el otro como servidor. Quien recibe conexiones (el ser-
vidor) esperará por comandos del cliente, para transferir archivos desde o hacia
el servidor.

Los archivos en el servidor se almacenarán en un directorio llamado "files".

Los comandos soportados son:

* get ARCHIVO: Transfiere un archivo desde el servidor hacia el cliente. El
	cliente deberá enviar un primer mensaje al servidor con la solicitud (el
	nombre del archivo a transferir), a lo cual el servidor responderá con un
	mensaje que contiene la información del archivo (por ejemplo, el tamaño
	en bytes), seguido del contenido del archivo. Si el archivo no existe, se
	deberá informar al cliente el error, y no se enviará nada a continuación
	de este mensaje.

*	put ARCHIVO: Transfiere un archivo desde el cliente hacia el servidor. El
	cliente deberá enviar un mensaje al servidor con la solicitud (el nombre
	del archivo a transferir y su tamaño en bytes), seguido del contenido del
	archivo. El servidor recibirá el mensaje, y con la información suministra-
	da, creará el archivo, leerá el contenido del socket y lo escribirá en el
	archivo (nuevo o existente).
* exit: El cliente deberá enviar un mensaje al servidor indicando que se va
	a cerrar la conexión.

Se debe permitir la comunicación entre un servidor y múltiples 
clientes, por lo cual en el servidor se deberá crear y administrar múltiples hilos,
uno por cada conexión. Se debe tener en cuenta que varios clientes pueden
tratar de enviar el mismo archivo al servidor. Si se presenta esta situación, el
servidor deberá rechazar el envío.

El programa servidor recibirá por argumento de línea de comandos el puerto 
(opcional, mayor que 1024) en el cual aceptará las conexiones.

El programa cliente recibirá por argumento de línea de comandos la dirección 
IP del servidor, y el número del puerto (opcional, mayor que 1024)

Si no se especifica el número de puerto, se usará el puerto número 1024.

Tanto el servidor como el cliente deberán terminar cuando se reciba la señal
SIGTERM.
