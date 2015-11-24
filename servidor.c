/*******************************************************
Protocolos de Transporte
Grado en Ingeniería Telemática
Dpto. Ingeníería de Telecomunicación
Univerisdad de Jaén

Fichero: servidor.c
Versión: 1.1
Fecha: 13/10/2015
Descripción:
	Servidor de eco sencillo TCP.

Autor: Juan Carlos Cuevas Martínez
Modificado por: Antonio Perez Pozuelo y David Sanchez Fernandez
*******************************************************/
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <Winsock2.h>

#include "protocol.h"




main()
{

	WORD wVersionRequested;
	WSADATA wsaData;
	SOCKET sockfd,nuevosockfd;
	struct sockaddr_in  local_addr,remote_addr; //Estructura sockaddr particularizada para IPV4
	char buffer_out[1024],buffer_in[1024], cmd[10], usr[10], pas[10];
	int err,tamanio;
	int fin=0, fin_conexion=0;
	int recibidos=0,enviados=0;
	int estado=0,i;
	char num1[4],num2[4],cad[20];

	/** INICIALIZACION DE BIBLIOTECA WINSOCK2 **
	 ** OJO!: SOLO WINDOWS                    **/
	wVersionRequested=MAKEWORD(1,1);
	err=WSAStartup(wVersionRequested,&wsaData);
	if(err!=0){
		return(-1);
	}
	if(LOBYTE(wsaData.wVersion)!=1||HIBYTE(wsaData.wVersion)!=1){
		WSACleanup() ;
		return(-2);
	}
	/** FIN INICIALIZACION DE BIBLIOTECA WINSOCK2 **/


	sockfd=socket(AF_INET,SOCK_STREAM,0);//Creación del socket; CON AF_INET LE INDICAMOS QUE ES IPV4, CON SOCK_STREAM LE INDICAMOS QUE USE TCP, CON 0 USAMOS EL PROTOCOLO POR DEFECTO DE LA FAMILIA EN NUESTRO CASO TCP

	if(sockfd==INVALID_SOCKET)	{
		return(-3);
	}
	else {
		local_addr.sin_family		=AF_INET;			// Familia de protocolos de Internet
		local_addr.sin_port			=htons(TCP_SERVICE_PORT);	// Puerto del servidor
		local_addr.sin_addr.s_addr	=htonl(INADDR_ANY);	// Direccion IP del servidor Any cualquier disponible
													// Cambiar para que conincida con la del host
	}
	
	// Enlace el socket a la direccion local (IP y puerto)
	if(bind(sockfd,(struct sockaddr*)&local_addr,sizeof(local_addr))<0)//blind(nuestro socket creado, puntero a la estructura de direccion del socket,tamaño de la estructura) Si devuelve -1 es porque ha fallado la vinculacion
		return(-4);
	
	//Se prepara el socket para recibir conexiones y se establece el tamaño de cola de espera
	if(listen(sockfd,5)!=0)//listen(socket creado y vinculado, numero maximo de solicitudes de conexion en espera)
		return (-6);
	
	tamanio=sizeof(remote_addr);

	do
	{
		printf ("SERVIDOR> ESPERANDO NUEVA CONEXION DE TRANSPORTE\r\n");
		
		nuevosockfd=accept(sockfd,(struct sockaddr*)&remote_addr,&tamanio);//accept(socket en el que escuchara el servidor, estructura direccion del cliente, tamaño de la estructura del cliente)

		if(nuevosockfd==INVALID_SOCKET) {
			
			return(-5);
		}

		printf ("SERVIDOR> CLIENTE CONECTADO\r\nSERVIDOR> [IP CLIENTE]> %s\r\nSERVIDOR> [CLIENTE PUERTO TCP]>%d\r\n",
					inet_ntoa(remote_addr.sin_addr),ntohs(remote_addr.sin_port));//inet_ntoa(remote_addr.sin_addr)->Devuelve la IP como array,ntohs(remote_addr.sin_port)->network to host

		//Mensaje de Bienvenida
		sprintf_s (buffer_out, sizeof(buffer_out), "%s Bienvenindo al servidor de ECO%s",OK,CRLF);
		
		enviados=send(nuevosockfd,buffer_out,(int)strlen(buffer_out),0);
		//TODO Comprobar error de envío
 
		if(enviados==SOCKET_ERROR){
			printf("SERVIDOR> Error enviando datos");
			continue;
		}else if (enviados==0)
		{
			printf("SERVIDOR> Se ha liberado la conexion de forma acordada");
			continue;
		}

		//Se reestablece el estado inicial
		estado = S_USER;
		fin_conexion = 0;

		printf ("SERVIDOR> Esperando conexion de aplicacion\r\n");
		do
		{
			//Se espera un comando del cliente
			recibidos = recv(nuevosockfd,buffer_in,1023,0);
			
			//TODO Comprobar posible error de recepción
			
			if(recibidos==SOCKET_ERROR){
				printf ("SERVIDOR> Error recibiendo datos");
				fin_conexion=1;
					continue;
			}else if (recibidos==0)
			{
				printf("SERVIDOR> Se ha liberado la conexion de forma acordada");
			fin_conexion=1;
			continue;
				}

			
			buffer_in[recibidos] = 0x00;
			printf ("SERVIDOR> [bytes recibidos]> %d\r\nSERVIDOR> [datos recibidos]>%s", recibidos, buffer_in);
			
			switch (estado)
			{
				case S_USER:    /*****************************************/
					strncpy_s ( cmd, sizeof(cmd),buffer_in, 4); //Se copian los 4 primeros caracteres que hemos recibido
					cmd[4]=0x00; // en C los arrays finalizan con el byte 0000 0000

					if ( strcmp(cmd,SC)==0 ) // si recibido es solicitud de conexion de aplicacion
					{
						sscanf_s (buffer_in,"USER %s\r\n",usr,sizeof(usr));
						
						// envia OK acepta todos los usuarios hasta que tenga la clave
						sprintf_s (buffer_out, sizeof(buffer_out), "%s%s", OK,CRLF);
						
						estado = S_PASS;
						printf ("SERVIDOR> Esperando clave\r\n");
					} else
					if ( strcmp(cmd,SD)==0 )
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Fin de la conexión%s", OK,CRLF);
						fin_conexion=1;
					}
					else
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Comando incorrecto%s",ER,CRLF);
					}
				break;

				case S_PASS: /******************************************************/

					
					strncpy_s ( cmd, sizeof(cmd), buffer_in, 4);
					cmd[4]=0x00; // en C los arrays finalizan con el byte 0000 0000

					if ( strcmp(cmd,PW)==0 ) // si comando recibido es password
					{
						sscanf_s (buffer_in,"PASS %s\r\n",pas,sizeof(usr));

						if ( (strcmp(usr,USER)==0) && (strcmp(pas,PASSWORD)==0) ) // si password recibido es correcto
						{
							// envia aceptacion de la conexion de aplicacion, nombre de usuario y
							// la direccion IP desde donde se ha conectado
							sprintf_s (buffer_out, sizeof(buffer_out), "%s %s IP(%s)%s", OK, usr, inet_ntoa(remote_addr.sin_addr),CRLF);
							estado = S_DATA;
							printf ("SERVIDOR> Esperando comando\r\n");
						}
						else
						{
							sprintf_s (buffer_out, sizeof(buffer_out), "%s Autenticación errónea%s",ER,CRLF);
							estado= S_USER;
						}
					} else
					if ( strcmp(cmd,SD)==0 )
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Fin de la conexión%s", OK,CRLF);
						fin_conexion=1;
					}
					else
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Comando incorrecto%s",ER,CRLF);
					}
				break;

				case S_DATA: /***********************************************************/
					
					buffer_in[recibidos] = 0x00;
					
					strncpy_s(cmd,sizeof(cmd), buffer_in, 4);
					cmd[4]=0x00;
					printf ("SERVIDOR> [Comando]>%s\r\n",cmd);
					if (strcmp(cmd,SU)==0)
					{
						strncpy_s(cad,sizeof(cad), buffer_in, 14);
						cad[14]=0x00;
						for (i = 5; i < 9; i++)
						{
							num1[i-5]=cad[i];
						}
						for (i = 10 ; i < 14; i++)
						{
							num2[i-10]=cad[i];
						}
						if (atoi(num1)+atoi(num2)>0)
						{
							sprintf_s (buffer_out, sizeof(buffer_out), "%s %d%s", OK,atoi(num1)+atoi(num2),CRLF);
						}else
						{
							sprintf_s (buffer_out, sizeof(buffer_out), "%s Ha habido un error de comando%s", ER,CRLF);
						}
					}
					
					else if ( strcmp(cmd,SD)==0 )
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Fin de la conexión%s", OK,CRLF);
						fin_conexion=1;
					}
					else if (strcmp(cmd,SD2)==0)
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Finalizando servidor%s", OK,CRLF);
						fin_conexion=1;
						fin=1;
					}
					else
					{
						sprintf_s (buffer_out, sizeof(buffer_out), "%s Comando incorrecto%s",ER,CRLF);
					}
					break;

				default:
					break;
					
			} // switch

			enviados=send(nuevosockfd,buffer_out,(int)strlen(buffer_out),0);
			//TODO 
				if (enviados==SOCKET_ERROR)
					{
						printf("SERVIDOR> Error enviando datos\r\n");
						fin_conexion=1;
				}else if (enviados==0)
				{
					printf("SERVIDOR> Se ha liberado la conexion de forma acordada");
					fin_conexion=1;
				}
				
				else
					{
						printf("SERVIDOR> Se han enviado los datos: %s",buffer_out);
					}

		} while (!fin_conexion);
		printf ("SERVIDOR> CERRANDO CONEXION DE TRANSPORTE\r\n");
		shutdown(nuevosockfd,SD_SEND);
		closesocket(nuevosockfd);

	}while(!fin);

	printf ("SERVIDOR> CERRANDO SERVIDOR\r\n");

	return(0);
} 
