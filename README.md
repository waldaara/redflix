# Aplicación de Servidor y Cliente

Este proyecto consiste en una aplicación de servidor y cliente para simular un sistema de transmisión de video.

## Requisitos

- GCC (compilador)
- Make
- Biblioteca pthread

## Estructura de Archivos

- `server.c`: Código fuente del servidor.
- `client.c`: Código fuente del cliente.
- `Makefile`: Script para compilar los programas.
- `README.txt`: Este archivo.

## Cómo Compilar

1. Abre una terminal en el directorio del proyecto.
2. Para compilar el servidor y el cliente:
   ```
   make
   ```
3. Para compilar solo el servidor o el cliente:
   - Servidor: `make server`
   - Cliente: `make client`
4. Para eliminar los archivos compilados:
   ```
   make clean
   ```

## Uso

### Iniciar el Servidor

Ejecuta el servidor con:
```
./server -p <puerto>
```

### Iniciar el Cliente

Ejecuta el cliente con:
```
./client -i <ip_servidor> -p <puerto>
```

### Comandos del Cliente

1. `1` - Reproducir video.
2. `2` - Pausar video.
3. `3` - Detener video y cerrar conexión.
4. `4` - Calidad baja.
5. `5` - Calidad media.
6. `6` - Calidad alta.

## Notas

- El servidor transmite cuadros de video desde `video.txt`.
- El cliente puede controlar la transmisión con los comandos mencionados.
