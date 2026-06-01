Simulación Cliente - Intermediario - Servidor con hilos

Como el protocolo escogido luego de la discusión intragrupal corresponde al Uppercase v3.0 del equipo 4 grupo 3 (Mismo con el que ya trabajamos en la anterior etapa) la simulación se mantiene utilizando la misma comunicación y comandos de la etapa anterior.

Se sigue trabajando con colas circulares protegidas con mutex.

Seguimos utilizando enqueue y dequeue para ingresar y extraer los mensajes de una cola a otra.

SEguimos trabajando con 4 colas para el proceso de transporte de consultas y respuestas por los distintas partes de la simulación del sistema:

ColaRequest: desde cliente a intermediario, se pasa la consulta.
ColaToServer: desde intermediario a servidor, se valida y pasa la consulta al server.
ColaFromServer: desde servidor a intermediario, luego de procesar la consulta se transporta la respuesta del server.
ColaResponse: desde intermediario a cliente, se le devuelve al cliente la respuesta a su respectiva consulta.