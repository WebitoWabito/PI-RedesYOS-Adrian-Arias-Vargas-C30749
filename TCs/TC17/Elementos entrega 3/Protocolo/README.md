Protocolo Intragrupal

Uppercase v3.0 (G03)

Luego de discutir entre ambos grupos se llegó a la conclusión de implementar el protocolo Uppercase v3.0 prpuesto por el equipo 4 grupo 3, mismo protocolo con el que anteriormente trabajamos la etapa 2.

Se trabaja la misma estructura de comunicación:

[Protocolo]/[Comando]/[mensaje]

Implementando los mismos comandos ya definidos:

ACK          A  Indica que el comando se hizo correctamente 
Connect      C  Crear conexión inicial fork-server 
Data         D  Respuesta del server (directorios, figuras) 
Fork         F  Comunicación entre Forks 
Get          G  Solicitud de figuras 
Kill         K  Servidor anuncia su muerte 
Patch        P  Actualización de servidores 
Quit         Q  Cerrar conexión fork-server 
Request      R  Solicitud de servidores o directorios 
Server Data  S  Dirección IP y puerto del servidor 